#!/bin/bash

if [ "$#" -ne 2 ]; then
    echo "Error: Two parameters are required."
    echo "Usage: $0 <side: server/client> <action time>"
    echo "Example: $0 server 10"
    exit 1
fi

# get parameters
FILE=$0
SIDE=$1
TIME=$2

# create directory
LOGDIR="/home/phx/MTRDMA/findings/data_path_anomalies/logs/${FILE%.sh}_logs"

mkdir -p $LOGDIR

SIZE=16

for i in $(seq 0 2 8)
do
    if [ "$SIDE" = "server" ]; then
        # server side cmd
        CMD="numactl --physcpubind=$i /home/phx/MTRDMA/perftest-v4.5-0.20/ib_write_bw -F --report_gbits --run_infinitely -d mlx5_1 -D 1 -s ${SIZE} -q 1 -p $((i + 90000)) > /dev/null & "
    else
        CMD="numactl --physcpubind=$i /home/phx/MTRDMA/perftest-v4.5-0.20/ib_write_bw -F --report_gbits --run_infinitely -d mlx5_1 -D 1 -s ${SIZE} -q 1 -p $((i + 90000)) 192.11.11.108 > ${LOGDIR}/log-ib_write_bw-test-cpu_$i.txt &"
    fi
done

if [ "$SIDE" = "client" ]; then
    sleep $TIME
    pkill ib_write_bw
fi

echo -e "\033[36m $0 $SIDE side Success! \033[0m"