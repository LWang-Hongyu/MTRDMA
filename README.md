# MTRDMA

Multi-Tenant RDMA Performance Isolation Solution

## Overview

MTRDMA is a solution designed to provide performance isolation for RDMA (Remote Direct Memory Access) in multi-tenant environments. It monitors and controls RDMA resources to ensure fair allocation and prevent performance interference between different tenants.

## Project Structure

- `compress_resource/`: Contains scripts for compressing resources
- `control_verbs_action/`: eBPF programs for monitoring and controlling RDMA Verbs actions
- `mtrdma_main.c`: Main program for initializing shared memory and managing RDMA resources
- `perftest-v4.5-0.20/`: Performance testing tools for RDMA
- `test/`: Test scripts and results

## How to Run

1. **Compile the main program**:
   ```bash
   gcc mtrdma_main.c -o mtrdma_main -lrt -lpthread
   ```

2. **Compile and load eBPF programs**:
   ```bash
   cd control_verbs_action/ebpf
   make
   # This will compile the eBPF program and the user-space loader
   
   # Run the eBPF monitor (requires root privileges)
   sudo ./rdma_monitor
   ```

3. **Run the main MTRDMA program**:
   ```bash
   # In a separate terminal, run the main program (requires root privileges)
   sudo ./mtrdma_main
   ```

4. **Compile and install the modified RDMA library**:
   ```bash
   cd rdma-core-58mlnx43
   # Follow standard build process for rdma-core
   mkdir build && cd build
   cmake ..
   make
   sudo make install
   ```

5. **Run performance tests**:
   ```bash
   # Using the provided test scripts
   cd test/bw_test
   ./bw_test.sh
   
   # Or using the perftest tools
   cd perftest-v4.5-0.20
   # Run RDMA performance tests like ib_write_bw
   ```

## Components

- **mtrdma_main**: Main daemon that manages shared memory and coordinates RDMA resource management
- **eBPF Monitor**: Kernel-level monitoring of RDMA Verbs actions using eBPF kprobes
- **Modified RDMA Library**: Customized version of rdma-core with MTRDMA support
- **Performance Tests**: Scripts and tools to measure and validate RDMA performance under MTRDMA control

## Requirements

- Linux kernel with eBPF support (4.18 or newer)
- LLVM/Clang for compiling eBPF programs
- RDMA capable hardware (Mellanox ConnectX series recommended)
- libbpf development files
- Appropriate permissions to load eBPF programs (usually root)
- CMake and build tools for compiling rdma-core

## How It Works

MTRDMA uses a hybrid architecture to provide RDMA performance isolation:

1. **User-space Manager**: The mtrdma_main program creates and manages a shared memory region that contains tenant information and resource quotas.

2. **eBPF Monitor**: Kernel-level monitoring of RDMA operations using eBPF kprobes attached to key functions like QP creation, modification, and destruction.

3. **Modified RDMA Stack**: A customized version of the rdma-core library that coordinates with the user-space manager through shared memory to enforce resource limits.

4. **Shared Memory Communication**: All components communicate through a shared memory region to coordinate resource allocation and tenant isolation.

The system works by tracking tenant resource usage and dynamically adjusting quotas to ensure fair allocation while preventing any single tenant from monopolizing RDMA resources.