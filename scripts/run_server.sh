#!/bin/bash

# CPU pinning for server (cores 0-3)
# Adjust core numbers based on your system
echo "Starting KV Server with CPU pinning to cores 0-3..."
taskset -c 0-3 ./build/bin/kv_server \
    --port 8080 \
    --threads 4 \
    --cache-size 10 \
    --db-conn "host=localhost user=postgres password=postgres dbname=kvstore"
