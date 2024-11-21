#!/bin/bash

TAG=$(date +%Y%m%d-%H%M%S)

BINDS="numactl --physcpubind=2 "
BINDC="numactl --physcpubind=2 "

# server ip
DEST="phx@115.157.197.8"

# server rdma ip
RNICDEST="192.10.10.106"

# test case of perftest
PERFCASE="ib_send_bw"

# exchange message size
SIZE=16

# server side cmd
SCMD="${PERFCASE} -F --report_gbits --run_infinitely -d mlx5_0 -D 1 -s ${SIZE}"

# client side cmd
CCMD="/home/phx/MTRDMA/perftest/${PERFCASE} -F --report_gbits --run_infinitely -d mlx5_0 -D 1 -s ${SIZE} ${RNICDEST}"

# Execute send operation
$BINDS $SCMD > /dev/null & ssh $DEST "timeout 10 $BINDC $CCMD " 2>&1 | tee log-$PERFCASE-$TAG.txt

pkill $PERFCASE
