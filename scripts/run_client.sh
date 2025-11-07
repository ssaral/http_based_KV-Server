#!/bin/bash

# CPU pinning for load generator (cores 4-7)
# Adjust core numbers based on your system
if [ $# -lt 3 ]; then
    echo "Usage: $0 <num_threads> <duration_seconds> <workload_type>"
    echo "Workload types: put_all, get_all, get_popular, get_put"
    exit 1
fi

NUM_THREADS=$1
DURATION=$2
WORKLOAD=$3

echo "Running load test with $NUM_THREADS threads for $DURATION seconds (workload: $WORKLOAD)..."
taskset -c 4-7 ./build/bin/load_generator \
    --url "http://localhost:8080" \
    --threads $NUM_THREADS \
    --duration $DURATION \
    --workload $WORKLOAD
