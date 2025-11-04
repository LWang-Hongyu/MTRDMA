#!/bin/bash

# Benchmark script to test ib_write_bw performance with different -l values
# Tests message rates for -l values from 1 to 256

OUTPUT_FILE="ib_write_benchmark_results.csv"
DEVICE="mlx5_1"
SIZE=16  # Message size in bytes
DURATION=5  # Duration for each test in seconds

# Create or overwrite the output file with headers
echo "l_value,msg_rate,bandwidth_gbits" > $OUTPUT_FILE

# Function to extract values from ib_write_bw output
extract_values() {
    local file_path=$1
    local msg_rate=$(tail -n  2 "$file_path" | head -n 1 | awk '{print $5}')
    local bandwidth=$(tail -n 2 "$file_path" | head -n 1 | awk '{print $4}')
    echo "$msg_rate,$bandwidth"
}

# Print test information
echo "Starting InfiniBand write benchmark test"
echo "Device: $DEVICE"
echo "Message size: $SIZE bytes"
echo "Test duration: $DURATION seconds per test"
echo "Results will be saved to: $OUTPUT_FILE"
echo ""
echo "Testing -l values from 1 to 256..."
echo ""

# Run tests for l values from 1 to 256
for l_value in $(seq 1 128); do
    echo -n "Testing with -l $l_value... "
    
    # Run the ib_write_bw command with current -l value
    # Using timeout to ensure test runs for exactly DURATION seconds
    # result=$(timeout $DURATION ib_write_bw -F -d $DEVICE --report_gbits -s $SIZE -D 1 -l $l_value 2>&1)
    result=$((ssh phx@115.157.197.8 "timeout $DURATION ib_write_bw -F -d $DEVICE --report_gbits --run_infinitely -s $SIZE -D 1 -l $l_value > /dev/null" &) \
            && sleep 2 && timeout $DURATION ib_write_bw -F -d $DEVICE --report_gbits --run_infinitely -s $SIZE -D 1 -l $l_value 192.11.11.108 2>&1 > test.txt ) 

    # Extract values
    values=$(extract_values "test.txt")
    
    if [ -n "$values" ]; then
        msg_rate=$(echo $values | cut -d, -f1)
        bandwidth=$(echo $values | cut -d, -f2)
        
        # Save to CSV
        echo "$l_value,$msg_rate,$bandwidth" >> $OUTPUT_FILE
        
        echo "msg_rate: $msg_rate, bandwidth: $bandwidth Gbits/sec"
    else
        echo "Failed to extract values. Check if ib_write_bw ran correctly."
        echo "$l_value,Error,Error" >> $OUTPUT_FILE
    fi
done

# Generate a simple summary report
echo ""
echo "Benchmark completed. Results saved to $OUTPUT_FILE"
echo ""
echo "Best message rates:"
sort -t, -k2 -nr $OUTPUT_FILE | head -5 | awk -F, '{print "l_value: " $1 ", msg_rate: " $2 ", bandwidth: " $3 " Gbits/sec"}'
echo ""
echo "To visualize results, you can use the following command to generate a graph:"
echo "awk -F, 'NR>1 {print \$1,\$2}' $OUTPUT_FILE | sort -n | gnuplot -e \"set terminal png; set output 'msg_rate_graph.png'; set xlabel 'l value'; set ylabel 'Message Rate'; plot '-' with lines title 'Message Rate vs l value'\""