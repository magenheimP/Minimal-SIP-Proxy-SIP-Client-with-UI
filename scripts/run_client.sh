#!/bin/bash

# Usage:
#   ./scripts/run_client.sh                              # UDP CLI, proxy at 127.0.0.1:5060
#   ./scripts/run_client.sh --ui                         # UDP UI mode
#   ./scripts/run_client.sh --tcp                        # TCP CLI, proxy at 127.0.0.1:5061
#   ./scripts/run_client.sh --tcp --ui                   # TCP UI mode
#   ./scripts/run_client.sh --ui 192.168.1.10 5060       # Custom proxy IP/port (UDP)
#   ./scripts/run_client.sh --tcp --ui 192.168.1.10 5061 # Custom proxy IP/port (TCP)
#
# Parameters (positional, after flags):
#   [mode]   --cli | --ui   (default: --cli)
#   [ip]     proxy IP       (default: 127.0.0.1)
#   [port]   proxy port     (default: 5060 for UDP, 5061 for TCP)

set -e

MODE="--cli"
USE_TCP=false
TCP_FLAG=""
SERVER_IP=""
SERVER_PORT=""

# Parse all flags first, collect positional args separately
POSITIONAL=()
while [[ $# -gt 0 ]]; do
    case "$1" in
        --tcp)
            USE_TCP=true
            TCP_FLAG="--tcp"
            shift
            ;;
        --cli|--ui)
            MODE="$1"
            shift
            ;;
        *)
            POSITIONAL+=("$1")
            shift
            ;;
    esac
done

# Assign positional args: [ip] [port]
SERVER_IP="${POSITIONAL[0]:-127.0.0.1}"

if [ -n "${POSITIONAL[1]:-}" ]; then
    SERVER_PORT="${POSITIONAL[1]}"
elif [ "$USE_TCP" = true ]; then
    SERVER_PORT=5061
else
    SERVER_PORT=5060
fi

TRANSPORT_LABEL="UDP"
[ "$USE_TCP" = true ] && TRANSPORT_LABEL="TCP"

echo "[INFO] Config:"
echo "       Mode:      $MODE"
echo "       Transport: $TRANSPORT_LABEL"
echo "       Server:    $SERVER_IP:$SERVER_PORT"

# Build if needed
if [ ! -d "build" ]; then
    echo "[INFO] Creating build directory..."
    cmake -B build
fi

echo "[INFO] Building project..."
cmake --build build

echo "[INFO] Running SIP Client..."

if [ "$MODE" = "--ui" ]; then
    ./build/bin/sip_client "$SERVER_IP" "$SERVER_PORT" --ui $TCP_FLAG
else
    ./build/bin/sip_client "$SERVER_IP" "$SERVER_PORT" --cli $TCP_FLAG
fi
