#!/usr/bin/env bash
################################################################################
# Memory Leak Test Script
# Purpose: Test memory leaks in entire project using Valgrind
# Requirements:
#   - sip_proxy must be built
#   - sip_client must be built
################################################################################

set -euo pipefail

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Paths
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/cmake-build-debug"
PROXY_BIN="${BUILD_DIR}/bin/sip_proxy"
CLIENT_BIN="${BUILD_DIR}/bin/sip_client"
RESULTS_DIR="${SCRIPT_DIR}/memory_test_results_$(date +%Y%m%d_%H%M%S)"
TEST_LOG="${RESULTS_DIR}/test.log"

mkdir -p "${RESULTS_DIR}"


log_info() {
    echo -e "${BLUE}[INFO]${NC} $*" | tee -a "${TEST_LOG}"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $*" | tee -a "${TEST_LOG}"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $*" | tee -a "${TEST_LOG}"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $*" | tee -a "${TEST_LOG}"
}

cleanup() {
    log_info "Cleaning up processes..."
    if [ -n "${PROXY_PID:-}" ] && kill -0 "$PROXY_PID" 2>/dev/null; then
        kill -TERM "$PROXY_PID" 2>/dev/null || true
        wait $PROXY_PID 2>/dev/null || true
    fi
    log_info "Cleanup complete"
}

trap cleanup EXIT INT TERM

check_binaries() {
    log_info "Checking binaries..."

    if [ ! -f "$PROXY_BIN" ]; then
        log_error "sip_proxy not found at $PROXY_BIN"
        log_error "Build first: cmake --build cmake-build-debug --target sip_proxy"
        exit 1
    fi

    if [ ! -f "$CLIENT_BIN" ]; then
        log_error "sip_client not found at $CLIENT_BIN"
        log_error "Build first: cmake --build cmake-build-debug --target sip_client"
        exit 1
    fi

    if ! command -v valgrind &> /dev/null; then
        log_error "Valgrind not found"
        log_error "Install: sudo apt install valgrind"
        exit 1
    fi

    log_success "All binaries found"
}

run_valgrind_proxy() {
    log_info "========================================="
    log_info "Running Valgrind on Proxy"
    log_info "========================================="

    local valgrind_proxy_log="${RESULTS_DIR}/valgrind_proxy.log"

    printf "\n" | valgrind --leak-check=full \
             --show-leak-kinds=all \
             --track-origins=yes \
             "$PROXY_BIN" > "$valgrind_proxy_log" 2>&1 &

    PROXY_PID=$!
    log_info "Proxy started under Valgrind, PID: $PROXY_PID"
    sleep 5

    if kill -0 $PROXY_PID 2>/dev/null; then
        kill -SIGINT $PROXY_PID
    fi
    wait $PROXY_PID 2>/dev/null || true

    if grep -q "All heap blocks were freed" "$valgrind_proxy_log"; then
        log_success "No memory leaks in proxy"
    else
        log_error "Memory leaks detected in proxy, check $valgrind_proxy_log"
    fi

    if grep -q "ERROR SUMMARY: 0 errors" "$valgrind_proxy_log"; then
        log_success "No memory errors in proxy"
    else
        log_error "Memory errors in proxy, check $valgrind_proxy_log"
    fi

    cat "$valgrind_proxy_log" | tee -a "${TEST_LOG}"
}

run_valgrind_client() {
    log_info "========================================="
    log_info "Running Valgrind on Client"
    log_info "========================================="

    local valgrind_client_log="${RESULTS_DIR}/valgrind_client.log"

    "$PROXY_BIN" &
    PROXY_PID=$!
    log_info "Proxy started for client test, PID: $PROXY_PID"
    sleep 2

    valgrind --leak-check=full \
             --show-leak-kinds=all \
             --track-origins=yes \
             "$CLIENT_BIN" 127.0.0.1 5060 --cli > "$valgrind_client_log" 2>&1 &

    local valgrind_pid=$!
    sleep 5

    if kill -0 $valgrind_pid 2>/dev/null; then
        kill -SIGINT $valgrind_pid
    fi
    wait $valgrind_pid 2>/dev/null || true

    if kill -0 $PROXY_PID 2>/dev/null; then
        kill -SIGINT $PROXY_PID
    fi
    wait $PROXY_PID 2>/dev/null || true
    unset PROXY_PID

    if grep -q "All heap blocks were freed" "$valgrind_client_log"; then
        log_success "No memory leaks in client"
    else
        log_error "Memory leaks detected in client, check $valgrind_client_log"
    fi

    if grep -q "ERROR SUMMARY: 0 errors" "$valgrind_client_log"; then
        log_success "No memory errors in client"
    else
        log_error "Memory errors in client, check $valgrind_client_log"
    fi

    cat "$valgrind_client_log" | tee -a "${TEST_LOG}"
}

generate_report() {
    local report_file="${RESULTS_DIR}/summary_report.txt"

    cat > "$report_file" <<EOF
================================================================================
MEMORY LEAK TEST REPORT
================================================================================
Test Date: $(date)

Tests Run:
  - Valgrind on sip_proxy
  - Valgrind on sip_client

Logs:
  - Proxy Valgrind Log:  ${RESULTS_DIR}/valgrind_proxy.log
  - Client Valgrind Log: ${RESULTS_DIR}/valgrind_client.log
  - Full Test Log:       ${TEST_LOG}

================================================================================
EOF

    cat "$report_file"
    log_info "Full report saved to: $report_file"
}

main() {
    log_info "========================================="
    log_info "MEMORY LEAK TEST"
    log_info "========================================="

    check_binaries
    run_valgrind_proxy
    run_valgrind_client
    generate_report

    log_success "========================================="
    log_success "MEMORY LEAK TEST COMPLETED"
    log_success "Results saved to: $RESULTS_DIR"
    log_success "========================================="
}

main "$@"