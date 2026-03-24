#!/bin/bash

set -e

MODE=${1:---cli}
SERVER_IP=${2:-127.0.0.1}
SERVER_PORT=${3:-5060}

echo "[INFO] Config:"
echo "       Mode: $MODE"
echo "       Server: $SERVER_IP:$SERVER_PORT"

# Build if needed
if [ ! -d "build" ]; then
    echo "[INFO] Creating build directory..."
    cmake -B build
fi

echo "[INFO] Building project..."
cmake --build build

echo "[INFO] Running SIP Client..."

if [ "$MODE" == "--ui" ]; then
    ./build/bin/sip_client $SERVER_IP $SERVER_PORT --ui
else
    ./build/bin/sip_client $SERVER_IP $SERVER_PORT --cli
fi