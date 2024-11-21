#!/bin/bash

if [ "$#" -ne 5 ]; then
    echo "Error: Five parameters are required."
    echo "Usage: $0 <operation: send/write/read> <type: bw/lat> <message size> <qp num> <test comment>"
    echo "Example: $0 send bw 1024 1 bw_test"ls
    exit 1
fi

TAG=$(date +%Y%m%d-%H%M%S)

BINDS="numactl --physcpubind=2 "
BINDC="numactl --physcpubind=2 "

# server ip
DEST="phx@115.157.197.8"

# server rdma ip
RNICDEST="192.11.11.106"

# get data operation
OPERATION=$1
TYPE=$2

# test case of perftest
PERFCASE="ib_${OPERATION}_${TYPE}"

# exchange message size
SIZE=$3

# test qp num
QPNUM=$4

# test comment
COMMENT=$5

if [ "$TYPE" = "bw" ]; then
    # server side cmd
    SCMD="/home/phx/MTRDMA/perftest-v4.5-0.20/${PERFCASE} -F --report_gbits --run_infinitely -d mlx5_1 -D 1 -s ${SIZE} -q ${QPNUM}"

    # client side cmd
    CCMD="/home/phx/MTRDMA/perftest-v4.5-0.20/${PERFCASE} -F --report_gbits --run_infinitely -d mlx5_1 -D 1 -s ${SIZE} ${RNICDEST} -q ${QPNUM}"
else
    SCMD="/home/phx/MTRDMA/perftest-v4.5-0.20/${PERFCASE} -F -d mlx5_1 -s ${SIZE}"

    CCMD="/home/phx/MTRDMA/perftest-v4.5-0.20/${PERFCASE} -F -d mlx5_1 -s ${SIZE} ${RNICDEST}"
fi

# Execute send operation
$BINDS $SCMD > /dev/null & ssh $DEST "timeout 10 $BINDC $CCMD " 2>&1 | tee ./logs/log-${PERFCASE}-${COMMENT}-${TAG}.txt

pkill $PERFCASE