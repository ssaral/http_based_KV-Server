#!/bin/bash

# Run a complete load test experiment with multiple load levels
# Usage: ./run_experiment.sh <workload_type> <output_file>

if [ $# -lt 2 ]; then
    echo "Usage: $0 <workload_type> <output_file>"
    echo "Workload types: put_all, get_all, get_popular, get_put"
    exit 1
fi

WORKLOAD=$1
OUTPUT_FILE=$2
DURATION=300  # 5 minutes per test
LOAD_LEVELS=(5 10 20 50 100)

echo "=== Running Load Test Experiment ===" > $OUTPUT_FILE
echo "Workload: $WORKLOAD" >> $OUTPUT_FILE
echo "Duration per test: $DURATION seconds" >> $OUTPUT_FILE
echo "Load levels: ${LOAD_LEVELS[@]}" >> $OUTPUT_FILE
echo "" >> $OUTPUT_FILE
echo "Load Level,Throughput (req/sec),Avg Response Time (ms),Success Rate (%)" >> $OUTPUT_FILE

for load_level in "${LOAD_LEVELS[@]}"; do
    echo "Running test with $load_level threads..."
    
    # Run the load test and capture output
    OUTPUT=$(bash scripts/run_client.sh $load_level $DURATION $WORKLOAD 2>&1)
    
    # Extract metrics from output
    THROUGHPUT=$(echo "$OUTPUT" | grep "Throughput:" | awk '{print $2}')
    AVG_RESPONSE=$(echo "$OUTPUT" | grep "Avg Response Time:" | awk '{print $4}')
    SUCCESS_RATE=$(echo "$OUTPUT" | grep "Success Rate:" | awk '{print $3}')
    
    # Append to output file
    echo "$load_level,$THROUGHPUT,$AVG_RESPONSE,$SUCCESS_RATE" >> $OUTPUT_FILE
    
    # Wait between tests
    sleep 10
done

echo ""
echo "Experiment complete! Results saved to $OUTPUT_FILE"
cat $OUTPUT_FILE
