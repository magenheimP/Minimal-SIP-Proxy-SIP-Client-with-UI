# Build & Run Guide

This project implements a minimal SIP Proxy and SIP Client with a Windows UI.
It supports basic SIP call flow (REGISTER, INVITE, BYE) over **UDP and TCP**.

The SIP Proxy includes a **metrics endpoint** (HTTP on port 8080) for real-time monitoring.

---

## Prerequisites

* C++20 compatible compiler (GCC 11+, Clang 14+, MSVC)
* CMake 3.20+
* Linux (for SIP Proxy)
* Windows/Linux (for SIP Client)
* Qt library (for UI)
* Git

---

## Clone Repository

```
git clone https://github.com/magenheimP/Minimal-SIP-Proxy-SIP-Client-with-UI.git
cd Minimal-SIP-Proxy-SIP-Client-with-UI
```

---

## Build SIP Proxy (Linux)

Manual build:

```
g++ -std=c++20 -pthread \
apps/sip_proxy_main.cpp \
proxy/src/*.cpp \
networking/src/*.cpp \
common/src/*.cpp \
-I./common/include \
-I./proxy/include \
-I./networking/include \
-o sip_proxy
```

Or using CMake:

```
cmake -B build
cmake --build build
```

---

## Run SIP Proxy (Linux)

**UDP only (default, port 5060):**

```
./sip_proxy
```

**UDP + TCP (ports 5060 and 5061):**

```
./sip_proxy --tcp-port 5061
```

**Custom ports:**

```
./sip_proxy --udp-port 5060 --tcp-port 5062
```

CMake build:

```
./build/bin/sip_proxy
./build/bin/sip_proxy --tcp-port 5061
```

**Note:** The metrics endpoint automatically starts on **port 8080** when the proxy starts.

---

## Run SIP Proxy using Script

You can also use the helper script:

```
./scripts/run_proxy.sh              # UDP only
./scripts/run_proxy.sh --tcp        # UDP + TCP (TCP on port 5061)
./scripts/run_proxy.sh --tcp-port 5062   # UDP + TCP on a custom port
```

This script:

* Compiles the proxy
* Runs it with the selected transport(s) immediately
* Metrics endpoint is available at http://localhost:8080

---

## Metrics Endpoint

### Overview

The SIP Proxy exposes real-time operational metrics via an HTTP endpoint on **port 8080**.

### Available Metrics

| Metric | Type | Description |
|--------|------|-------------|
| `SIP_messages_received` | counter | Total SIP messages received by the proxy |
| `SIP_messages_sent` | counter | Total SIP messages sent by the proxy |
| `SIP_active_calls` | gauge | Current number of active calls |
| `SIP_registered_users` | gauge | Current number of registered users |

### Accessing Metrics

**Using curl:**

```bash
curl http://localhost:8080
```

**Using web browser:**

Open in browser: `http://localhost:8080`

**Example output:**

```
SIP_messages_received counter
SIP_messages_received 142
SIP_messages_sent counter
SIP_messages_sent 138
SIP_active_calls gauge
SIP_active_calls 2
SIP_registered_users gauge
SIP_registered_users 3
```

### Monitoring in Real-Time

**Watch metrics continuously:**

```bash
watch -n 1 curl -s http://localhost:8080
```

**Create a monitoring script:**

```bash
#!/bin/bash
# monitor.sh

while true; do
    clear
    echo "=== SIP Proxy Metrics ==="
    echo "Time: $(date)"
    echo ""
    curl -s http://localhost:8080
    echo ""
    echo "========================="
    sleep 2
done
```

Make it executable and run:

```bash
chmod +x monitor.sh
./monitor.sh
```

### Integration with Monitoring Tools

**Prometheus integration (example):**

```yaml
# prometheus.yml
scrape_configs:
  - job_name: 'sip-proxy'
    static_configs:
      - targets: ['localhost:8080']
    scrape_interval: 5s
```

**Note:** Current format is plain text. For Prometheus integration, the output format would need to be converted to Prometheus exposition format.

---

## Build SIP Client

```
cmake -B build
cmake --build build
```

---

## Run SIP Client (Manual)

**UDP mode (default):**

```
./build/bin/sip_client 127.0.0.1 5060 --cli
./build/bin/sip_client 127.0.0.1 5060 --ui
```

**TCP mode:**

```
./build/bin/sip_client 127.0.0.1 5061 --cli --tcp
./build/bin/sip_client 127.0.0.1 5061 --ui --tcp
```

---

## Run SIP Client using Script

Use the helper script:

```
./scripts/run_client.sh [--tcp] [mode] [ip] [port]
```

### Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `--tcp` | *(absent)* | Enable TCP transport. When present, the default port changes to `5061`. Omit for UDP (default port `5060`). |
| `[mode]` | `--cli` | `--cli` runs a terminal interface. `--ui` launches the Qt graphical window. |
| `[ip]` | `127.0.0.1` | IP address of the **SIP Proxy** to connect to. |
| `[port]` | `5060` (UDP) / `5061` (TCP) | UDP or TCP port the **SIP Proxy** is listening on. Must match the proxy's configured port. |

### Examples

Default (UDP CLI):

```
./scripts/run_client.sh
```

UDP UI mode:

```
./scripts/run_client.sh --ui
```

TCP CLI mode:

```
./scripts/run_client.sh --tcp
```

TCP UI mode:

```
./scripts/run_client.sh --tcp --ui
```

Custom server (UDP):

```
./scripts/run_client.sh --ui 192.168.1.10 5060
```

Custom server (TCP):

```
./scripts/run_client.sh --tcp --ui 192.168.1.10 5061
```

### What the script does:

* Creates `build/` if missing
* Builds the project using CMake
* Runs the client in selected mode (CLI/UI) over the selected transport (UDP/TCP)

---

## Run Stress Test

**UDP (default):**

```
rm -rf build/
cmake -B build
cmake --build build
sudo ./scripts/stress_test.sh
```

**TCP:**

```
rm -rf build/
cmake -B build
cmake --build build
sudo ./scripts/stress_test.sh --tcp
```

The `--tcp` flag starts the proxy with `--tcp-port 5061` and passes `--tcp` to every client. Results are saved to a directory named `stress_test_results_<timestamp>_TCP` (or `_UDP`) inside `scripts/`.

**Monitoring during stress test:**

In a separate terminal, monitor metrics in real-time:

```bash
watch -n 1 curl -s http://localhost:8080
```

This allows you to observe:
* Message throughput (`SIP_messages_received` and `SIP_messages_sent` counters increasing)
* Peak concurrent calls (`SIP_active_calls` gauge)
* Registered users count

---

## Example Scenario

Terminal 1 — Start proxy:

```
./sip_proxy                     # UDP
./sip_proxy --tcp-port 5061     # TCP
```

Terminal 2 — Monitor metrics:

```
watch -n 1 curl -s http://localhost:8080
```

Terminal 3 — Run client A (alice):

```
./scripts/run_client.sh --cli           # UDP
./scripts/run_client.sh --tcp --cli     # TCP
```

Terminal 4 — Run client B (bob):

```
./scripts/run_client.sh --cli           # UDP
./scripts/run_client.sh --tcp --cli     # TCP
```

### Steps:

1. Alice registers → Observe `SIP_registered_users` increase to 1
2. Bob registers → Observe `SIP_registered_users` increase to 2
3. Alice calls Bob → Observe `SIP_active_calls` increase to 1
4. Bob answers → Call established
5. Alice hangs up → Observe `SIP_active_calls` decrease to 0
6. Bob receives end of call
7. Bob exits the client → Observe `SIP_registered_users` decrease to 1
8. Alice exits the client → Observe `SIP_registered_users` decrease to 0

---

## Troubleshooting SIP Proxy

### Port 5060 (or 5061) already in use

The proxy failed to bind to its port on startup.

**Possible fixes:**

* Stop the process currently using the port:

  ```
  sudo lsof -i :5060
  sudo lsof -i :5061
  sudo kill -9 <PID>
  ```
* Or specify a different port when starting the proxy.

---

### Port 8080 (metrics) already in use

The metrics endpoint failed to start.

**Possible fixes:**

* Stop the process using port 8080:

  ```
  sudo lsof -i :8080
  sudo kill -9 <PID>
  ```
* Note: The proxy will continue to function for SIP traffic even if metrics endpoint fails to start.

---

### Proxy fails to start

The executable runs but exits immediately or shows errors.

**Possible fixes:**

* Ensure all required source files are compiled
* Rebuild the project:

  ```
  rm -rf build/
  cmake -B build
  cmake --build build
  ```
* Verify compiler supports **C++20**

---

### No incoming messages

The proxy is running but not receiving SIP messages.

**Possible fixes:**

* Check firewall settings (UDP traffic on port 5060 and TCP traffic on port 5061 must be allowed)
* Ensure the client transport matches the proxy transport (`--tcp` on both, or neither)
* Verify the proxy is bound to the correct network interface

---

### Metrics endpoint not responding

Cannot access http://localhost:8080

**Possible fixes:**

* Verify the proxy started successfully (check console output for "[Metrics] Server running on port 8080")
* Check if port 8080 is blocked by firewall:

  ```
  sudo ufw status
  sudo ufw allow 8080
  ```
* Try accessing from the local machine first before remote access
* Check if another service is using port 8080

---

## Troubleshooting SIP Client

### No response from proxy

The client sends requests but receives no reply.

**Possible fixes:**

* Ensure the SIP Proxy is running with the matching transport
* Verify IP, port, and transport flag match:

  ```
  ./scripts/run_client.sh --cli 127.0.0.1 5060        # UDP
  ./scripts/run_client.sh --tcp --cli 127.0.0.1 5061  # TCP
  ```
* Check firewall settings
* Use metrics endpoint to verify proxy is receiving messages:

  ```
  curl http://localhost:8080
  # Check if SIP_messages_received is increasing
  ```

---

### Client fails to start

The client does not launch or crashes immediately.

**Possible fixes:**

* Rebuild the project:

  ```
  rm -rf build/
  cmake -B build
  cmake --build build
  ```
* Ensure all dependencies are installed (especially Qt for UI mode)

---

### UI mode not working

The client runs in CLI but fails in UI mode.

**Possible fixes:**

* Verify Qt is installed:

  ```
  sudo apt install qtbase5-dev
  ```
* Ensure you are running in an environment that supports GUI (not pure SSH without X11)

---

### Call setup fails (INVITE issues)

Calls are not established between clients.

**Possible fixes:**

* Ensure both clients are registered before calling
* Verify both clients use the same transport (`--tcp` on both, or neither)
* Verify usernames/IDs are correct
* Check proxy logs for routing issues
* Monitor metrics to see if messages are being sent/received:

  ```
  curl http://localhost:8080
  ```

---

### TCP connection refused

Client prints a connection error when using `--tcp`.

**Possible fixes:**

* Confirm the proxy was started with `--tcp-port <port>`
* Confirm the client port matches: `./scripts/run_client.sh --tcp --cli 127.0.0.1 5061`
* Check that no firewall rule is blocking TCP on that port

---

## Metrics Usage Examples

### Example 1: Basic Monitoring

```bash
# Start proxy
./sip_proxy --tcp-port 5061

# In another terminal, watch metrics
watch -n 1 curl -s http://localhost:8080
```

### Example 2: Performance Testing

```bash
# Run stress test
sudo ./scripts/stress_test.sh --tcp

# In another terminal, monitor metrics continuously
while true; do
    echo "=== $(date) ==="
    curl -s http://localhost:8080 | grep -E "SIP_messages|SIP_active_calls"
    echo ""
    sleep 1
done
```

### Example 3: Logging Metrics to File

```bash
# Create monitoring script
cat > log_metrics.sh << 'EOF'
#!/bin/bash
while true; do
    timestamp=$(date +"%Y-%m-%d %H:%M:%S")
    metrics=$(curl -s http://localhost:8080 | tr '\n' ' ')
    echo "$timestamp | $metrics" >> metrics.log
    sleep 5
done
EOF

chmod +x log_metrics.sh
./log_metrics.sh
```

### Example 4: Alert on High Active Calls

```bash
#!/bin/bash
# alert_high_calls.sh

THRESHOLD=10

while true; do
    active_calls=$(curl -s http://localhost:8080 | grep "SIP_active_calls [0-9]" | awk '{print $2}')
    
    if [ "$active_calls" -gt "$THRESHOLD" ]; then
        echo "ALERT: High active calls: $active_calls"
        # Send notification (email, Slack, etc.)
    fi
    
    sleep 10
done
```

---

## Additional Notes

* The metrics endpoint runs independently of SIP traffic and does not impact proxy performance
* Metrics are stored in memory using atomic counters (thread-safe)
* The metrics server automatically stops when the proxy shuts down
* Current implementation uses plain text format; can be extended to support Prometheus format
* Counters (`SIP_messages_received`, `SIP_messages_sent`) are cumulative and never decrease
* Gauges (`SIP_active_calls`, `SIP_registered_users`) reflect current state and can increase or decrease