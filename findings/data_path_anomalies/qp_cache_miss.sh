#!/bin/bash

if [ "$#" -ne 5 ]; then
    echo "Error: Five parameters are required."
    echo "Usage: $0 <side: server/client> <action time> <qp_num> <size> <number of cpu>"
    echo "Example: $0 server 10 10 1024 5"
    exit 1
fi

PERFCASE=ib_write_bw

pkill $PERFCASE

# get parameters
FILE=$0
SIDE=$1
TIME=$2
QPNUM=$3
SIZE=$4
CPUNUM=$5
if [ $CPUNUM -gt 39 ]; then
    echo "Error: CPU number cannot be greater than 39"
    exit 1
fi

# create directory
LOGDIR="/home/phx/MTRDMA/findings/data_path_anomalies/logs/${FILE%.sh}_logs"

if [ "$SIDE" = "client" ]; then
    mkdir -p $LOGDIR
    rm -f $LOGDIR/*
fi

for i in $(seq 0 1 $CPUNUM)
do
    if [ "$SIDE" = "server" ]; then
        # server side cmd
        /home/phx/MTRDMA/perftest-v4.5-0.20/ib_write_bw -F --report_gbits --run_infinitely -d mlx5_1 -D 1 -s ${SIZE} -q ${QPNUM} -p $((i + 80000)) > /dev/null &
    else
        /home/phx/MTRDMA/perftest-v4.5-0.20/ib_write_bw -F --report_gbits --run_infinitely -d mlx5_1 -D 1 -s ${SIZE} -q ${QPNUM} -p $((i + 80000)) 192.11.11.108 > ${LOGDIR}/log-ib_write_bw-test-cpu_$i.txt &
    fi
done

if [ "$SIDE" = "client" ]; then
    sleep $TIME
    pkill ib_write_bw

    bash /home/phx/MTRDMA/findings/data_path_anomalies/logs/statistical_data.sh $LOGDIR "QP_NUM: $((QPNUM * (CPUNUM + 1))), MESSAGE_SIZE: $SIZE"
else
    sleep $((TIME + 5))
    pkill ib_write_bw
fi

echo -e "\033[36m $0 $SIDE side Success! \033[0m"