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
static bool should_intercept_resource(enum rdma_resource_type resource_type, unsigned long resource_count);
static bool should_intercept_frequency(enum rdma_monitor_type verb_type, unsigned long frequency);
static void print_interception_config();
static void print_resource_counts(struct resource_stats *stats);
static void print_cgroup_stats(int cgroup_map_fd, int resource_map_fd);
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
		for (int i = 0; i < RDMA_RESOURCE_MAX; i++) {
			intercept_config.max_resource_count[i] = 0;  // Disable by default
		}
		for (int i = 0; i < RDMA_MONITOR_TYPE_MAX; i++) {
			intercept_config.max_frequency[i] = 0;  // Disable by default
		}
		return -1;
	}

	char line[256];
	while (fgets(line, sizeof(line), file)) {
		// Skip comments and empty lines
		if (line[0] == '#' || line[0] == '\n') {
			continue;
		}

		char name[32];
		long value;
		int matched = sscanf(line, "%31s %ld", name, &value);
		
		if (matched != 2) {
			fprintf(stderr, "Warning: Invalid config line: %s", line);
			continue;
		}

		// Check if it's a resource type
		bool found = false;
		for (int i = 0; i < RDMA_RESOURCE_MAX; i++) {
			if (strcmp(name, rdma_resource_names[i]) == 0) {
				intercept_config.max_resource_count[i] = value;
				found = true;
				break;
			}
		}
		
		// If not a resource type, check if it's a verb type
		if (!found) {
			for (int i = 0; i < RDMA_MONITOR_TYPE_MAX; i++) {
				if (strcmp(name, rdma_verb_names[i]) == 0) {
					intercept_config.max_frequency[i] = value;
					found = true;
					break;
				}
			}
		}

		if (!found) {
			fprintf(stderr, "Warning: Unknown name in config: %s\n", name);
		}
	}

	fclose(file);
	return 0;
}

// Function to check if resource count based interception is needed
static bool should_intercept_resource(enum rdma_resource_type resource_type, unsigned long resource_count) {
	if (intercept_config.max_resource_count[resource_type] > 0 && 
	    resource_count > intercept_config.max_resource_count[resource_type]) {
		return true;
	}
	return false;
}

// Function to check if frequency based interception is needed
static bool should_intercept_frequency(enum rdma_monitor_type verb_type, unsigned long frequency) {
	if (intercept_config.max_frequency[verb_type] > 0 && 
	    frequency > intercept_config.max_frequency[verb_type]) {
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
	
	// Print resource count based interception config
	printf("Resource Count Based Interception:\n");
	for (int i = 0; i < RDMA_RESOURCE_MAX; i++) {
		if (intercept_config.max_resource_count[i] > 0) {
			printf("  %-15s: >%llu\n", rdma_resource_names[i], 
			       (unsigned long long)intercept_config.max_resource_count[i]);
		}
	}
	
	// Print frequency based interception config
	printf("Frequency Based Interception:\n");
	for (int i = 0; i < RDMA_MONITOR_TYPE_MAX; i++) {
		if (intercept_config.max_frequency[i] > 0) {
			printf("  %-15s: >%llu/s\n", rdma_verb_names[i], 
			       (unsigned long long)intercept_config.max_frequency[i]);
		}
	}
	
	// Print disabled interceptions
	printf("Disabled Interceptions:\n");
	bool has_disabled = false;
	for (int i = 0; i < RDMA_RESOURCE_MAX; i++) {
		if (intercept_config.max_resource_count[i] == 0) {
			printf("  %-15s: DISABLED\n", rdma_resource_names[i]);
			has_disabled = true;
		}
	}
	for (int i = 0; i < RDMA_MONITOR_TYPE_MAX; i++) {
		if (intercept_config.max_frequency[i] == 0) {
			printf("  %-15s: DISABLED\n", rdma_verb_names[i]);
			has_disabled = true;
		}
	}
	if (!has_disabled) {
		printf("  (None)\n");
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
		
		// Check for resource-based interceptions
		bool qp_intercept = should_intercept_resource(RDMA_RESOURCE_QP, stats->qp_count);
		bool pd_intercept = should_intercept_resource(RDMA_RESOURCE_PD, stats->pd_count);
		bool cq_intercept = should_intercept_resource(RDMA_RESOURCE_CQ, stats->cq_count);
		bool mr_intercept = should_intercept_resource(RDMA_RESOURCE_MR, stats->mr_count);
		
		printf("QP (Queue Pairs):     %llu %s\n", stats->qp_count, qp_intercept ? "(*** INTERCEPTED ***)" : "");
		printf("PD (Protection Domains): %llu %s\n", stats->pd_count, pd_intercept ? "(*** INTERCEPTED ***)" : "");
		printf("CQ (Completion Queues): %llu %s\n", stats->cq_count, cq_intercept ? "(*** INTERCEPTED ***)" : "");
		printf("MR (Memory Regions):  %llu %s\n", stats->mr_count, mr_intercept ? "(*** INTERCEPTED ***)" : "");
		print_separator();
		printf("\n");
		
		// Update previous stats
		prev_stats = *stats;
	}
}

// Function to print per-cgroup statistics
static void print_cgroup_stats(int cgroup_map_fd, int resource_map_fd) {
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
						bool intercept = false;
						
						// For resource creation/destruction verbs, check resource count based interception
						switch (i) {
						case RDMA_MONITOR_QP_CREATE:
							{
								__u32 key = 0;
								struct resource_stats global_stats;
								if (bpf_map_lookup_elem(resource_map_fd, &key, &global_stats) == 0) {
									intercept = should_intercept_resource(RDMA_RESOURCE_QP, global_stats.qp_count);
								}
							}
							break;
						case RDMA_MONITOR_PD_ALLOC:
							{
								__u32 key = 0;
								struct resource_stats global_stats;
								if (bpf_map_lookup_elem(resource_map_fd, &key, &global_stats) == 0) {
									intercept = should_intercept_resource(RDMA_RESOURCE_PD, global_stats.pd_count);
								}
							}
							break;
						case RDMA_MONITOR_CQ_CREATE:
							{
								__u32 key = 0;
								struct resource_stats global_stats;
								if (bpf_map_lookup_elem(resource_map_fd, &key, &global_stats) == 0) {
									intercept = should_intercept_resource(RDMA_RESOURCE_CQ, global_stats.cq_count);
								}
							}
							break;
						case RDMA_MONITOR_MR_REG:
							{
								__u32 key = 0;
								struct resource_stats global_stats;
								if (bpf_map_lookup_elem(resource_map_fd, &key, &global_stats) == 0) {
									intercept = should_intercept_resource(RDMA_RESOURCE_MR, global_stats.mr_count);
								}
							}
							break;
						default:
							// For other verbs, check frequency based interception
							intercept = should_intercept_frequency(i, frequency);
							break;
						}
						
						if (intercept) {
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
	unsigned long qp_create_freq = (current_stats->qp_count > prev_stats_copy->qp_count) ? 
		(current_stats->qp_count - prev_stats_copy->qp_count) / output_interval : 0;
	unsigned long pd_alloc_freq = (current_stats->pd_count > prev_stats_copy->pd_count) ? 
		(current_stats->pd_count - prev_stats_copy->pd_count) / output_interval : 0;
	unsigned long cq_create_freq = (current_stats->cq_count > prev_stats_copy->cq_count) ? 
		(current_stats->cq_count - prev_stats_copy->cq_count) / output_interval : 0;
	unsigned long mr_reg_freq = (current_stats->mr_count > prev_stats_copy->mr_count) ? 
		(current_stats->mr_count - prev_stats_copy->mr_count) / output_interval : 0;
		
	// Check for frequency-based interceptions
	bool qp_create_intercept = should_intercept_frequency(RDMA_MONITOR_QP_CREATE, qp_create_freq);
	bool pd_alloc_intercept = should_intercept_frequency(RDMA_MONITOR_PD_ALLOC, pd_alloc_freq);
	bool cq_create_intercept = should_intercept_frequency(RDMA_MONITOR_CQ_CREATE, cq_create_freq);
	bool mr_reg_intercept = should_intercept_frequency(RDMA_MONITOR_MR_REG, mr_reg_freq);
		
	printf("QP Create:  %lu/s %s\n", qp_create_freq, qp_create_intercept ? "(*** INTERCEPTED ***)" : "");
	printf("PD Alloc:   %lu/s %s\n", pd_alloc_freq, pd_alloc_intercept ? "(*** INTERCEPTED ***)" : "");
	printf("CQ Create:  %lu/s %s\n", cq_create_freq, cq_create_intercept ? "(*** INTERCEPTED ***)" : "");
	printf("MR Reg:     %lu/s %s\n", mr_reg_freq, mr_reg_intercept ? "(*** INTERCEPTED ***)" : "");
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
				print_cgroup_stats(cgroup_map_fd, map_fd);
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