# RDMA Control Path Verbs Monitoring Tool

This tool uses eBPF technology to monitor RDMA control path operations, including creation, modification, and destruction of resources such as QP (Queue Pair), PD (Protection Domain), CQ (Completion Queue), and MR (Memory Region).

## Features

- Real-time monitoring of RDMA control path operations
- Kernel-level monitoring using eBPF with minimal performance overhead
- Global state tracking of RDMA resources (total count of each resource type)
- Per-cgroup tracking of RDMA operations for multi-tenant isolation analysis
- Per-second call rate statistics for each RDMA operation type
- Configurable interception based on call frequency or total count thresholds
- Configurable output interval with timestamped results
- Clear visual separation of output sections for better readability
- Supports monitoring of various RDMA control operations:
  - QP creation/modification/destruction
  - PD allocation/deallocation
  - CQ creation/destruction
  - MR registration/deregistration
  - CM connection requests
  - GID queries

## Directory Structure

```
control_verbs_action/
├── ebpf/
│   ├── rdma_monitor.bpf.c     # eBPF kernel code
│   ├── rdma_monitor.c         # User-space monitoring program
│   ├── rdma_monitor.h         # Header file, defining event structures
│   └── Makefile               # Build script
├── kprobe_ibv_qp.c            # Simple kprobe example
├── loader.c                   # eBPF program loader
├── config.txt                 # Interception configuration file
└── Makefile                   # Main Makefile
```

## How It Works

### eBPF Monitoring Module

The eBPF-based monitoring tool captures RDMA control path operations by setting kprobe probes in the kernel:

1. `mlx5_ib_create_qp` - Monitor QP creation
2. `mlx5_ib_modify_qp` - Monitor QP modification
3. `mlx5_ib_destroy_qp` - Monitor QP destruction
4. `mlx5_ib_alloc_pd` - Monitor PD allocation
5. `mlx5_ib_dealloc_pd` - Monitor PD deallocation
6. `mlx5_ib_create_cq` - Monitor CQ creation
7. `mlx5_ib_destroy_cq` - Monitor CQ destruction
8. `mlx5_ib_reg_user_mr` - Monitor MR registration
9. `mlx5_ib_dereg_mr` - Monitor MR deregistration
10. `rdma_get_gid_attr` and `rdma_read_gid_attr_ndev_rcu` - Monitor GID queries
11. `tracepoint/rdma_cma/cm_send_req` - Monitor connection management requests

### Data Transfer

Uses BPF ring buffer for efficient data transfer between kernel and user space.

### Global State Tracking

The tool maintains global counts of RDMA resources in the kernel using BPF maps:
- Total number of Queue Pairs (QP)
- Total number of Protection Domains (PD)
- Total number of Completion Queues (CQ)
- Total number of Memory Regions (MR)

These counts are atomically updated as resources are created or destroyed, providing real-time visibility into system-wide RDMA resource usage.

### Per-Cgroup Tracking

The tool also tracks RDMA operations per cgroup, allowing for fine-grained analysis of resource usage by different applications or tenants:
- Each RDMA control operation is associated with the cgroup ID of the process that triggered it
- Operations are counted separately for each cgroup
- This enables multi-tenant isolation analysis and resource accounting

### Call Rate Statistics

The tool calculates and displays the per-second call rate for each RDMA operation type:
- QP operations (creation, modification, destruction)
- PD operations (allocation, deallocation)
- CQ operations (creation, destruction)
- MR operations (registration, deregistration)

### Interception Based on Thresholds

The tool supports two types of interception of RDMA operations based on configurable thresholds:

1. **Resource Count Based Interception**:
   - Triggered when total resource count exceeds a specified limit
   - Only intercepts resource creation/registration operations, not destruction/deallocation
   - Examples: When QP count > limit, intercept QP creation but allow QP destruction

2. **Frequency Based Interception**:
   - Triggered when operation call rate exceeds a specified rate (per second)
   - Can intercept any operation type
   - Examples: When MR registration rate > limit, intercept MR registration operations

## Configuration File

The interception behavior is controlled by a configuration file (default: `config.txt`). Each line in the configuration file specifies thresholds for either resource count or operation frequency:

```
RESOURCE_TYPE MAX_COUNT
VERB_TYPE MAX_FREQUENCY
```

- `RESOURCE_TYPE`: The RDMA resource type (e.g., QP_COUNT, MR_COUNT)
- `MAX_COUNT`: Maximum allowed resource count (0 to disable count check)
- `VERB_TYPE`: The RDMA operation type (e.g., QP_CREATE, MR_REG)
- `MAX_FREQUENCY`: Maximum allowed calls per second (0 to disable frequency check)

Interception Logic:
- **Resource Count Based**: When current resource count > MAX_COUNT, intercept CREATE/ALLOC operations but allow DESTROY/DEALLOC
- **Frequency Based**: When verb call rate > MAX_FREQUENCY, intercept the specific verb
- Use 0 to disable either type of check

Example configuration:
```
# Intercept QP creation when total QP count > 1000
QP_COUNT 1000

# Intercept MR registration when rate > 200/s
MR_REG 200

# Intercept QP creation when rate > 100/s
QP_CREATE 100

# No interception for PD count
PD_COUNT 0
```

## Compilation and Execution

### Compilation

```bash
cd control_verbs_action
make
```

### Running the Monitoring Tool

```bash
cd ebpf
sudo ./rdma_monitor
```

### Command Line Options

```bash
sudo ./rdma_monitor -i <seconds> -c <config_file>
```

- `-i, --interval=SECONDS` - Set the output interval in seconds (default: 1)
- `-c, --config=CONFIG_FILE` - Set the configuration file path (default: config.txt)

Examples:
```bash
# Output every 5 seconds with default config
sudo ./rdma_monitor -i 5

# Use custom configuration file
sudo ./rdma_monitor -c /path/to/custom_config.txt

# Use custom configuration file and output interval
sudo ./rdma_monitor -c /path/to/custom_config.txt -i 10
```

## Use Cases

This tool can be used in the following scenarios:

1. **Performance Analysis**: Monitor control path behavior of RDMA applications
2. **Resource Management**: Track usage of RDMA resources (QP, PD, CQ, MR)
3. **Security Auditing**: Detect abnormal RDMA control operations
4. **Debugging Assistance**: Help debug issues with RDMA applications
5. **Multi-tenant Isolation**: Monitor RDMA resource usage by different tenants to provide basis for resource isolation
6. **Load Monitoring**: Track RDMA control path load through call rate statistics
7. **Abuse Detection**: Detect and intercept excessive use of RDMA resources

## Sample Output

When running, the program outputs information in the following format:

```
RDMA Control Path Monitor Started (interval: 2 seconds)
==================================================

==================================================
[2023-11-05 10:45:10] Interception Configuration:
==================================================
Resource Count Based Interception:
  QP_COUNT       : >1000
  MR_COUNT       : >5000

Frequency Based Interception:
  MR_REG         : >200/s
  QP_CREATE      : >100/s

Disabled Interceptions:
  PD_COUNT       : DISABLED
  CQ_COUNT       : DISABLED
  QP_MODIFY      : DISABLED
  PD_ALLOC       : DISABLED
==================================================

==================================================
[2023-11-05 10:45:10] RDMA Resource Counts:
==================================================
QP (Queue Pairs):     5
PD (Protection Domains): 3
CQ (Completion Queues): 5
MR (Memory Regions):  12
==================================================

[2023-11-05 10:45:10] Per-Cgroup RDMA Statistics:
==================================================
CGROUP ID: 1234567890
  RDMA_MONITOR_QP_CREATE    : 2
  RDMA_MONITOR_MR_REG       : 5
  RDMA_MONITOR_CQ_CREATE    : 2

CGROUP ID: 9876543210
  RDMA_MONITOR_QP_CREATE    : 3
  RDMA_MONITOR_MR_REG       : 7 (*** INTERCEPTED ***)
  RDMA_MONITOR_CQ_CREATE    : 3
  RDMA_MONITOR_PD_ALLOC     : 1
==================================================

[2023-11-05 10:45:10] RDMA Operation Frequency (calls per second):
==================================================
QP Create:  2/s
PD Alloc:   0/s
CQ Create:  1/s
MR Reg:     205/s (*** INTERCEPTED ***)
==================================================

==================================================
[2023-11-05 10:45:12] RDMA Control Path Monitor Stopped
==================================================
```

## Dependencies

- Linux kernel version >= 4.18 (with eBPF support)
- libbpf library
- clang/llvm compiler
- RDMA drivers (currently adapted for Mellanox drivers)

## Notes

1. Root privileges are required to run eBPF programs
2. Current implementation primarily targets Mellanox network card drivers
3. Kernel function names may need adjustment based on the actual environment
4. Interception currently only shows warnings in output; actual blocking requires additional system-level controls
5. Configuration file uses 0 to disable either resource count or frequency checks
6. Resource count based interception only affects creation/registration operations, not destruction/deallocation