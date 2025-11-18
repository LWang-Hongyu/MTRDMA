// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/* Copyright (c) 2020 Facebook */
#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>
#include "rdma_monitor.h"

char LICENSE[] SEC("license") = "Dual BSD/GPL";

// Maps for storing statistics
struct {
	__uint(type, BPF_MAP_TYPE_HASH);
	__uint(max_entries, 10240);
	__type(key, __u64); // cgroup id
	__type(value, struct cgroup_stats);
} cgroup_stats SEC(".maps");

struct {
	__uint(type, BPF_MAP_TYPE_ARRAY);
	__uint(max_entries, 1);
	__type(key, __u32);
	__type(value, struct resource_stats);
} resource_counts SEC(".maps");

struct {
	__uint(type, BPF_MAP_TYPE_RINGBUF);
	__uint(max_entries, 256 * 1024);
} rb SEC(".maps");

// Interception configuration map
struct {
	__uint(type, BPF_MAP_TYPE_ARRAY);
	__uint(max_entries, 1);
	__type(key, __u32);
	__type(value, struct interception_config);
} intercept_config_map SEC(".maps");

// Helper function to get cgroup ID
static __u64 get_cgroup_id() {
	struct task_struct *task = (struct task_struct *)bpf_get_current_task();
	struct cgroup *cgrp = BPF_CORE_READ(task, cgroups, subsys[0], cgroup);
	return BPF_CORE_READ(cgrp, kn, id);
}


SEC("kprobe/mlx5_ib_create_qp")
int BPF_KPROBE(ib_create_qp)
{
	__u64 cgroup_id = get_cgroup_id();
	struct cgroup_stats *cgroup_stats_entry;
	struct cgroup_stats new_cgroup_stats = {};

	// Update global resource count
	__u32 key = 0;
	struct resource_stats *stats = bpf_map_lookup_elem(&resource_counts, &key);
	if (stats) {
		__sync_fetch_and_add(&stats->qp_count, 1);
	}

	// Update per-cgroup statistics
	cgroup_stats_entry = bpf_map_lookup_elem(&cgroup_stats, &cgroup_id);
	if (cgroup_stats_entry) {
		cgroup_stats_entry->counts[RDMA_MONITOR_QP_CREATE]++;
		
		// For frequency-based interception check
		// We would need to implement time-based checks in user space for accurate frequency calculation
	} else {
		new_cgroup_stats.counts[RDMA_MONITOR_QP_CREATE] = 1;
		bpf_map_update_elem(&cgroup_stats, &cgroup_id, &new_cgroup_stats, BPF_ANY);
	}

	return 0;
}

SEC("kprobe/mlx5_ib_modify_qp")
int BPF_KPROBE(ib_modify_qp, struct ib_qp *qp, struct ib_qp_attr *attr, int attr_mask)
{
	__u64 cgroup_id = get_cgroup_id();
	struct cgroup_stats *cgroup_stats_entry;
	struct cgroup_stats new_cgroup_stats = {};

	// Update per-cgroup statistics
	cgroup_stats_entry = bpf_map_lookup_elem(&cgroup_stats, &cgroup_id);
	if (cgroup_stats_entry) {
		cgroup_stats_entry->counts[RDMA_MONITOR_QP_MODIFY]++;
	} else {
		new_cgroup_stats.counts[RDMA_MONITOR_QP_MODIFY] = 1;
		bpf_map_update_elem(&cgroup_stats, &cgroup_id, &new_cgroup_stats, BPF_ANY);
	}

	return 0;
}

SEC("kprobe/mlx5_ib_destroy_qp")
int BPF_KPROBE(ib_destroy_qp)
{
	__u32 key = 0;
	struct resource_stats *stats;
	__u64 cgroup_id = get_cgroup_id();
	struct cgroup_stats *cgroup_stats_entry;
	struct cgroup_stats new_cgroup_stats = {};

	// Update global resource count
	stats = bpf_map_lookup_elem(&resource_counts, &key);
	if (stats) {
		if (stats->qp_count > 0) {
			__sync_fetch_and_sub(&stats->qp_count, 1);
		}
	}

	// Update per-cgroup statistics
	cgroup_stats_entry = bpf_map_lookup_elem(&cgroup_stats, &cgroup_id);
	if (cgroup_stats_entry) {
		cgroup_stats_entry->counts[RDMA_MONITOR_QP_DESTORY]++;
	} else {
		new_cgroup_stats.counts[RDMA_MONITOR_QP_DESTORY] = 1;
		bpf_map_update_elem(&cgroup_stats, &cgroup_id, &new_cgroup_stats, BPF_ANY);
	}

	return 0;
}

SEC("kprobe/mlx5_ib_alloc_pd")
int BPF_KPROBE(ib_alloc_pd)
{
	__u32 key = 0;
	struct resource_stats *stats;
	__u64 cgroup_id = get_cgroup_id();
	struct cgroup_stats *cgroup_stats_entry;
	struct cgroup_stats new_cgroup_stats = {};

	// Update global resource count
	stats = bpf_map_lookup_elem(&resource_counts, &key);
	if (stats) {
		__sync_fetch_and_add(&stats->pd_count, 1);
	}

	// Update per-cgroup statistics
	cgroup_stats_entry = bpf_map_lookup_elem(&cgroup_stats, &cgroup_id);
	if (cgroup_stats_entry) {
		cgroup_stats_entry->counts[RDMA_MONITOR_PD_ALLOC]++;
		
		// For frequency-based interception check
		// We would need to implement time-based checks in user space for accurate frequency calculation
	} else {
		new_cgroup_stats.counts[RDMA_MONITOR_PD_ALLOC] = 1;
		bpf_map_update_elem(&cgroup_stats, &cgroup_id, &new_cgroup_stats, BPF_ANY);
	}

	return 0;
}

SEC("kprobe/mlx5_ib_dealloc_pd")
int BPF_KPROBE(ib_dealloc_pd)
{
	__u32 key = 0;
	struct resource_stats *stats;
	__u64 cgroup_id = get_cgroup_id();
	struct cgroup_stats *cgroup_stats_entry;
	struct cgroup_stats new_cgroup_stats = {};

	// Update global resource count
	stats = bpf_map_lookup_elem(&resource_counts, &key);
	if (stats) {
		if (stats->pd_count > 0) {
			__sync_fetch_and_sub(&stats->pd_count, 1);
		}
	}

	// Update per-cgroup statistics
	cgroup_stats_entry = bpf_map_lookup_elem(&cgroup_stats, &cgroup_id);
	if (cgroup_stats_entry) {
		cgroup_stats_entry->counts[RDMA_MONITOR_PD_DEALLOC]++;
	} else {
		new_cgroup_stats.counts[RDMA_MONITOR_PD_DEALLOC] = 1;
		bpf_map_update_elem(&cgroup_stats, &cgroup_id, &new_cgroup_stats, BPF_ANY);
	}

	return 0;
}

SEC("kprobe/mlx5_ib_create_cq")
int BPF_KPROBE(ib_create_cq)
{
	__u32 key = 0;
	struct resource_stats *stats;
	__u64 cgroup_id = get_cgroup_id();
	struct cgroup_stats *cgroup_stats_entry;
	struct cgroup_stats new_cgroup_stats = {};

	// Update global resource count
	stats = bpf_map_lookup_elem(&resource_counts, &key);
	if (stats) {
		__sync_fetch_and_add(&stats->cq_count, 1);
	}

	// Update per-cgroup statistics
	cgroup_stats_entry = bpf_map_lookup_elem(&cgroup_stats, &cgroup_id);
	if (cgroup_stats_entry) {
		cgroup_stats_entry->counts[RDMA_MONITOR_CQ_CREATE]++;
		
		// For frequency-based interception check
		// We would need to implement time-based checks in user space for accurate frequency calculation
	} else {
		new_cgroup_stats.counts[RDMA_MONITOR_CQ_CREATE] = 1;
		bpf_map_update_elem(&cgroup_stats, &cgroup_id, &new_cgroup_stats, BPF_ANY);
	}

	return 0;
}

SEC("kprobe/mlx5_ib_destroy_cq")
int BPF_KPROBE(ib_destroy_cq)
{
	__u32 key = 0;
	struct resource_stats *stats;
	__u64 cgroup_id = get_cgroup_id();
	struct cgroup_stats *cgroup_stats_entry;
	struct cgroup_stats new_cgroup_stats = {};

	// Update global resource count
	stats = bpf_map_lookup_elem(&resource_counts, &key);
	if (stats) {
		if (stats->cq_count > 0) {
			__sync_fetch_and_sub(&stats->cq_count, 1);
		}
	}

	// Update per-cgroup statistics
	cgroup_stats_entry = bpf_map_lookup_elem(&cgroup_stats, &cgroup_id);
	if (cgroup_stats_entry) {
		cgroup_stats_entry->counts[RDMA_MONITOR_CQ_DESTORY]++;
	} else {
		new_cgroup_stats.counts[RDMA_MONITOR_CQ_DESTORY] = 1;
		bpf_map_update_elem(&cgroup_stats, &cgroup_id, &new_cgroup_stats, BPF_ANY);
	}

	return 0;
}

SEC("kprobe/mlx5_ib_reg_user_mr")
int BPF_KPROBE(ib_reg_user_mr, struct ib_pd *pd, u64 start, u64 length, u64 virt_addr, int access_flags)
{
	__u32 key = 0;
	struct resource_stats *stats;
	__u64 cgroup_id = get_cgroup_id();
	struct cgroup_stats *cgroup_stats_entry;
	struct cgroup_stats new_cgroup_stats = {};

	// Update global resource count
	stats = bpf_map_lookup_elem(&resource_counts, &key);
	if (stats) {
		__sync_fetch_and_add(&stats->mr_count, 1);
	}

	// Update per-cgroup statistics
	cgroup_stats_entry = bpf_map_lookup_elem(&cgroup_stats, &cgroup_id);
	if (cgroup_stats_entry) {
		cgroup_stats_entry->counts[RDMA_MONITOR_MR_REG]++;
		
		// For frequency-based interception check
		// We would need to implement time-based checks in user space for accurate frequency calculation
	} else {
		new_cgroup_stats.counts[RDMA_MONITOR_MR_REG] = 1;
		bpf_map_update_elem(&cgroup_stats, &cgroup_id, &new_cgroup_stats, BPF_ANY);
	}

	return 0;
}

SEC("kprobe/mlx5_ib_dereg_mr")
int BPF_KPROBE(ib_dereg_mr)
{
	__u32 key = 0;
	struct resource_stats *stats;
	__u64 cgroup_id = get_cgroup_id();
	struct cgroup_stats *cgroup_stats_entry;
	struct cgroup_stats new_cgroup_stats = {};

	// Update global resource count
	stats = bpf_map_lookup_elem(&resource_counts, &key);
	if (stats) {
		if (stats->mr_count > 0) {
			__sync_fetch_and_sub(&stats->mr_count, 1);
		}
	}

	// Update per-cgroup statistics
	cgroup_stats_entry = bpf_map_lookup_elem(&cgroup_stats, &cgroup_id);
	if (cgroup_stats_entry) {
		cgroup_stats_entry->counts[RDMA_MONITOR_MR_DEREG]++;
	} else {
		new_cgroup_stats.counts[RDMA_MONITOR_MR_DEREG] = 1;
		bpf_map_update_elem(&cgroup_stats, &cgroup_id, &new_cgroup_stats, BPF_ANY);
	}

	return 0;
}

SEC("kprobe/rdma_get_gid_attr")
int BPF_KPROBE(ib_gid_query1)
{
	__u64 cgroup_id = get_cgroup_id();
	struct cgroup_stats *cgroup_stats_entry;
	struct cgroup_stats new_cgroup_stats = {};

	// Update per-cgroup statistics
	cgroup_stats_entry = bpf_map_lookup_elem(&cgroup_stats, &cgroup_id);
	if (cgroup_stats_entry) {
		cgroup_stats_entry->counts[RDMA_MONITOR_GID_QUERY]++;
	} else {
		new_cgroup_stats.counts[RDMA_MONITOR_GID_QUERY] = 1;
		bpf_map_update_elem(&cgroup_stats, &cgroup_id, &new_cgroup_stats, BPF_ANY);
	}

	return 0;
}

SEC("kprobe/rdma_read_gid_attr_ndev_rcu")
int BPF_KPROBE(ib_gid_query2)
{
	__u64 cgroup_id = get_cgroup_id();
	struct cgroup_stats *cgroup_stats_entry;
	struct cgroup_stats new_cgroup_stats = {};

	// Update per-cgroup statistics
	cgroup_stats_entry = bpf_map_lookup_elem(&cgroup_stats, &cgroup_id);
	if (cgroup_stats_entry) {
		cgroup_stats_entry->counts[RDMA_MONITOR_GID_QUERY]++;
	} else {
		new_cgroup_stats.counts[RDMA_MONITOR_GID_QUERY] = 1;
		bpf_map_update_elem(&cgroup_stats, &cgroup_id, &new_cgroup_stats, BPF_ANY);
	}

	return 0;
}

SEC("tracepoint/rdma_cma/cm_send_req")
int BPF_PROG(cm_send_req, u64 cm_id, u64 qp, u64 srcaddr, u64 dstaddr)
{
	__u64 cgroup_id = get_cgroup_id();
	struct cgroup_stats *cgroup_stats_entry;
	struct cgroup_stats new_cgroup_stats = {};

	// Update per-cgroup statistics
	cgroup_stats_entry = bpf_map_lookup_elem(&cgroup_stats, &cgroup_id);
	if (cgroup_stats_entry) {
		cgroup_stats_entry->counts[RDMA_MONITOR_CM_SEND_REQ]++;
	} else {
		new_cgroup_stats.counts[RDMA_MONITOR_CM_SEND_REQ] = 1;
		bpf_map_update_elem(&cgroup_stats, &cgroup_id, &new_cgroup_stats, BPF_ANY);
	}

	return 0;
}