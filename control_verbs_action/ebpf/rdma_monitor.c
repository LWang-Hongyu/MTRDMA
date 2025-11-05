// SPDX-License-Identifier: (LGPL-2.1 OR BSD-2-Clause)
/* Copyright (c) 2020 Facebook */
#include <argp.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <sys/resource.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>
#include <unistd.h>
#include <stdlib.h>
#include "rdma_monitor.h"
#include "rdma_monitor.skel.h"

// Global variables
static volatile bool exiting = false;
static unsigned long output_interval = 1; // 默认输出间隔为1秒

// Global variable to store previous resource counts for change detection
static struct resource_stats prev_stats = {0, 0, 0, 0};

static void sig_handler(int sig)
{
	exiting = true;
}

// Function to parse command line arguments
static error_t parse_opt(int key, char *arg, struct argp_state *state) {
	switch (key) {
	case 'i':
		output_interval = strtoul(arg, NULL, 10);
		if (output_interval == 0)
			argp_usage(state);
		break;
	case ARGP_KEY_ARG:
		argp_usage(state);
		break;
	case ARGP_KEY_END:
		break;
	default:
		return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

static struct argp_option options[] = {
	{ "interval", 'i', "SECONDS", 0, "Output interval in seconds (default: 1)" },
	{},
};

static struct argp argp = {
	.options = options,
	.parser = parse_opt,
	.doc = "RDMA Control Path Monitor",
};

static int handle_event(void *ctx, void *data, size_t data_sz)
{
	// const struct event *e = data; // 注释掉未使用的变量
	// char gid[33]; // Commented out as it's not used

	/* 注释掉详细的事件输出 */
	/*
	printf("%-16s %-8d %-32s", e->comm, e->pid, rdma_monitor_tpye_str(e->type));
	switch (e->type)
	{
	case RDMA_MONITOR_QP_CREATE:
		//printf("QPN: %-16d DQPN: %-16d FUNc: 1", e->qp.qpn, e->qp.dest_qpn);
		break;
	case RDMA_MONITOR_QP_MODIFY:
		// gid_to_wire_gid(&e->qp.gid, gid);
		printf("QPN: %-16d DQPN: %-16d FUNc: 1\n", e->qp.qpn, e->qp.dest_qpn);
		break;
	case RDMA_MONITOR_MR_REG:
		// printf("VA: %-16llx LEN: %-16llu", e->mr.va, e->mr.len);
		break;
	case RDMA_MONITOR_CM_SEND_REQ:
		// printf("CM ID: %-16d QPN: %-16d src: %s dst: %s", e->cm.cm_id, e->cm.qpn, e->cm.srcaddr, e->cm.dstaddr);
		break;
	case RDMA_MONITOR_GID_QUERY:
		
	// printf("CM ID: %-16d QPN: %-16d src: %s dst: %s", e->cm.cm_id, e->cm.qpn, e->cm.srcaddr, e->cm.dstaddr);
		break;
	default:
		break;
	}
	*/

	// printf("\n");

	return 0;
}

// Function to print current timestamp
static void print_timestamp() {
	time_t now;
	struct tm *tm_info;
	char timestamp[26];
	
	time(&now);
	tm_info = localtime(&now);
	strftime(timestamp, 26, "%Y-%m-%d %H:%M:%S", tm_info);
	printf("[%s] ", timestamp);
}

// Function to print separator line
static void print_separator() {
	printf("==================================================\n");
}

// Function to print current resource counts
static void print_resource_counts(struct resource_stats *stats) {
	// Only print if there's a change or this is the first time
	if (stats->qp_count != prev_stats.qp_count || 
	    stats->pd_count != prev_stats.pd_count ||
	    stats->cq_count != prev_stats.cq_count ||
	    stats->mr_count != prev_stats.mr_count) {
	    
		print_separator();
		print_timestamp();
		printf("RDMA Resource Counts:\n");
		print_separator();
		printf("QP (Queue Pairs):     %llu\n", stats->qp_count);
		printf("PD (Protection Domains): %llu\n", stats->pd_count);
		printf("CQ (Completion Queues): %llu\n", stats->cq_count);
		printf("MR (Memory Regions):  %llu\n", stats->mr_count);
		print_separator();
		printf("\n");
		
		// Update previous stats
		prev_stats = *stats;
	}
}

// Function to print per-cgroup statistics
static void print_cgroup_stats(int cgroup_map_fd) {
	__u64 lookup_key = 0;
	__u64 next_key;
	struct cgroup_stats stats;
	bool has_data = false;
	
	// 遍历所有cgroup统计信息
	while (bpf_map_get_next_key(cgroup_map_fd, &lookup_key, &next_key) == 0) {
		if (bpf_map_lookup_elem(cgroup_map_fd, &next_key, &stats) == 0) {
			// 检查是否有任何非零计数
			bool has_counts = false;
			for (int i = 0; i < RDMA_MONITOR_TYPE_MAX; i++) {
				if (stats.counts[i] > 0) {
					has_counts = true;
					break;
				}
			}
			
			if (has_counts) {
				if (!has_data) {
					print_timestamp();
					printf("Per-Cgroup RDMA Statistics:\n");
					print_separator();
					has_data = true;
				}
				
				printf("CGROUP ID: %llu\n", next_key);
				for (int i = 0; i < RDMA_MONITOR_TYPE_MAX; i++) {
					if (stats.counts[i] > 0) {
						printf("  %-25s: %llu\n", rdma_monitor_tpye_str(i), stats.counts[i]);
					}
				}
				printf("\n");
			}
		}
		lookup_key = next_key;
	}
	
	if (has_data) {
		print_separator();
		printf("\n");
	}
}

// Function to print frequency statistics
static void print_frequency_stats(struct resource_stats *current_stats, struct resource_stats *prev_stats_copy) {
	print_timestamp();
	printf("RDMA Operation Frequency (calls per second):\n");
	print_separator();
	
	// Calculate and display frequency for each resource type
	unsigned long qp_freq = (current_stats->qp_count > prev_stats_copy->qp_count) ? 
		(current_stats->qp_count - prev_stats_copy->qp_count) / output_interval : 0;
	unsigned long pd_freq = (current_stats->pd_count > prev_stats_copy->pd_count) ? 
		(current_stats->pd_count - prev_stats_copy->pd_count) / output_interval : 0;
	unsigned long cq_freq = (current_stats->cq_count > prev_stats_copy->cq_count) ? 
		(current_stats->cq_count - prev_stats_copy->cq_count) / output_interval : 0;
	unsigned long mr_freq = (current_stats->mr_count > prev_stats_copy->mr_count) ? 
		(current_stats->mr_count - prev_stats_copy->mr_count) / output_interval : 0;
		
	printf("QP Operations:  %lu/s\n", qp_freq);
	printf("PD Operations:  %lu/s\n", pd_freq);
	printf("CQ Operations:  %lu/s\n", cq_freq);
	printf("MR Operations:  %lu/s\n", mr_freq);
	print_separator();
	printf("\n");
}

int main(int argc, char **argv)
{
	struct ring_buffer *rb = NULL;
	struct rdma_monitor_bpf *skel;
	struct resource_stats stats, prev_stats_copy;
	int err;
	int map_fd, cgroup_map_fd;
	time_t last_output_time = 0;

	/* Parse command line arguments */
	err = argp_parse(&argp, argc, argv, 0, NULL, NULL);
	if (err)
		return err;

	/* Cleaner handling of Ctrl-C */
	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);

	/* Load and verify BPF application */
	skel = rdma_monitor_bpf__open();
	if (!skel)
	{
		fprintf(stderr, "Failed to open and load BPF skeleton\n");
		return 1;
	}

	/* Load & verify BPF programs */
	err = rdma_monitor_bpf__load(skel);
	if (err)
	{
		fprintf(stderr, "Failed to load and verify BPF skeleton\n");
		goto cleanup;
	}

	/* Attach tracepoints */
	err = rdma_monitor_bpf__attach(skel);
	if (err)
	{
		fprintf(stderr, "Failed to attach BPF skeleton\n");
		goto cleanup;
	}

	/* Set up ring buffer polling */
	rb = ring_buffer__new(bpf_map__fd(skel->maps.rb), handle_event, NULL, NULL);
	if (!rb)
	{
		err = -1;
		fprintf(stderr, "Failed to create ring buffer\n");
		goto cleanup;
	}

	/* Get map file descriptor for resource counts */
	map_fd = bpf_map__fd(skel->maps.resource_counts);
	cgroup_map_fd = bpf_map__fd(skel->maps.cgroup_stats);

	/* Process events */
	printf("RDMA Control Path Monitor Started (interval: %lu seconds)\n", output_interval);
	print_separator();
	printf("\n");
	
	// Initialize previous stats copy
	prev_stats_copy = (struct resource_stats){0, 0, 0, 0};
	
	while (!exiting)
	{
		err = ring_buffer__poll(rb, 100 /* timeout, ms */);
		/* Ctrl-C will cause -EINTR */
		if (err == -EINTR)
		{
			err = 0;
			break;
		}
		if (err < 0)
		{
			printf("Error polling perf buffer: %d\n", err);
			break;
		}
		
		/* Check if it's time to output statistics */
		time_t current_time = time(NULL);
		if (current_time - last_output_time >= (time_t)output_interval) {
			/* Check and display current resource counts */
			__u32 key = 0;
			if (bpf_map_lookup_elem(map_fd, &key, &stats) == 0) {
				print_resource_counts(&stats);
				print_cgroup_stats(cgroup_map_fd);
				print_frequency_stats(&stats, &prev_stats_copy);
				
				// Update previous stats copy for next frequency calculation
				prev_stats_copy = stats;
			}
			
			last_output_time = current_time;
		}
	}
	
	print_separator();
	print_timestamp();
	printf("RDMA Control Path Monitor Stopped\n");
	print_separator();

cleanup:
	/* Clean up */
	ring_buffer__free(rb);
	rdma_monitor_bpf__destroy(skel);

	return err < 0 ? -err : 0;
}