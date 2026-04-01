# Build & Run Guide

This project implements a minimal SIP Proxy and SIP Client with a Windows UI.
It supports basic SIP call flow (REGISTER, INVITE, BYE) over **UDP and TCP**.

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

---

## Example Scenario

Terminal 1 — Start proxy:

```
./sip_proxy                     # UDP
./sip_proxy --tcp-port 5061     # TCP
```

Terminal 2 — Run client A (alice):

```
./scripts/run_client.sh --cli           # UDP
./scripts/run_client.sh --tcp --cli     # TCP
```

Terminal 3 — Run client B (bob):

```
./scripts/run_client.sh --cli           # UDP
./scripts/run_client.sh --tcp --cli     # TCP
```

### Steps:

1. Alice registers
2. Bob registers
3. Alice calls Bob
4. Bob answers
5. Alice hangs up
6. Bob receives end of call
7. Bob exits the client
8. Alice exits the client

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

---

### TCP connection refused

Client prints a connection error when using `--tcp`.

**Possible fixes:**

* Confirm the proxy was started with `--tcp-port <port>`
* Confirm the client port matches: `./scripts/run_client.sh --tcp --cli 127.0.0.1 5061`
* Check that no firewall rule is blocking TCP on that port
