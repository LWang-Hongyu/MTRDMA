#!/bin/bash

SIZE=16
for i in $(seq 0 2 8)
do
    SIZE=$((SIZE * 2))

    echo $SIZE
done