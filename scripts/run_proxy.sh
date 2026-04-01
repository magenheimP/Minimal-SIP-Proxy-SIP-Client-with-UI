#!/bin/bash

# Usage:
#   ./scripts/run_proxy.sh              # UDP only (default, port 5060)
#   ./scripts/run_proxy.sh --tcp        # UDP + TCP (ports 5060 and 5061)
#   ./scripts/run_proxy.sh --tcp-port 5062   # UDP + TCP on a custom port

set -e

UDP_PORT=5060
TCP_PORT=0
TCP_ARGS=""

# Parse arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        --tcp)
            TCP_PORT=5061
            shift
            ;;
        --tcp-port)
            TCP_PORT="$2"
            shift 2
            ;;
        *)
            echo "[ERROR] Unknown argument: $1"
            echo "Usage: $0 [--tcp] [--tcp-port <port>]"
            exit 1
            ;;
    esac
done

if [ "$TCP_PORT" -gt 0 ] 2>/dev/null; then
    TCP_ARGS="--tcp-port ${TCP_PORT}"
fi

echo "[INFO] Building SIP Proxy..."

if [ ! -d "build" ]; then
    echo "[INFO] Creating build directory..."
    cmake -B build
fi

cmake --build build

PROXY_BIN="./build/bin/sip_proxy"

echo "[INFO] Starting SIP Proxy..."
if [ -n "$TCP_ARGS" ]; then
    echo "[INFO] Transport: UDP port ${UDP_PORT} + TCP port ${TCP_PORT}"
    "$PROXY_BIN" --udp-port "${UDP_PORT}" ${TCP_ARGS}
else
    echo "[INFO] Transport: UDP port ${UDP_PORT} only"
    "$PROXY_BIN" --udp-port "${UDP_PORT}"
fi
