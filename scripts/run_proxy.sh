#!/bin/bash

g++ -std=c++20 -pthread \
apps/sip_proxy_main.cpp \
proxy/src/*.cpp \
networking/src/*.cpp \
common/src/*.cpp \
-I./common/include \
-I./proxy/include \
-I./networking/include \
-o sip_proxy

./sip_proxy