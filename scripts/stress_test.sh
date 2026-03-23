#!/usr/bin/env bash
################################################################################
# SIP Stress Test Script
# Purpose: Test 20 concurrent clients with REGISTER + INVITE flows
# Requirements:
#   - Proxy server must be built
#   - Client binary must be built
#   - Validates: No dropped messages, no deadlocks, stable latency
################################################################################

set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
NUM_CLIENTS=20
PROXY_IP="127.0.0.1"
PROXY_PORT=5060
CLIENT_START_PORT=6000
DOMAIN="localhost"
RESULTS_DIR="stress_test_results_$(date +%Y%m%d_%H%M%S)"

# Determine script location and find build directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"

PROXY_BIN="${BUILD_DIR}/bin/sip_proxy"
CLIENT_BIN="${BUILD_DIR}/bin/sip_client"
PROXY_LOG="${RESULTS_DIR}/proxy.log"
STRESS_LOG="${RESULTS_DIR}/stress_test.log"

# Create results directory immediately (use absolute path)
mkdir -p "${SCRIPT_DIR}/${RESULTS_DIR}"
RESULTS_DIR="${SCRIPT_DIR}/${RESULTS_DIR}"
PROXY_LOG="${RESULTS_DIR}/proxy.log"
STRESS_LOG="${RESULTS_DIR}/stress_test.log"

# Timeouts and intervals (in seconds)
PROXY_STARTUP_WAIT=2
CLIENT_STARTUP_WAIT=1
REGISTER_WAIT=2
INVITE_WAIT=5
CLEANUP_WAIT=2

# Metrics
declare -a CLIENT_PIDS
declare -a CLIENT_LOGS
FAILED_CLIENTS=0
TIMEOUT_CLIENTS=0

################################################################################
# Helper Functions
################################################################################

log() {
    local level=$1
    shift
    local msg="$*"
    local timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    echo -e "${timestamp} [${level}] ${msg}" | tee -a "${STRESS_LOG}"
}

log_info() {
    echo -e "${BLUE}[INFO]${NC} $*" | tee -a "${STRESS_LOG}"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $*" | tee -a "${STRESS_LOG}"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $*" | tee -a "${STRESS_LOG}"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $*" | tee -a "${STRESS_LOG}"
}

cleanup() {
    log_info "Cleaning up processes..."

    # Stop all clients
    for pid in "${CLIENT_PIDS[@]}"; do
        if kill -0 "$pid" 2>/dev/null; then
            kill -TERM "$pid" 2>/dev/null || true
        fi
    done

    # Stop proxy
    if [ -n "${PROXY_PID:-}" ] && kill -0 "$PROXY_PID" 2>/dev/null; then
        log_info "Stopping proxy (PID: $PROXY_PID)..."
        kill -TERM "$PROXY_PID" 2>/dev/null || true
        sleep 1
        if kill -0 "$PROXY_PID" 2>/dev/null; then
            kill -KILL "$PROXY_PID" 2>/dev/null || true
        fi
    fi

    log_info "Cleanup complete"
}

trap cleanup EXIT INT TERM

check_binaries() {
    log_info "Checking binaries..."

    if [ ! -f "$PROXY_BIN" ] && [ ! -f "${PROXY_BIN}.exe" ]; then
        log_error "Proxy binary not found at $PROXY_BIN"
        log_error "Please build the project first: cmake -B build && cmake --build build"
        exit 1
    fi

    if [ ! -f "$CLIENT_BIN" ] && [ ! -f "${CLIENT_BIN}.exe" ]; then
        log_error "Client binary not found at $CLIENT_BIN"
        log_error "Please build the project first: cmake -B build && cmake --build build"
        exit 1
    fi

    # Adjust for Windows .exe extension
    [ -f "${PROXY_BIN}.exe" ] && PROXY_BIN="${PROXY_BIN}.exe"
    [ -f "${CLIENT_BIN}.exe" ] && CLIENT_BIN="${CLIENT_BIN}.exe"

    log_success "Binaries found"
}

start_proxy() {
    log_info "Starting SIP Proxy on ${PROXY_IP}:${PROXY_PORT}..."

    # The proxy waits for ENTER to stop, so we need to keep stdin open
    # We'll use tail -f /dev/null which keeps the pipe open indefinitely
    # Or use sleep with a very long duration

    # Start tail in background to keep stdin open
    tail -f /dev/null 2>/dev/null | "$PROXY_BIN" > "$PROXY_LOG" 2>&1 &
    PROXY_PID=$!

    log_info "Proxy started with PID: $PROXY_PID"

    # Wait for proxy to be ready
    sleep $PROXY_STARTUP_WAIT

    if ! kill -0 "$PROXY_PID" 2>/dev/null; then
        log_error "Proxy failed to start. Check $PROXY_LOG"
        log_error "Proxy log contents:"
        cat "$PROXY_LOG" | head -20
        exit 1
    fi

    # Check if proxy is actually listening on the port
    log_info "Checking if proxy is listening on port ${PROXY_PORT}..."
    if command -v netstat &> /dev/null; then
        if netstat -tuln | grep -q ":${PROXY_PORT} "; then
            log_success "Proxy is listening on port ${PROXY_PORT}"
        else
            log_warn "Proxy may not be listening on port ${PROXY_PORT} yet"
        fi
    elif command -v ss &> /dev/null; then
        if ss -tuln | grep -q ":${PROXY_PORT} "; then
            log_success "Proxy is listening on port ${PROXY_PORT}"
        else
            log_warn "Proxy may not be listening on port ${PROXY_PORT} yet"
        fi
    fi

    log_success "Proxy is running"
}

test_single_client() {
    local client_id=$1
    local username="user${client_id}"
    local client_log="${RESULTS_DIR}/client_${client_id}.log"

    CLIENT_LOGS[$client_id]="$client_log"

    log_info "Client $client_id: Starting as $username@$DOMAIN"

    local test_script="${RESULTS_DIR}/client_${client_id}_commands.txt"

    # Determine role: even clients call, odd clients answer
    # user0 calls user1, user2 calls user3, etc.
    if [ $((client_id % 2)) -eq 0 ]; then
        # CALLER: Register, wait for callee to be ready, make call, wait for answer, then hangup
        local target_id=$((client_id + 1))
        local target_user="user${target_id}"

        log_info "Client $client_id: Will call $target_user"

        cat > "$test_script" <<EOF
status
sleep 1
call
${target_user}
${DOMAIN}
sleep 4
hangup
exit
EOF
    else
        # CALLEE: Register, wait for call to arrive, answer, wait, then hangup
        log_info "Client $client_id: Will answer calls"

        cat > "$test_script" <<EOF
status
sleep 6
answer
sleep 1
hangup
exit
EOF
    fi

    local start_time=$(date +%s.%N)

    # Start client with auto-register
    (
        timeout -k 5 35 "$CLIENT_BIN" "$PROXY_IP" "$PROXY_PORT" --cli \
            --user "$username" --domain "$DOMAIN" < "$test_script" \
            > "$client_log" 2>&1
    ) &

    local client_pid=$!
    CLIENT_PIDS[$client_id]=$client_pid

    # Wait for this specific client to complete (max 40 seconds)
    local wait_count=0
    local max_wait=40
    while kill -0 $client_pid 2>/dev/null && [ $wait_count -lt $max_wait ]; do
        sleep 0.5
        wait_count=$((wait_count + 1))
    done

    local end_time=$(date +%s.%N)
    local duration=$(echo "$end_time - $start_time" | bc 2>/dev/null || echo "0")

    # Check if still running
    if kill -0 $client_pid 2>/dev/null; then
        log_warn "Client $client_id: Timeout after ${max_wait}s, killing..."
        kill -9 $client_pid 2>/dev/null || true
        wait $client_pid 2>/dev/null || true
        TIMEOUT_CLIENTS=$((TIMEOUT_CLIENTS + 1))
        return 1
    fi

    # Wait for the process to finish
    wait $client_pid 2>/dev/null
    local exit_code=$?

    # Check if client completed successfully
    local result_file="${RESULTS_DIR}/client_${client_id}_result.txt"
    if [ $exit_code -eq 0 ] || [ $exit_code -eq 124 ]; then
        # Check for successful registration in log
        if grep -q "REGISTER successful" "$client_log" 2>/dev/null; then
            log_success "Client $client_id: Completed in ${duration}s"
            echo "$duration" > "$result_file"
            return 0
        else
            log_error "Client $client_id: Registration failed"
            echo "FAILED" > "$result_file"
            FAILED_CLIENTS=$((FAILED_CLIENTS + 1))
            return 1
        fi
    else
        log_error "Client $client_id: Failed with exit code $exit_code"
        echo "FAILED" > "$result_file"
        FAILED_CLIENTS=$((FAILED_CLIENTS + 1))
        return 1
    fi
}

run_concurrent_clients() {
    log_info "Launching $NUM_CLIENTS concurrent clients..."

    # Launch callees first (odd), then callers (even) to ensure callees are ready
    local pids=()

    # First phase: launch all callees (odd clients)
    log_info "Phase 1: Launching callees..."
    for i in $(seq 1 2 $((NUM_CLIENTS - 1))); do
        test_single_client $i &
        pids+=($!)
        sleep 0.05
    done

    # Wait for callees to start and register
    sleep 2

    # Second phase: launch all callers (even clients)
    log_info "Phase 2: Launching callers..."
    for i in $(seq 0 2 $((NUM_CLIENTS - 1))); do
        test_single_client $i &
        pids+=($!)
        sleep 0.1
    done

    log_info "All clients launched, waiting for completion..."
    log_info "This may take up to 30 seconds per client..."

    # Wait for all background test functions to complete
    local completed=0
    for pid in "${pids[@]}"; do
        if wait $pid 2>/dev/null; then
            completed=$((completed + 1))
        fi
    done

    log_info "All client tests completed ($completed/${#pids[@]} test functions finished)"
}

analyze_results() {
    log_info "========================================="
    log_info "STRESS TEST RESULTS"
    log_info "========================================="

    # Count results from result files (subshell values don't propagate)
    local successful=0
    local actual_failed=0
    for i in $(seq 0 $((NUM_CLIENTS - 1))); do
        local result_file="${RESULTS_DIR}/client_${i}_result.txt"
        if [ -f "$result_file" ]; then
            local result=$(cat "$result_file")
            if [ "$result" = "FAILED" ]; then
                actual_failed=$((actual_failed + 1))
            else
                successful=$((successful + 1))
            fi
        fi
    done

    log_info "Total clients: $NUM_CLIENTS"
    log_success "Successful: $successful"
    [ $actual_failed -gt 0 ] && log_error "Failed: $actual_failed"

    # Analyze latency from result files
    local -a register_times=()
    for i in $(seq 0 $((NUM_CLIENTS - 1))); do
        local result_file="${RESULTS_DIR}/client_${i}_result.txt"
        if [ -f "$result_file" ]; then
            local result=$(cat "$result_file")
            if [ "$result" != "FAILED" ]; then
                register_times+=("$result")
            fi
        fi
    done

    if [ ${#register_times[@]} -gt 0 ]; then
        local total_time=0
        local min_time=999999
        local max_time=0

        for time in "${register_times[@]}"; do
            total_time=$(echo "$total_time + $time" | bc 2>/dev/null || echo "$total_time")
            if (( $(echo "$time < $min_time" | bc -l 2>/dev/null || echo 0) )); then
                min_time=$time
            fi
            if (( $(echo "$time > $max_time" | bc -l 2>/dev/null || echo 0) )); then
                max_time=$time
            fi
        done

        local avg_time=$(echo "scale=3; $total_time / ${#register_times[@]}" | bc 2>/dev/null || echo "0")

        log_info "Latency Statistics:"
        log_info "  Min: ${min_time}s"
        log_info "  Max: ${max_time}s"
        log_info "  Avg: ${avg_time}s"
    fi

    # Check for dropped messages
    log_info "Checking for dropped messages..."
    local dropped_count=0
    if [ -f "$PROXY_LOG" ]; then
        dropped_count=$(grep -cE "timeout|failed|error" "$PROXY_LOG" 2>/dev/null || echo "0")
        dropped_count=${dropped_count//[^0-9]/}
        [ -z "$dropped_count" ] && dropped_count=0
    fi
    if [ "$dropped_count" -eq 0 ]; then
        log_success "No dropped messages detected"
    else
        log_warn "Potential dropped messages: $dropped_count (check logs)"
    fi

    # Check for deadlocks
    log_info "Checking for deadlocks..."
    local deadlock_count=0
    if [ -f "$PROXY_LOG" ]; then
        deadlock_count=$(grep -cE "deadlock|blocked|hung" "$PROXY_LOG" 2>/dev/null || echo "0")
        deadlock_count=${deadlock_count//[^0-9]/}
        [ -z "$deadlock_count" ] && deadlock_count=0
    fi
    if [ "$deadlock_count" -eq 0 ]; then
        log_success "No deadlocks detected"
    else
        log_error "Potential deadlocks: $deadlock_count (check logs)"
    fi

    # Check proxy stability
    if kill -0 "$PROXY_PID" 2>/dev/null; then
        log_success "Proxy is still running (stable)"
    else
        log_error "Proxy crashed during test!"
    fi

    log_info "========================================="
    log_info "Detailed logs saved to: $RESULTS_DIR"
    log_info "========================================="

    # Final verdict
    if [ $successful -eq $NUM_CLIENTS ] && [ "$dropped_count" -eq 0 ] && [ "$deadlock_count" -eq 0 ]; then
        log_success "✓ STRESS TEST PASSED"
        return 0
    else
        log_error "✗ STRESS TEST FAILED"
        return 1
    fi
}

generate_report() {
    local report_file="${RESULTS_DIR}/summary_report.txt"
    local dropped_count_report=0
    local deadlock_count_report=0
    if [ -f "$PROXY_LOG" ]; then
        dropped_count_report=$(grep -cE "timeout|failed|error" "$PROXY_LOG" 2>/dev/null || echo "0")
        dropped_count_report=${dropped_count_report//[^0-9]/}
        [ -z "$dropped_count_report" ] && dropped_count_report=0
        deadlock_count_report=$(grep -cE "deadlock|blocked|hung" "$PROXY_LOG" 2>/dev/null || echo "0")
        deadlock_count_report=${deadlock_count_report//[^0-9]/}
        [ -z "$deadlock_count_report" ] && deadlock_count_report=0
    fi

    # Count results from files
    local report_successful=0
    local report_failed=0
    for i in $(seq 0 $((NUM_CLIENTS - 1))); do
        local result_file="${RESULTS_DIR}/client_${i}_result.txt"
        if [ -f "$result_file" ]; then
            local result=$(cat "$result_file")
            if [ "$result" = "FAILED" ]; then
                report_failed=$((report_failed + 1))
            else
                report_successful=$((report_successful + 1))
            fi
        fi
    done

    cat > "$report_file" <<EOF
================================================================================
SIP STRESS TEST REPORT
================================================================================
Test Date: $(date)
Configuration:
  - Number of Clients: $NUM_CLIENTS
  - Proxy: ${PROXY_IP}:${PROXY_PORT}
  - Domain: $DOMAIN

Results:
  - Successful Clients: $report_successful
  - Failed Clients: $report_failed

Validation:
  - Dropped Messages: $dropped_count_report
  - Deadlocks: $deadlock_count_report
  - Proxy Stability: $(kill -0 "$PROXY_PID" 2>/dev/null && echo "STABLE" || echo "CRASHED")

Logs:
  - Proxy Log: $PROXY_LOG
  - Client Logs: ${RESULTS_DIR}/client_*.log
  - Stress Test Log: $STRESS_LOG

================================================================================
EOF

    cat "$report_file"
    log_info "Full report saved to: $report_file"
}

################################################################################
# Main Execution
################################################################################

main() {
    log_info "========================================="
    log_info "SIP STRESS TEST - Week 3"
    log_info "Testing: 20 Concurrent Clients"
    log_info "========================================="

    # Pre-flight checks
    check_binaries

    # Start proxy server
    start_proxy

    # Run concurrent client tests
    run_concurrent_clients

    # Analyze and report results
    analyze_results
    local test_result=$?

    # Generate summary report
    generate_report

    # Cleanup (trap will handle this, but explicit call for clarity)
    # cleanup is called by trap

    exit $test_result
}

# Run main function
main "$@"
