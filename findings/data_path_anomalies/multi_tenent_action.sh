#!/bin/bash

for i in $(seq 0 2 20)
do
    bash data_verbs_bench.sh $i send lat 1024 3 $((i + 90000)) cycle_test_cpu_$i &
done