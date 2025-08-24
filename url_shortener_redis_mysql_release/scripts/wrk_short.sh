#!/usr/bin/env bash
set -euo pipefail
URL=${1:-http://127.0.0.1:8080}
set -x
ID=$(curl -s -XPOST "$URL/api/shorten" -H 'content-type: application/json' -d '{"url":"https://example.com"}' | sed -n 's/.*"id":"\([^"]*\)".*/\1/p')
echo "ID=$ID"
wrk -t8 -c512 -d30s "$URL/s/$ID"
