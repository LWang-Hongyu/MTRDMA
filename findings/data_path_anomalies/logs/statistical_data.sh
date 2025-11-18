#!/bin/bash

logs_dir=$1
comment=$2

# check dir
if [ ! -d "$logs_dir" ]; then
    echo "dir $logs_dir is non-existent"
    exit 1
fi

total_bw=0

# loop dir
for file in "$logs_dir"/*; do
    # check file
    if [ ! -f "$file" ]; then
        continue
    fi

    last_line=$(tail -n 1 "$file")
    byte_count=$(echo "$last_line" | awk '{print $1}')
    bw_average=$(echo "$last_line" | awk '{print $4}')
    msg_rate=$(echo "$last_line" | awk '{print $5}')

    total_bw=$(echo "$total_bw + $bw_average" | bc)

    # 打印结果
    echo "file: $(basename "$file")"
    echo "bytes: $byte_count"
    echo "BW average[Gb/sec]: $bw_average"
    echo "MsgRate[Mpps]: $msg_rate"
    echo "-----------------------------"

    echo "$(basename "$file"),$byte_count,$bw_average,$msg_rate" >> /home/phx/MTRDMA/findings/data_path_anomalies/logs/temp.csv
done

echo "total bw: $total_bw" | tee -a /home/phx/MTRDMA/findings/data_path_anomalies/logs/temp.csv
echo "comment: $comment" >> /home/phx/MTRDMA/findings/data_path_anomalies/logs/temp.csv
echo "-----------------------------" >> /home/phx/MTRDMA/findings/data_path_anomalies/logs/temp.csv


