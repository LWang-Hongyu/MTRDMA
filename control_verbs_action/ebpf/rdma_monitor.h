/* SPDX-License-Identifier: (LGPL-2.1 OR BSD-2-Clause) */
/* Copyright (c) 2020 Facebook */
#ifndef __RDMA_MONITOR_H
#define __RDMA_MONITOR_H

#define TASK_COMM_LEN 16
#define MAX_FILENAME_LEN 127
#define MAX_CGROUPS 64
#define MAX_VERBS 11
#define MAX_RESOURCES 4

enum rdma_monitor_type {
	RDMA_MONITOR_QP_CREATE,
	RDMA_MONITOR_QP_MODIFY,
	RDMA_MONITOR_QP_DESTORY,
	RDMA_MONITOR_PD_ALLOC,
	RDMA_MONITOR_PD_DEALLOC,
	RDMA_MONITOR_CQ_CREATE,
	RDMA_MONITOR_CQ_DESTORY,
	RDMA_MONITOR_MR_REG,
	RDMA_MONITOR_MR_DEREG,
	RDMA_MONITOR_CM_SEND_REQ,
	RDMA_MONITOR_GID_QUERY,
	RDMA_MONITOR_TYPE_MAX,
};

// Resource types for count-based interception
enum rdma_resource_type {
	RDMA_RESOURCE_QP,
	RDMA_RESOURCE_PD,
	RDMA_RESOURCE_CQ,
	RDMA_RESOURCE_MR,
	RDMA_RESOURCE_MAX,
};

// Verb names for configuration matching
static const char *rdma_verb_names[RDMA_MONITOR_TYPE_MAX] = {
	"QP_CREATE",
	"QP_MODIFY", 
	"QP_DESTROY",
	"PD_ALLOC",
	"PD_DEALLOC",
	"CQ_CREATE",
	"CQ_DESTROY",
	"MR_REG",
	"MR_DEREG",
	"CM_SEND_REQ",
	"GID_QUERY"
};

// Resource names for configuration matching
static const char *rdma_resource_names[RDMA_RESOURCE_MAX] = {
	"QP_COUNT",
	"PD_COUNT",
	"CQ_COUNT",
	"MR_COUNT"
};

static inline char *rdma_monitor_tpye_str(enum rdma_monitor_type type)
{
	switch (type) {
	case RDMA_MONITOR_QP_CREATE:
		return "RDMA_MONITOR_QP_CREATE";
	case RDMA_MONITOR_QP_MODIFY:
		return "RDMA_MONITOR_QP_MODIFY";
	case RDMA_MONITOR_QP_DESTORY:
		return "RDMA_MONITOR_QP_DESTORY";
	case RDMA_MONITOR_PD_ALLOC:
		return "RDMA_MONITOR_PD_ALLOC";
	case RDMA_MONITOR_PD_DEALLOC:
		return "RDMA_MONITOR_PD_DEALLOC";
	case RDMA_MONITOR_CQ_CREATE:
		return "RDMA_MONITOR_CQ_CREATE";
	case RDMA_MONITOR_CQ_DESTORY:
		return "RDMA_MONITOR_CQ_DESTORY";
	case RDMA_MONITOR_MR_REG:
		return "RDMA_MONITOR_MR_REG";
	case RDMA_MONITOR_MR_DEREG:
		return "RDMA_MONITOR_MR_DEREG";
	case RDMA_MONITOR_CM_SEND_REQ:
		return "RDMA_MONITOR_CM_SEND_REQ";
	case RDMA_MONITOR_GID_QUERY:
		return "RDMA_MONITOR_GID_QUERY";
	default:
		return "UNKNOWN EVENT";
	}
}

// Structure for tracking global resource counts
struct resource_stats {
	__u64 qp_count;
	__u64 pd_count;
	__u64 cq_count;
	__u64 mr_count;
};

// Structure for tracking per-cgroup statistics
struct cgroup_stats {
	__u64 counts[RDMA_MONITOR_TYPE_MAX];
};

// Structure for interception thresholds
struct interception_config {
	// Max resource counts for count-based interception (0 to disable)
	__u64 max_resource_count[RDMA_RESOURCE_MAX];
	
	// Max call frequency for frequency-based interception (0 to disable)
	__u64 max_frequency[RDMA_MONITOR_TYPE_MAX];
};

union ibv_gid {
	uint8_t			raw[16];
	struct {
		__be64	subnet_prefix;
		__be64	interface_id;
	} global;
};

struct event {
	int pid;
	enum rdma_monitor_type type;
	char comm[TASK_COMM_LEN];
	__u64 cgroup_id;  // 添加cgroup ID字段
	struct qp {
		__u32 qpn; // 本地QPN
		__u32 dest_qpn; // 目的QPN	
		union ibv_gid gid;
	} qp;
	struct mr {
		__u64 va; // MR VA
		__u64 rkey; // MR REKY
		__u64 len; // MR LEN
	} mr;
	struct cm {
		__u32 cm_id; // CM ID
		__u32 qpn; // 本地QPN
		__u8 srcaddr[28];
		__u8 dstaddr[28];

	} cm;

};


#endif /* __RDMA_MONITOR_H */