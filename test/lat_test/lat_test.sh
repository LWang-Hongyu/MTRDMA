#!/bin/bash

# 配置参数
SERVER_IP="192.11.11.106"  # 远端服务器IP
DEVICE="mlx5_1"           # 使用的RDMA设备
ITERATIONS=5000000        # 迭代次数
OUTPUT_FILE="latency_results_back_4096_mtrdma.csv"  # 输出结果文件

# 创建结果文件并写入标题行
echo "bytes,t_avg[usec],99%_percentile[usec]" > $OUTPUT_FILE

# 注：请将user替换为实际的用户名，可能需要设置免密登录
pkill ib_write_lat

# 循环测试不同的消息大小
for ((size=16; size<=4096; size*=2)); do
    echo "Testing with message size: $size bytes"
    
    # 运行客户端测试
    result=$(ib_write_lat  -F -s $size -n $ITERATIONS -d $DEVICE -p $(($size + 50000)) > /dev/null & ssh phx@115.157.197.8 "ib_write_lat -F -s $size -n $ITERATIONS -d $DEVICE $SERVER_IP -p $(($size + 50000))" )
    
    echo "$result" > "result_$size.txt"

    # 提取所需数据
    t_avg=$(echo "$result" | awk '/^ '$size'/ {print $5}')
    percentile_99=$(echo "$result" | awk '/^ '$size'/ {print $8}')
    
    # 将结果写入文件
    echo "$size,$t_avg,$percentile_99" >> $OUTPUT_FILE
    
    # 每次测试间隔
    sleep 1
done

echo "All tests completed. Results saved to $OUTPUT_FILE"