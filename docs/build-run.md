# Build & Run Guide

This project implements a minimal SIP Proxy and SIP Client with a Windows UI.
It supports basic SIP call flow (REGISTER, INVITE, BYE) over UDP.

## Prerequisites

- C++20 compatible compiler (GCC 11+, Clang 14+, MSVC)
- CMake 3.20+
- Linux (for SIP Proxy)
- Windows (for SIP Client UI)
- Qt library (for UI)
- Git

## Clone Repository

git clone <repo-url>
cd Minimal-SIP-Proxy-SIP-Client-with-UI

## Build SIP Proxy (Linux)

g++ -std=c++20 -pthread \
apps/sip_proxy_main.cpp \
proxy/src/*.cpp \
networking/src/*.cpp \
common/src/*.cpp \
-I./common/include \
-I./proxy/include \
-I./networking/include \
-o sip_proxy

or

cmake -B build
cmake --build build

## Run SIP Proxy (Linux)

./sip_proxy or ./build/bin/sip_proxy

## Build SIP Client (Windows)

cmake -B build
cmake --build build

## Run SIP Client (Windows)
For Cli:
./build/bin/sip_client 127.0.0.1 5060 --cli
For UI:
./build/bin/sip_client 127.0.0.1 5060 --ui

## Run Stress test

Terminal commands:
rm -rf build/
cmake -B build
cmake --build build
./scripts/stress_test.sh

## Example Scenario

Terminal 1:
./sip_proxy

Terminal 2:
Run client A (alice)

Terminal 3:
Run client B (bob)

Steps:
1. Alice registers
2. Bob registers
3. Alice calls Bob
4. Bob answers
5. Alice hangsup
6. Bob receives end of call
7. Bob exits
8. Alice exits

## Troubleshooting

- Port 5060 already in use:
  → kill process or change port

- No response from proxy:
  → check firewall / IP

- Build fails:
  → ensure CMake version is correct