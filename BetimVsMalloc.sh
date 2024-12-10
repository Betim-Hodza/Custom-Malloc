#!/bin/bash

make clean
make

# List of custom malloc implementations
ALLOCATORS=("ff" "bf" "wf" "nf")
ALLOCATOR_LIBS=("libmalloc-ff.so" "libmalloc-bf.so" "libmalloc-wf.so" "libmalloc-nf.so")

# List of test programs (add your test program names here)
TESTS=("mallocTest1" "mallocTest2" "mallocTest3" "mallocTest4" "mallocTest5")

# Function to run tests with custom allocator
run_custom_malloc() {
    allocator_name=$1
    allocator_lib=$2
    test_program=$3

    echo "Running $test_program with $allocator_name allocator"
    env LD_PRELOAD=./lib/$allocator_lib ./tests/$test_program > ./results/output_${allocator_name}_${test_program}.txt
    echo "Output saved to output_${allocator_name}_${test_program}.txt"
}

# Function to run tests with system malloc
run_system_malloc() {
    test_program=$1

    echo "Running $test_program with system malloc"
    ./tests/$test_program > ./results/output_system_${test_program}.txt
    echo "Output saved to output_system_${test_program}.txt"
}

# Main script execution
for test in "${TESTS[@]}"; do
    echo "----------------------------------------"
    echo "Starting tests for $test"
    echo "----------------------------------------"

    # Run test with system malloc
    run_system_malloc $test

    # Run test with each custom allocator
    for i in "${!ALLOCATORS[@]}"; do
        allocator=${ALLOCATORS[$i]}
        lib=${ALLOCATOR_LIBS[$i]}
        run_custom_malloc $allocator $lib $test
    done
done

# Optional: Display the results
echo
echo "Test execution completed. Outputs are saved in the results directory."
echo "You can compare the outputs using comaprePerformance.sh or view them directly."
