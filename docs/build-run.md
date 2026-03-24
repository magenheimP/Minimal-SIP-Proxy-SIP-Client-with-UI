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
git clone <repo-url>
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
./scripts/stress_test.sh
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
7. Bob exits
8. Alice exits

---

## Troubleshooting

* **Port 5060 already in use**
  → Kill the process or change port

* **No response from proxy**
  → Check firewall / IP

* **Build fails**
  → Ensure CMake version is correct
  → Ensure Qt is installed (`qtbase5-dev`)

---
