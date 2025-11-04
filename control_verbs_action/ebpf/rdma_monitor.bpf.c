// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/* Copyright (c) 2020 Facebook */
#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>
#include "rdma_monitor.h"

char LICENSE[] SEC("license") = "Dual BSD/GPL";

struct {
	__uint(type, BPF_MAP_TYPE_RINGBUF);
	__uint(max_entries, 256 * 1024);
} rb SEC(".maps");

// Map to store global resource counts
struct {
	__uint(type, BPF_MAP_TYPE_ARRAY);
	__uint(max_entries, 1);
	__type(key, __u32);
	__type(value, struct resource_stats);
} resource_counts SEC(".maps");

// Map to store per-cgroup statistics
struct {
	__uint(type, BPF_MAP_TYPE_HASH);
	__uint(max_entries, MAX_CGROUPS);
	__type(key, __u64);  // cgroup ID
	__type(value, struct cgroup_stats);
} cgroup_stats SEC(".maps");

SEC("kprobe/mlx5_ib_create_qp")
int BPF_KPROBE(ib_uverbs_post_send)
{

	struct event *e;
	pid_t pid;
	u64 ts;
	u64 cgroup_id;

	pid = bpf_get_current_pid_tgid() >> 32;
	ts = bpf_ktime_get_ns();
	cgroup_id = bpf_get_current_cgroup_id();
    
	/* reserve sample from BPF ringbuf */
	e = bpf_ringbuf_reserve(&rb, sizeof(*e), 0);
	if (!e)
		return 0;

	e->pid = pid;
	e->type = RDMA_MONITOR_QP_CREATE;
	e->cgroup_id = cgroup_id;
	bpf_get_current_comm(&e->comm, sizeof(e->comm));
    
	/* successfully submit it to user-space for post-processing */
	bpf_ringbuf_submit(e, 0);

	/* Update global QP count */
	__u32 key = 0;
	struct resource_stats *stats = bpf_map_lookup_elem(&resource_counts, &key);
	if (stats) {
		__sync_fetch_and_add(&stats->qp_count, 1);
	}
	
	/* Update per-cgroup statistics */
	struct cgroup_stats *cg_stats = bpf_map_lookup_elem(&cgroup_stats, &cgroup_id);
	if (!cg_stats) {
		struct cgroup_stats new_cg_stats = {};
		bpf_map_update_elem(&cgroup_stats, &cgroup_id, &new_cg_stats, BPF_ANY);
		cg_stats = bpf_map_lookup_elem(&cgroup_stats, &cgroup_id);
	}
	
	if (cg_stats) {
		cg_stats->counts[RDMA_MONITOR_QP_CREATE]++;
	}

	return 0;
}

SEC("kprobe/mlx5_ib_modify_qp")
int BPF_KPROBE(mlx5_ib_modify_qp, struct ib_qp *ibqp, struct ib_qp_attr *attr,
		      int attr_mask, struct ib_udata *udata)
{

	struct event *e;
	pid_t pid;
	u64 ts;
	u64 cgroup_id;

	pid = bpf_get_current_pid_tgid() >> 32;
	ts = bpf_ktime_get_ns();
	cgroup_id = bpf_get_current_cgroup_id();
    
	/* reserve sample from BPF ringbuf */
	e = bpf_ringbuf_reserve(&rb, sizeof(*e), 0);
	if (!e)
		return 0;

	e->pid = pid;
	e->type = RDMA_MONITOR_QP_MODIFY;
	e->cgroup_id = cgroup_id;
	bpf_probe_read(&e->qp.dest_qpn, sizeof(e->qp.dest_qpn), &attr->dest_qp_num);
	bpf_probe_read(&e->qp.qpn, sizeof(e->qp.qpn), &ibqp->qp_num);
	bpf_probe_read(&e->qp.gid, sizeof(e->qp.gid), &attr->ah_attr.grh.dgid);
	bpf_get_current_comm(&e->comm, sizeof(e->comm));
    
	/* successfully submit it to user-space for post-processing */
	bpf_ringbuf_submit(e, 0);
	
	/* Update per-cgroup statistics */
	struct cgroup_stats *cg_stats = bpf_map_lookup_elem(&cgroup_stats, &cgroup_id);
	if (!cg_stats) {
		struct cgroup_stats new_cg_stats = {};
		bpf_map_update_elem(&cgroup_stats, &cgroup_id, &new_cg_stats, BPF_ANY);
		cg_stats = bpf_map_lookup_elem(&cgroup_stats, &cgroup_id);
	}
	
	if (cg_stats) {
		cg_stats->counts[RDMA_MONITOR_QP_MODIFY]++;
	}

	return 0;
}

SEC("kprobe/mlx5_ib_destroy_qp")
int BPF_KPROBE(mlx5_ib_destroy_qp)
{

	struct event *e;
	pid_t pid;
	u64 ts;
	u64 cgroup_id;

	pid = bpf_get_current_pid_tgid() >> 32;
	ts = bpf_ktime_get_ns();
	cgroup_id = bpf_get_current_cgroup_id();
    
	/* reserve sample from BPF ringbuf */
	e = bpf_ringbuf_reserve(&rb, sizeof(*e), 0);
	if (!e)
		return 0;

	e->pid = pid;
	e->type = RDMA_MONITOR_QP_DESTORY;
	e->cgroup_id = cgroup_id;
	bpf_get_current_comm(&e->comm, sizeof(e->comm));
    
	/* successfully submit it to user-space for post-processing */
	bpf_ringbuf_submit(e, 0);

	/* Update global QP count */
	__u32 key = 0;
	struct resource_stats *stats = bpf_map_lookup_elem(&resource_counts, &key);
	if (stats && stats->qp_count > 0) {
		__sync_fetch_and_sub(&stats->qp_count, 1);
	}
	
	/* Update per-cgroup statistics */
	struct cgroup_stats *cg_stats = bpf_map_lookup_elem(&cgroup_stats, &cgroup_id);
	if (!cg_stats) {
		struct cgroup_stats new_cg_stats = {};
		bpf_map_update_elem(&cgroup_stats, &cgroup_id, &new_cg_stats, BPF_ANY);
		cg_stats = bpf_map_lookup_elem(&cgroup_stats, &cgroup_id);
	}
	
	if (cg_stats) {
		cg_stats->counts[RDMA_MONITOR_QP_DESTORY]++;
	}

	return 0;
}

SEC("kprobe/mlx5_ib_alloc_pd")
int BPF_KPROBE(mlx5_ib_alloc_pd)
{

	struct event *e;
	pid_t pid;
	u64 ts;
	u64 cgroup_id;

	pid = bpf_get_current_pid_tgid() >> 32;
	ts = bpf_ktime_get_ns();
	cgroup_id = bpf_get_current_cgroup_id();
    
	/* reserve sample from BPF ringbuf */
	e = bpf_ringbuf_reserve(&rb, sizeof(*e), 0);
	if (!e)
		return 0;

	e->pid = pid;
	e->type = RDMA_MONITOR_PD_ALLOC;
	e->cgroup_id = cgroup_id;
	bpf_get_current_comm(&e->comm, sizeof(e->comm));
    
	/* successfully submit it to user-space for post-processing */
	bpf_ringbuf_submit(e, 0);

	/* Update global PD count */
	__u32 key = 0;
	struct resource_stats *stats = bpf_map_lookup_elem(&resource_counts, &key);
	if (stats) {
		__sync_fetch_and_add(&stats->pd_count, 1);
	}
	
	/* Update per-cgroup statistics */
	struct cgroup_stats *cg_stats = bpf_map_lookup_elem(&cgroup_stats, &cgroup_id);
	if (!cg_stats) {
		struct cgroup_stats new_cg_stats = {};
		bpf_map_update_elem(&cgroup_stats, &cgroup_id, &new_cg_stats, BPF_ANY);
		cg_stats = bpf_map_lookup_elem(&cgroup_stats, &cgroup_id);
	}
	
	if (cg_stats) {
		cg_stats->counts[RDMA_MONITOR_PD_ALLOC]++;
	}

	return 0;
}

SEC("kprobe/mlx5_ib_dealloc_pd")
int BPF_KPROBE(mlx5_ib_dealloc_pd)
{

	struct event *e;
	pid_t pid;
	u64 ts;
	u64 cgroup_id;

	pid = bpf_get_current_pid_tgid() >> 32;
	ts = bpf_ktime_get_ns();
	cgroup_id = bpf_get_current_cgroup_id();
    
	/* reserve sample from BPF ringbuf */
	e = bpf_ringbuf_reserve(&rb, sizeof(*e), 0);
	if (!e)
		return 0;

	e->pid = pid;
	e->type = RDMA_MONITOR_PD_DEALLOC;
	e->cgroup_id = cgroup_id;
	bpf_get_current_comm(&e->comm, sizeof(e->comm));
    
	/* successfully submit it to user-space for post-processing */
	bpf_ringbuf_submit(e, 0);

	/* Update global PD count */
	__u32 key = 0;
	struct resource_stats *stats = bpf_map_lookup_elem(&resource_counts, &key);
	if (stats && stats->pd_count > 0) {
		__sync_fetch_and_sub(&stats->pd_count, 1);
	}
	
	/* Update per-cgroup statistics */
	struct cgroup_stats *cg_stats = bpf_map_lookup_elem(&cgroup_stats, &cgroup_id);
	if (!cg_stats) {
		struct cgroup_stats new_cg_stats = {};
		bpf_map_update_elem(&cgroup_stats, &cgroup_id, &new_cg_stats, BPF_ANY);
		cg_stats = bpf_map_lookup_elem(&cgroup_stats, &cgroup_id);
	}
	
	if (cg_stats) {
		cg_stats->counts[RDMA_MONITOR_PD_DEALLOC]++;
	}

	return 0;
}

SEC("kprobe/mlx5_ib_create_cq")
int BPF_KPROBE(mlx5_ib_create_cq)
{

	struct event *e;
	pid_t pid;
	u64 ts;
	u64 cgroup_id;

	pid = bpf_get_current_pid_tgid() >> 32;
	ts = bpf_ktime_get_ns();
	cgroup_id = bpf_get_current_cgroup_id();
    
	/* reserve sample from BPF ringbuf */
	e = bpf_ringbuf_reserve(&rb, sizeof(*e), 0);
	if (!e)
		return 0;

	e->pid = pid;
	e->type = RDMA_MONITOR_CQ_CREATE;
	e->cgroup_id = cgroup_id;
	bpf_get_current_comm(&e->comm, sizeof(e->comm));
    
	/* successfully submit it to user-space for post-processing */
	bpf_ringbuf_submit(e, 0);

	/* Update global CQ count */
	__u32 key = 0;
	struct resource_stats *stats = bpf_map_lookup_elem(&resource_counts, &key);
	if (stats) {
		__sync_fetch_and_add(&stats->cq_count, 1);
	}
	
	/* Update per-cgroup statistics */
	struct cgroup_stats *cg_stats = bpf_map_lookup_elem(&cgroup_stats, &cgroup_id);
	if (!cg_stats) {
		struct cgroup_stats new_cg_stats = {};
		bpf_map_update_elem(&cgroup_stats, &cgroup_id, &new_cg_stats, BPF_ANY);
		cg_stats = bpf_map_lookup_elem(&cgroup_stats, &cgroup_id);
	}
	
	if (cg_stats) {
		cg_stats->counts[RDMA_MONITOR_CQ_CREATE]++;
	}

	return 0;
}

SEC("kprobe/mlx5_ib_destroy_cq")
int BPF_KPROBE(mlx5_ib_destroy_cq)
{

	struct event *e;
	pid_t pid;
	u64 ts;
	u64 cgroup_id;

	pid = bpf_get_current_pid_tgid() >> 32;
	ts = bpf_ktime_get_ns();
	cgroup_id = bpf_get_current_cgroup_id();
    
	/* reserve sample from BPF ringbuf */
	e = bpf_ringbuf_reserve(&rb, sizeof(*e), 0);
	if (!e)
		return 0;

	e->pid = pid;
	e->type = RDMA_MONITOR_CQ_DESTORY;
	e->cgroup_id = cgroup_id;
	bpf_get_current_comm(&e->comm, sizeof(e->comm));
    
	/* successfully submit it to user-space for post-processing */
	bpf_ringbuf_submit(e, 0);

	/* Update global CQ count */
	__u32 key = 0;
	struct resource_stats *stats = bpf_map_lookup_elem(&resource_counts, &key);
	if (stats && stats->cq_count > 0) {
		__sync_fetch_and_sub(&stats->cq_count, 1);
	}
	
	/* Update per-cgroup statistics */
	struct cgroup_stats *cg_stats = bpf_map_lookup_elem(&cgroup_stats, &cgroup_id);
	if (!cg_stats) {
		struct cgroup_stats new_cg_stats = {};
		bpf_map_update_elem(&cgroup_stats, &cgroup_id, &new_cg_stats, BPF_ANY);
		cg_stats = bpf_map_lookup_elem(&cgroup_stats, &cgroup_id);
	}
	
	if (cg_stats) {
		cg_stats->counts[RDMA_MONITOR_CQ_DESTORY]++;
	}

	return 0;
}

SEC("kprobe/mlx5_ib_reg_user_mr")
int BPF_KPROBE(mlx5_ib_reg_user_mr, struct ib_pd *pd, u64 start, u64 length, u64 virt_addr, int access_flags)
{

	struct event *e;
	pid_t pid;
	u64 ts;
	u64 cgroup_id;

	pid = bpf_get_current_pid_tgid() >> 32;
	ts = bpf_ktime_get_ns();
	cgroup_id = bpf_get_current_cgroup_id();
    
	/* reserve sample from BPF ringbuf */
	e = bpf_ringbuf_reserve(&rb, sizeof(*e), 0);
	if (!e)
		return 0;

	e->pid = pid;
	e->type = RDMA_MONITOR_MR_REG;
	e->cgroup_id = cgroup_id;
	e->mr.va = virt_addr;
	e->mr.len = length;
	bpf_get_current_comm(&e->comm, sizeof(e->comm));
    
	/* successfully submit it to user-space for post-processing */
	bpf_ringbuf_submit(e, 0);

	/* Update global MR count */
	__u32 key = 0;
	struct resource_stats *stats = bpf_map_lookup_elem(&resource_counts, &key);
	if (stats) {
		__sync_fetch_and_add(&stats->mr_count, 1);
	}
	
	/* Update per-cgroup statistics */
	struct cgroup_stats *cg_stats = bpf_map_lookup_elem(&cgroup_stats, &cgroup_id);
	if (!cg_stats) {
		struct cgroup_stats new_cg_stats = {};
		bpf_map_update_elem(&cgroup_stats, &cgroup_id, &new_cg_stats, BPF_ANY);
		cg_stats = bpf_map_lookup_elem(&cgroup_stats, &cgroup_id);
	}
	
	if (cg_stats) {
		cg_stats->counts[RDMA_MONITOR_MR_REG]++;
	}

	return 0;
}

SEC("kprobe/mlx5_ib_dereg_mr")
int BPF_KPROBE(mlx5_ib_dereg_mr)
{

	struct event *e;
	pid_t pid;
	u64 ts;
	u64 cgroup_id;

	pid = bpf_get_current_pid_tgid() >> 32;
	ts = bpf_ktime_get_ns();
	cgroup_id = bpf_get_current_cgroup_id();
    
	/* reserve sample from BPF ringbuf */
	e = bpf_ringbuf_reserve(&rb, sizeof(*e), 0);
	if (!e)
		return 0;

	e->pid = pid;
	e->type = RDMA_MONITOR_MR_DEREG;
	e->cgroup_id = cgroup_id;
	bpf_get_current_comm(&e->comm, sizeof(e->comm));
    
	/* successfully submit it to user-space for post-processing */
	bpf_ringbuf_submit(e, 0);

	/* Update global MR count */
	__u32 key = 0;
	struct resource_stats *stats = bpf_map_lookup_elem(&resource_counts, &key);
	if (stats && stats->mr_count > 0) {
		__sync_fetch_and_sub(&stats->mr_count, 1);
	}
	
	/* Update per-cgroup statistics */
	struct cgroup_stats *cg_stats = bpf_map_lookup_elem(&cgroup_stats, &cgroup_id);
	if (!cg_stats) {
		struct cgroup_stats new_cg_stats = {};
		bpf_map_update_elem(&cgroup_stats, &cgroup_id, &new_cg_stats, BPF_ANY);
		cg_stats = bpf_map_lookup_elem(&cgroup_stats, &cgroup_id);
	}
	
	if (cg_stats) {
		cg_stats->counts[RDMA_MONITOR_MR_DEREG]++;
	}

	return 0;
}
SEC("kprobe/rdma_get_gid_attr")
int BPF_KPROBE(rdma_get_gid_attr)
{

	struct event *e;
	pid_t pid;
	u64 ts;
	u64 cgroup_id;

	pid = bpf_get_current_pid_tgid() >> 32;
	ts = bpf_ktime_get_ns();
	cgroup_id = bpf_get_current_cgroup_id();
    
	/* reserve sample from BPF ringbuf */
	e = bpf_ringbuf_reserve(&rb, sizeof(*e), 0);
	if (!e)
		return 0;

	e->pid = pid;
	e->type = RDMA_MONITOR_GID_QUERY;
	e->cgroup_id = cgroup_id;
	bpf_get_current_comm(&e->comm, sizeof(e->comm));
    
	/* successfully submit it to user-space for post-processing */
	bpf_ringbuf_submit(e, 0);
	
	/* Update per-cgroup statistics */
	struct cgroup_stats *cg_stats = bpf_map_lookup_elem(&cgroup_stats, &cgroup_id);
	if (!cg_stats) {
		struct cgroup_stats new_cg_stats = {};
		bpf_map_update_elem(&cgroup_stats, &cgroup_id, &new_cg_stats, BPF_ANY);
		cg_stats = bpf_map_lookup_elem(&cgroup_stats, &cgroup_id);
	}
	
	if (cg_stats) {
		cg_stats->counts[RDMA_MONITOR_GID_QUERY]++;
	}

	return 0;
}
SEC("kprobe/rdma_read_gid_attr_ndev_rcu")
int BPF_KPROBE(rdma_read_gid_attr_ndev_rcu)
{

	struct event *e;
	pid_t pid;
	u64 ts;
	u64 cgroup_id;

	pid = bpf_get_current_pid_tgid() >> 32;
	ts = bpf_ktime_get_ns();
	cgroup_id = bpf_get_current_cgroup_id();
    
	/* reserve sample from BPF ringbuf */
	e = bpf_ringbuf_reserve(&rb, sizeof(*e), 0);
	if (!e)
		return 0;

	e->pid = pid;
	e->type = RDMA_MONITOR_GID_QUERY;
	e->cgroup_id = cgroup_id;
	bpf_get_current_comm(&e->comm, sizeof(e->comm));
    
	/* successfully submit it to user-space for post-processing */
	bpf_ringbuf_submit(e, 0);
	
	/* Update per-cgroup statistics */
	struct cgroup_stats *cg_stats = bpf_map_lookup_elem(&cgroup_stats, &cgroup_id);
	if (!cg_stats) {
		struct cgroup_stats new_cg_stats = {};
		bpf_map_update_elem(&cgroup_stats, &cgroup_id, &new_cg_stats, BPF_ANY);
		cg_stats = bpf_map_lookup_elem(&cgroup_stats, &cgroup_id);
	}
	
	if (cg_stats) {
		cg_stats->counts[RDMA_MONITOR_GID_QUERY]++;
	}

	return 0;
}

struct cm_send_req_args
{
    // size:2; signed:0;
    __u16 common_type;
    // size:1; signed:0;
    __u8 common_flags;
    // size:1; signed:0;
    __u8 common_preempt_count;
    // size:4; signed:1;
    __s32 common_pid;

    // size:4; signed:1;
    __u32 cm_id;;
    // size:8; signed:0;
    __u32 tos;;
    // size:8; signed:0;
    __u32 qp_num;
    // size:28; signed:0;
	__u8 srcaddr[28];
    // size:28; signed:0;
	__u8 dstaddr[28];

};

SEC("tracepoint/rdma_cma/cm_send_req")
int tracepoint__rdma_cma__cm_send_req(struct cm_send_req_args *ctx)
{

	struct event *e;
	pid_t pid;
	u64 ts;
	u64 cgroup_id;

	pid = bpf_get_current_pid_tgid() >> 32;
	ts = bpf_ktime_get_ns();
	cgroup_id = bpf_get_current_cgroup_id();
    
	/* reserve sample from BPF ringbuf */
	e = bpf_ringbuf_reserve(&rb, sizeof(*e), 0);
	if (!e)
		return 0;

	e->pid = pid;
	e->type = RDMA_MONITOR_CM_SEND_REQ;
	e->cgroup_id = cgroup_id;
	bpf_probe_read(&e->cm.cm_id, sizeof(e->cm.cm_id), &ctx->cm_id);
	bpf_probe_read(&e->cm.qpn, sizeof(e->cm.qpn), &ctx->qp_num);
	bpf_probe_read_str(&e->cm.srcaddr, 28, &ctx->srcaddr);
	bpf_probe_read_str(&e->cm.dstaddr, 28, &ctx->dstaddr);
	// bpf_printk("fmt: src: %c %c %c", ctx->dstaddr[1],ctx->dstaddr[2],ctx->dstaddr[3]);
	bpf_get_current_comm(&e->comm, sizeof(e->comm));
    
	/* successfully submit it to user-space for post-processing */
	bpf_ringbuf_submit(e, 0);
	
	/* Update per-cgroup statistics */
	struct cgroup_stats *cg_stats = bpf_map_lookup_elem(&cgroup_stats, &cgroup_id);
	if (!cg_stats) {
		struct cgroup_stats new_cg_stats = {};
		bpf_map_update_elem(&cgroup_stats, &cgroup_id, &new_cg_stats, BPF_ANY);
		cg_stats = bpf_map_lookup_elem(&cgroup_stats, &cgroup_id);
	}
	
	if (cg_stats) {
		cg_stats->counts[RDMA_MONITOR_CM_SEND_REQ]++;
	}

	return 0;
}