#!/bin/bash

if [ "$#" -ne 2 ]; then
    echo "Error: Two parameters are required."
    echo "Usage: $0 <side: server/client> <action time>"
    echo "Example: $0 server 10"
    exit 1
fi

PERFCASE=ib_write_bw

pkill $PERFCASE

# get parameters
FILE=$0
SIDE=$1
TIME=$2

# create directory
LOGDIR="/home/phx/MTRDMA/findings/data_path_anomalies/logs/${FILE%.sh}_logs"

if [ "$SIDE" = "client" ]; then
    mkdir -p $LOGDIR
    rm -f $LOGDIR/*
fi

# BASESIZE=8192
# SIZE=$BASESIZE
# ITERS=5

SIZE=1024

for i in $(seq 0 1 39)
do
    if [ "$SIDE" = "server" ]; then
        # server side cmd
        numactl --physcpubind=$i /home/phx/MTRDMA/perftest-v4.5-0.20/ib_write_bw -F --report_gbits --run_infinitely -d mlx5_1 -D 1 -s ${SIZE} -q 10 -p $((i + 90000)) > /dev/null &
    else
        numactl --physcpubind=$i /home/phx/MTRDMA/perftest-v4.5-0.20/ib_write_bw -F --report_gbits --run_infinitely -d mlx5_1 -D 1 -s ${SIZE} -q 10 -p $((i + 90000)) 192.11.11.108 > ${LOGDIR}/log-ib_write_bw-test-cpu_$i.txt &
    fi

    # SIZE=$((BASESIZE * ITERS))
    # ITERS=$((ITERS + 5))
done

# numactl --physcpubind=38 /home/phx/MTRDMA/perftest-v4.5-0.20/ib_write_bw -F --report_gbits --run_infinitely -d mlx5_1 -D 1 -s 4096 -q 1 -p 90038 192.11.11.108 > ${LOGDIR}/log-ib_write_bw-test-cpu_38.txt &

if [ "$SIDE" = "client" ]; then
    sleep $TIME
    pkill ib_write_bw

    bash /home/phx/MTRDMA/findings/data_path_anomalies/logs/statistical_data.sh
else
    sleep $((TIME + 5))
    pkill ib_write_bw
fi

echo -e "\033[36m $0 $SIDE side Success! \033[0m"