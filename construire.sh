#!/usr/bin/env bash
# g++ seul. `make` fait la meme chose quand il est installe.
set -e; cd "$(dirname "$0")"; mkdir -p bin
${CXX:-g++} -std=c++20 -O2 -Wall -Wextra -Wpedantic -o bin/emotion-renderer src/main.cpp
echo "bin/emotion-renderer"
