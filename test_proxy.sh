#!/bin/bash

# test_proxy.sh - automated tests for myproxy

PROXY_PORT=8080
PROXY_HOST=127.0.0.1
FORBIDDEN_FILE=forbidden.txt
LOG_FILE=access.log

# Wait for proxy to be ready
sleep 1

# Clear previous logs
rm -f "$LOG_FILE"

# Test 1: Simple GET request
curl -x http://$PROXY_HOST:$PROXY_PORT http://example.com/ -o /dev/null -s -w "Test 1 (GET example.com): %{http_code}\n"

# Test 2: Simple HEAD request
curl -I -x http://$PROXY_HOST:$PROXY_PORT http://example.com/ -s -o /dev/null -w "Test 2 (HEAD example.com): %{http_code}\n"

# Test 3: Forbidden site (should return 403)
echo "www.fakenews.com" > $FORBIDDEN_FILE
curl -x http://$PROXY_HOST:$PROXY_PORT http://fakenews.com/ -o /dev/null -s -w "Test 3 (GET forbidden site): %{http_code}\n"

# Test 4: Bad method (POST)
curl -X POST -x http://$PROXY_HOST:$PROXY_PORT http://example.com/ -o /dev/null -s -w "Test 4 (POST method): %{http_code}\n"

# Test 5: Non-existent domain (should return 502)
curl -x http://$PROXY_HOST:$PROXY_PORT http://nonexistent12345.ucsc/ -o /dev/null -s -w "Test 5 (bad DNS): %{http_code}\n"

# Test 6: CONNECT method (should return 501)
echo -e "CONNECT www.example.com:443 HTTP/1.1\r\nHost: www.example.com:443\r\n\r\n" \
    | nc $PROXY_HOST $PROXY_PORT | head -n 1 | grep -q "501" && \
    echo "Test 6 (CONNECT method): 501" || echo "Test 6 (CONNECT method): Failed"
