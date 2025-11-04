#!/bin/bash

if [ "$#" -ne 9 ]; then
    echo "Error: Nine parameters are required."
    echo "Usage: $0 <cpu id> <operation: send/write/read> <type: bw/lat> <message size> <qp num> <port> <action time> <test comment> <log dir>"
    echo "Example: $0 2 send bw 1024 1 90000 10 bw_test <log dir>"
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

# action time
TIME=$7

# test comment
COMMENT=$8

# log dir
LOGDIR=$9


if [ "$TYPE" = "bw" ]; then
    # server side cmd
    SCMD="/home/phx/MTRDMA/perftest-v4.5-0.20/${PERFCASE} -F --report_gbits --run_infinitely -d mlx5_1 -D 1 -s ${SIZE} -q ${QPNUM} -p ${PORT}"

    # client side cmd
    CCMD="/home/phx/MTRDMA/perftest-v4.5-0.20/${PERFCASE} -F --report_gbits --run_infinitely -d mlx5_1 -D 1 -s ${SIZE} ${RNICDEST} -q ${QPNUM} -p ${PORT}"
else
    SCMD="/home/phx/MTRDMA/perftest-v4.5-0.20/${PERFCASE} -F -d mlx5_1 -s ${SIZE} -p ${PORT}"

    CCMD="/home/phx/MTRDMA/perftest-v4.5-0.20/${PERFCASE} -F -d mlx5_1 -s ${SIZE} ${RNICDEST} -p ${PORT}"
fi

# Execute send operation
$BINDS $SCMD > /dev/null & ssh $DEST "timeout $TIME $BINDC $CCMD " 2>&1 > ${LOGDIR}/log-${PERFCASE}-${COMMENT}-${TAG}.txt