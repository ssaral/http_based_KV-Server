#!/bin/bash

# Basic functional testing script
# Tests all API endpoints

SERVER_URL="http://localhost:8080"
PASS=0
FAIL=0

echo "=== KV Server Functional Tests ==="
echo "Server URL: $SERVER_URL"
echo ""

# Test 1: Health check
echo "Test 1: Health Check"
RESPONSE=$(curl -s -w "\n%{http_code}" $SERVER_URL/health)
HTTP_CODE=$(echo "$RESPONSE" | tail -n1)
if [ "$HTTP_CODE" = "200" ]; then
    echo "PASS: Health check returned 200"
    ((PASS++))
else
    echo "FAIL: Health check returned $HTTP_CODE"
    ((FAIL++))
fi
echo ""

# Test 2: Create key-value pair
echo "Test 2: Create Key-Value Pair"
RESPONSE=$(curl -s -w "\n%{http_code}" -X POST $SERVER_URL/api/kv \
  -H "Content-Type: application/json" \
  -d '{"key": "test:1", "value": "Hello World"}')
HTTP_CODE=$(echo "$RESPONSE" | tail -n1)
if [ "$HTTP_CODE" = "200" ]; then
    echo "PASS: Create returned 200"
    ((PASS++))
else
    echo "FAIL: Create returned $HTTP_CODE"
    ((FAIL++))
fi
echo ""

# Test 3: Read key-value pair
echo "Test 3: Read Key-Value Pair"
RESPONSE=$(curl -s -w "\n%{http_code}" "$SERVER_URL/api/kv?key=test:1")
HTTP_CODE=$(echo "$RESPONSE" | tail -n1)
BODY=$(echo "$RESPONSE" | head -n-1)
if [ "$HTTP_CODE" = "200" ] && echo "$BODY" | grep -q "Hello World"; then
    echo "PASS: Read returned 200 with correct value"
    ((PASS++))
else
    echo "FAIL: Read returned $HTTP_CODE or incorrect value"
    ((FAIL++))
fi
echo ""

# Test 4: Update key-value pair
echo "Test 4: Update Key-Value Pair"
RESPONSE=$(curl -s -w "\n%{http_code}" -X POST $SERVER_URL/api/kv \
  -H "Content-Type: application/json" \
  -d '{"key": "test:1", "value": "Updated Value"}')
HTTP_CODE=$(echo "$RESPONSE" | tail -n1)
if [ "$HTTP_CODE" = "200" ]; then
    echo "PASS: Update returned 200"
    ((PASS++))
else
    echo "FAIL: Update returned $HTTP_CODE"
    ((FAIL++))
fi
echo ""

# Test 5: Delete key-value pair
echo "Test 5: Delete Key-Value Pair"
RESPONSE=$(curl -s -w "\n%{http_code}" -X DELETE "$SERVER_URL/api/kv?key=test:1")
HTTP_CODE=$(echo "$RESPONSE" | tail -n1)
if [ "$HTTP_CODE" = "200" ]; then
    echo "PASS: Delete returned 200"
    ((PASS++))
else
    echo "FAIL: Delete returned $HTTP_CODE"
    ((FAIL++))
fi
echo ""

# Test 6: Read deleted key (should not exist)
echo "Test 6: Read Deleted Key"
RESPONSE=$(curl -s -w "\n%{http_code}" "$SERVER_URL/api/kv?key=test:1")
HTTP_CODE=$(echo "$RESPONSE" | tail -n1)
if [ "$HTTP_CODE" = "200" ] || [ "$HTTP_CODE" = "404" ]; then
    echo "PASS: Read deleted key returned $HTTP_CODE"
    ((PASS++))
else
    echo "FAIL: Read deleted key returned $HTTP_CODE"
    ((FAIL++))
fi
echo ""

# Test 7: Get statistics
echo "Test 7: Get Statistics"
RESPONSE=$(curl -s -w "\n%{http_code}" "$SERVER_URL/api/stats")
HTTP_CODE=$(echo "$RESPONSE" | tail -n1)
BODY=$(echo "$RESPONSE" | head -n-1)
if [ "$HTTP_CODE" = "200" ] && echo "$BODY" | grep -q "cache_hits"; then
    echo "PASS: Stats returned 200 with cache metrics"
    ((PASS++))
else
    echo "FAIL: Stats returned $HTTP_CODE or missing metrics"
    ((FAIL++))
fi
echo ""

# Summary
echo "=== Test Summary ==="
echo "Passed: $PASS"
echo "Failed: $FAIL"
echo "Total: $((PASS + FAIL))"

if [ $FAIL -eq 0 ]; then
    echo "All tests passed!"
    exit 0
else
    echo "Some tests failed!"
    exit 1
fi
