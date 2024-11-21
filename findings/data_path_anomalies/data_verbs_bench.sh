#!/bin/bash

if [ "$#" -ne 7 ]; then
    echo "Error: Seven parameters are required."
    echo "Usage: $0 <cpu id> <operation: send/write/read> <type: bw/lat> <message size> <qp num> <port> <test comment>"
    echo "Example: $0 2 send bw 1024 1 90000 bw_test"ls
    exit 1
fi

TAG=$(date +%Y%m%d-%H%M%S)

CPU=$1

BINDS="numactl --physcpubind=${CPU} "
BINDC="numactl --physcpubind=${CPU} "

# server ip
DEST="phx@115.157.197.8"

# server rdma ip
RNICDEST="192.11.11.106"

# get data operation
OPERATION=$2
TYPE=$3

# test case of perftest
PERFCASE="ib_${OPERATION}_${TYPE}"

# exchange message size
SIZE=$4

# test qp num
QPNUM=$5

PORT=$6

# test comment
COMMENT=$7



if [ "$TYPE" = "bw" ]; then
    # server side cmd
    SCMD="/home/phx/MTRDMA/perftest-v4.5-0.20/${PERFCASE} -F --report_gbits --run_infinitely -d mlx5_1 -D 1 -s ${SIZE} -q ${QPNUM} -p ${PORT}"

    # client side cmd
    CCMD="/home/phx/MTRDMA/perftest-v4.5-0.20/${PERFCASE} -F --report_gbits --run_infinitely -d mlx5_1 -D 1 -s ${SIZE} ${RNICDEST} -q ${QPNUM} -p ${PORT}"
else
    SCMD="/home/phx/MTRDMA/perftest-v4.5-0.20/${PERFCASE} -F -d mlx5_1 -s ${SIZE} -p ${PORT}"

    CCMD="/home/phx/MTRDMA/perftest-v4.5-0.20/${PERFCASE} -F -d mlx5_1 -s ${SIZE} ${RNICDEST} -p ${PORT}"
fi

# create directory
mkdir -p "/home/phx/MTRDMA/findings/data_path_anomalies/logs"

# Execute send operation
$BINDS $SCMD > /dev/null & ssh $DEST "timeout 10 $BINDC $CCMD " 2>&1 > ./logs/log-${PERFCASE}-${COMMENT}-${TAG}.txt