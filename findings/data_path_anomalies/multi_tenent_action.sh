#!/bin/bash

TIME=10

FILE=$0
# create directory
LOGDIR="/home/phx/MTRDMA/findings/data_path_anomalies/logs/${FILE%.sh}_logs"

mkdir -p $LOGDIR

for i in $(seq 0 2 8)
do
    bash data_verbs_bench.sh $i send bw 1024 3 $((i + 90000)) $TIME time_test_cpu_$i $LOGDIR &
done

sleep $TIME

pkill ib_send_bw

echo -e "\033[36m $0 Success! \033[0m"