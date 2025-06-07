#!/bin/bash

PROXY_PORT=8080
PROXY_ADDR="http://127.0.0.1:$PROXY_PORT"
ACCESS_LOG="doc/access.log"
FORBIDDEN_FILE="doc/forbidden.txt"

echo "[*] Starting test suite..."

# 1. Test: Simple GET request to allowed site
echo "[1] Testing allowed GET request..."
curl -x $PROXY_ADDR -s -o /dev/null -w "%{http_code}\n" http://example.com

# 2. Test: HEAD request to allowed site
echo "[2] Testing allowed HEAD request..."
curl -I -x $PROXY_ADDR -s -o /dev/null -w "%{http_code}\n" http://example.com

# 3. Test: Forbidden site
echo "[3] Testing forbidden site (should return 403)..."
curl -x $PROXY_ADDR -s -o /dev/null -w "%{http_code}\n" http://fakenews.com

# 4. Test: Invalid HTTP method
echo "[4] Testing POST (should return 501)..."
curl -X POST -x $PROXY_ADDR -s -o /dev/null -w "%{http_code}\n" http://example.com

# 5. Test: Unresolvable domain
echo "[5] Testing bad domain (should return 502)..."
curl -x $PROXY_ADDR -s -o /dev/null -w "%{http_code}\n" http://nonexistent12345.ucsc

echo "[*] Tests complete. Check $ACCESS_LOG for log output."
