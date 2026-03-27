# Build & Run Guide

This project implements a minimal SIP Proxy and SIP Client with a Windows UI.
It supports basic SIP call flow (REGISTER, INVITE, BYE) over UDP.

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

Manual:

```
./sip_proxy
```

CMake build:

```
./build/bin/sip_proxy
```

---

## Run SIP Proxy using Script

You can also use the helper script:

```
./scripts/run_proxy.sh
```

This script:

* Compiles the proxy
* Runs it immediately

---

## Build SIP Client

```
cmake -B build
cmake --build build
```

---

## Run SIP Client (Manual)

CLI:

```
./build/bin/sip_client 127.0.0.1 5060 --cli
```

UI:

```
./build/bin/sip_client 127.0.0.1 5060 --ui
```

---

## Run SIP Client using Script

Use the helper script:

```
./scripts/run_client.sh [mode] [ip] [port]
```
### Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `[mode]` | `--cli` | `--cli` runs a terminal interface. `--ui` launches the Qt graphical window. |
| `[ip]` | `127.0.0.1` | IP address of the **SIP Proxy** to connect to. |
| `[port]` | `5060` | UDP port the **SIP Proxy** is listening on. Must match the proxy's configured port. |



### Examples

Default (CLI):

```
./scripts/run_client.sh
```

CLI explicitly:

```
./scripts/run_client.sh --cli
```

UI mode:

```
./scripts/run_client.sh --ui
```

Custom server:

```
./scripts/run_client.sh --ui 192.168.1.10 5060
```

### What the script does:

* Creates `build/` if missing
* Builds the project using CMake
* Runs the client in selected mode (CLI/UI)

---

## Run Stress Test

```
rm -rf build/
cmake -B build
cmake --build build
sudo ./scripts/stress_test.sh
and then enter your password
```

---

## Example Scenario

Terminal 1:

```
./sip_proxy
```

Terminal 2:
Run client A (alice)

Terminal 3:
Run client B (bob)

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

### Port 5060 already in use

The proxy failed to bind to port `5060` on startup.

**Possible fixes:**

* Stop the process currently using the port:

  ```
  sudo lsof -i :5060
  sudo kill -9 <PID>
  ```
* Or change the proxy’s listening port in your configuration/code.

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

* Check firewall settings (UDP traffic on port 5060 must be allowed)
* Ensure the client is sending to the correct IP and port
* Verify the proxy is bound to the correct network interface

---

## Troubleshooting SIP Client

### No response from proxy

The client sends requests but receives no reply.

**Possible fixes:**

* Ensure the SIP Proxy is running
* Verify IP and port:

  ```
  ./scripts/run_client.sh --cli 127.0.0.1 5060
  ```
* Check firewall settings (UDP must not be blocked)

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
* Verify usernames/IDs are correct
* Check proxy logs for routing issues

---

