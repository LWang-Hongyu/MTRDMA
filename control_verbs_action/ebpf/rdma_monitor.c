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
#include <string.h>
#include <limits.h>
#include "rdma_monitor.h"
#include "rdma_monitor.skel.h"

static volatile bool exiting = false;
static unsigned long output_interval = 1; // 默认输出间隔为1秒
static char config_file[256] = "config.txt"; // 默认配置文件

// Global variable to store previous resource counts for change detection
static struct resource_stats prev_stats = {0, 0, 0, 0};

// Interception configuration
static struct interception_config intercept_config = {0};

// Function declarations
static void print_separator();
static void print_timestamp();
static int parse_config_file(const char *filename);
static bool should_intercept(enum rdma_monitor_type type, unsigned long frequency, unsigned long total_count);
static void print_interception_config();
static void print_resource_counts(struct resource_stats *stats);
static void print_cgroup_stats(int cgroup_map_fd);
static void print_frequency_stats(struct resource_stats *current_stats, struct resource_stats *prev_stats_copy);

// Function to parse command line arguments
static error_t parse_opt(int key, char *arg, struct argp_state *state) {
	switch (key) {
	case 'i':
		output_interval = strtoul(arg, NULL, 10);
		if (output_interval == 0)
			argp_usage(state);
		break;
	case 'c':
		strncpy(config_file, arg, sizeof(config_file) - 1);
		config_file[sizeof(config_file) - 1] = '\0';
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
	{ "config", 'c', "CONFIG_FILE", 0, "Configuration file path (default: config.txt)" },
	{},
};

static struct argp argp = {
	.options = options,
	.parser = parse_opt,
	.doc = "RDMA Control Path Monitor",
};

// Function to print separator line
static void print_separator() {
	printf("==================================================\n");
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

// Function to parse configuration file
static int parse_config_file(const char *filename) {
	FILE *file = fopen(filename, "r");
	if (!file) {
		fprintf(stderr, "Warning: Could not open config file %s, using default thresholds\n", filename);
		// Set default thresholds
		for (int i = 0; i < RDMA_MONITOR_TYPE_MAX; i++) {
			intercept_config.max_frequency[i] = 0;  // Disable by default
			intercept_config.max_total_count[i] = 0; // Disable by default
		}
		return -1;
	}

	char line[256];
	while (fgets(line, sizeof(line), file)) {
		// Skip comments and empty lines
		if (line[0] == '#' || line[0] == '\n') {
			continue;
		}

		char verb_name[32];
		long max_freq, max_count;
		int matched = sscanf(line, "%31s %ld %ld", verb_name, &max_freq, &max_count);
		
		if (matched != 3) {
			fprintf(stderr, "Warning: Invalid config line: %s", line);
			continue;
		}

		// Find matching verb type
		enum rdma_monitor_type verb_type = RDMA_MONITOR_TYPE_MAX;
		for (int i = 0; i < MAX_VERBS; i++) {
			if (strcmp(verb_name, rdma_verb_names[i]) == 0) {
				verb_type = i;
				break;
			}
		}

		if (verb_type != RDMA_MONITOR_TYPE_MAX) {
			intercept_config.max_frequency[verb_type] = max_freq;
			intercept_config.max_total_count[verb_type] = max_count;
		} else {
			fprintf(stderr, "Warning: Unknown verb name in config: %s\n", verb_name);
		}
	}

	fclose(file);
	return 0;
}

// Function to check if interception is needed
static bool should_intercept(enum rdma_monitor_type type, unsigned long frequency, unsigned long total_count) {
	// Check frequency threshold
	if (intercept_config.max_frequency[type] > 0 && frequency > intercept_config.max_frequency[type]) {
		return true;
	}
	
	// Check total count threshold
	if (intercept_config.max_total_count[type] > 0 && total_count > intercept_config.max_total_count[type]) {
		return true;
	}
	
	return false;
}

static void sig_handler(int sig)
{
	exiting = true;
}

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

// Function to print interception configuration
static void print_interception_config() {
	print_timestamp();
	printf("Interception Configuration:\n");
	print_separator();
	
	for (int i = 0; i < RDMA_MONITOR_TYPE_MAX; i++) {
		if (intercept_config.max_frequency[i] == 0 && intercept_config.max_total_count[i] == 0) {
			continue; // Skip if both are disabled
		}
		
		printf("%-25s: Frequency=%s%llu/s%s, Total=%s%llu%s\n",
		       rdma_monitor_tpye_str(i),
		       (intercept_config.max_frequency[i] == 0) ? "" : ">",
		       (unsigned long long)intercept_config.max_frequency[i],
		       (intercept_config.max_frequency[i] == 0) ? "DISABLED" : "",
		       (intercept_config.max_total_count[i] == 0) ? "" : ">",
		       (unsigned long long)intercept_config.max_total_count[i],
		       (intercept_config.max_total_count[i] == 0) ? "DISABLED" : "");
	}
	print_separator();
	printf("\n");
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
						// Check if interception is needed
						unsigned long frequency = stats.counts[i] / output_interval;
						if (should_intercept(i, frequency, stats.counts[i])) {
							printf("  %-25s: %llu (*** INTERCEPTED ***)\n", rdma_monitor_tpye_str(i), stats.counts[i]);
						} else {
							printf("  %-25s: %llu\n", rdma_monitor_tpye_str(i), stats.counts[i]);
						}
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
		
	// Check for interceptions
	bool qp_intercept = should_intercept(RDMA_MONITOR_QP_CREATE, qp_freq, current_stats->qp_count) ||
	                    should_intercept(RDMA_MONITOR_QP_MODIFY, qp_freq, current_stats->qp_count) ||
	                    should_intercept(RDMA_MONITOR_QP_DESTORY, qp_freq, current_stats->qp_count);
	bool pd_intercept = should_intercept(RDMA_MONITOR_PD_ALLOC, pd_freq, current_stats->pd_count) ||
	                    should_intercept(RDMA_MONITOR_PD_DEALLOC, pd_freq, current_stats->pd_count);
	bool cq_intercept = should_intercept(RDMA_MONITOR_CQ_CREATE, cq_freq, current_stats->cq_count) ||
	                    should_intercept(RDMA_MONITOR_CQ_DESTORY, cq_freq, current_stats->cq_count);
	bool mr_intercept = should_intercept(RDMA_MONITOR_MR_REG, mr_freq, current_stats->mr_count) ||
	                    should_intercept(RDMA_MONITOR_MR_DEREG, mr_freq, current_stats->mr_count);
		
	printf("QP Operations:  %lu/s %s\n", qp_freq, qp_intercept ? "(*** INTERCEPTED ***)" : "");
	printf("PD Operations:  %lu/s %s\n", pd_freq, pd_intercept ? "(*** INTERCEPTED ***)" : "");
	printf("CQ Operations:  %lu/s %s\n", cq_freq, cq_intercept ? "(*** INTERCEPTED ***)" : "");
	printf("MR Operations:  %lu/s %s\n", mr_freq, mr_intercept ? "(*** INTERCEPTED ***)" : "");
	print_separator();
	printf("\n");
}

// Function to update interception configuration in eBPF map
static int update_ebpf_intercept_config(struct rdma_monitor_bpf *skel) {
	int config_map_fd = bpf_map__fd(skel->maps.intercept_config_map);
	__u32 key = 0;
	
	return bpf_map_update_elem(config_map_fd, &key, &intercept_config, BPF_ANY);
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

	/* Parse configuration file */
	parse_config_file(config_file);

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

	/* Update interception configuration in eBPF map */
	err = update_ebpf_intercept_config(skel);
	if (err)
	{
		fprintf(stderr, "Failed to update interception configuration in eBPF map: %d\n", err);
		// Continue anyway, as interception is not critical for monitoring
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
	
	// Print interception configuration
	print_interception_config();
	
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