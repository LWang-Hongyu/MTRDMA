#!/bin/bash

# 配置参数
SERVER_IP="192.11.11.106"  # 远端服务器IP
DEVICE="mlx5_1"           # 使用的RDMA设备
OUTPUT_FILE="bw_results_back_4096_mtrdma.csv"  # 输出结果文件

# 创建结果文件并写入标题行
echo "bytes,avg_bw,avg_msg_rate" > $OUTPUT_FILE

# 注：请将user替换为实际的用户名，可能需要设置免密登录

# 循环测试不同的消息大小
for ((size=16; size<=4096; size*=2)); do
    echo "Testing with message size: $size bytes"
    
    # 运行客户端测试
    result=$(timeout 7 ib_write_bw  -F -s $size --run_infinitely --report_gbits -d $DEVICE -D 1 -p $(($size + 50000)) > /dev/null & ssh phx@115.157.197.8 "timeout 5 ib_write_bw  -F -s $size --run_infinitely --report_gbits -d $DEVICE -D 1 $SERVER_IP -p $(($size + 50000))" )
    
    echo "$result" > "result_$size.txt"

    # 提取所需数据
    avg_bw=$(echo "$result" | awk '/^ '$size'/ {print $4}' | tail -1)
    avg_msg_rate=$(echo "$result" | awk '/^ '$size'/ {print $5}' | tail -1)
    
    # 将结果写入文件
    echo "$size,$avg_bw,$avg_msg_rate" >> $OUTPUT_FILE
    
    # 每次测试间隔
    sleep 1
done

echo "All tests completed. Results saved to $OUTPUT_FILE"