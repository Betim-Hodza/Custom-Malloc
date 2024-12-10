#!/bin/bash

# List of custom malloc implementations
ALLOCATORS=("ff" "bf" "wf" "nf")
ALLOCATOR_NAMES=("First Fit" "Next Fit" "Best Fit" "Worst Fit" )

# List of test programs
TESTS=("mallocTest1" "mallocTest2" "mallocTest3" "mallocTest4" "mallocTest5")

# Function to extract execution time from output file
extract_time() {
    file=$1
    grep "Execution Time:" "$file" | awk '{print $3}'
}

extract_maxheap() {
    file=$1
    metric_name=$2
    grep -E "^$metric_name:" "$file" | awk '{print $3}'
}

# Extract specific metric from output file
extract_metric() {
    file=$1
    metric_name=$2
    grep -E "^$metric_name:" "$file" | awk '{print $2}'
}

# Main script execution
echo "Performance Comparison of Custom malloc Implementations vs System malloc"
echo "========================================================================================="
printf "%-15s %-15s %-15s %-15s %-15s %-15s %-15s\n" "Test Program" "Allocator" "Time (s)" "Speedup" "MaxHeap" "Fragments" "Splits"

for test in "${TESTS[@]}"; do
    # Extract system malloc execution time
    system_output="./results/output_system_${test}.txt"
    if [ -f "$system_output" ]; then
        system_time=$(extract_time "$system_output")
    else
        echo "System output file $system_output not found."
        continue
    fi
    # Extract sys Max heap
    sys_maxHeap=$(grep "sys maxheap:" "$system_output" | awk '{print $3}')
    if [ -z "$sys_maxHeap" ]; then
        sys_maxHeap="N/A"
    fi

    # Display system malloc time
    printf "%-15s %-15s %-15s %-15s %-15s %-15s %-15s\n" "$test" "System" "$system_time" "1.00000x" "N/A" "N/A" "N/A"

    # Loop over custom allocators
    for i in "${!ALLOCATORS[@]}"; do
        allocator=${ALLOCATORS[$i]}
        allocator_name=${ALLOCATOR_NAMES[$i]}
        output_file="./results/output_${allocator}_${test}.txt"

        if [ -f "$output_file" ]; then
            allocator_time=$(extract_time "$output_file")
            if [ -z "$allocator_time" ]; then
                echo "Execution time not found in $output_file."
                continue
            fi

            # Calculate speedup or slowdown
            speedup=$(echo "scale=6; $system_time / $allocator_time" | bc)
            # Format the speedup to 6 decimal places and append 'x'
            formatted_speedup=$(printf "%.5fx" "$speedup")

            # Extract fragmentation ratio
            fragmentation_ratio=$(grep "Fragmentation Ratio:" "$output_file" | awk '{print $3}')
            if [ -z "$fragmentation_ratio" ]; then
                fragmentation_ratio="N/A"
            fi

            # Extract number of splits
            splits=$(extract_metric "$output_file" "splits")
            if [ -z "$splits" ]; then
                splits="N/A"
            fi

            # Extract Max heap
            maxHeap=$(extract_maxheap "$output_file" "max heap")
            if [ -z "$maxHeap" ]; then
                maxHeap="N/A"
            fi


            printf "%-15s %-15s %-15s %-15s %-15s %-15s %-15s \n" "$test" "$allocator_name" "$allocator_time" "$formatted_speedup" "$maxHeap" "$fragmentation_ratio" "$splits"
        else
            echo "Output file $output_file not found."
        fi
    done
    echo "---------------------------------------------------------------------------------------------------"
done
