#!/usr/bin/env bash
# 从仓库根编译并运行 Day01 + Day06。在 Git Bash / w64devkit 下执行：
#   bash week1-cpp-basics/day07-review/week1_smoke.sh
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

CXXFLAGS=(-std=c++17 -Wall -Wextra)
OUT="${TMPDIR:-/tmp}"
mkdir -p "$OUT"

echo "== day01 MyString =="
g++ "${CXXFLAGS[@]}" -o "$OUT/day01_test" \
  week1-cpp-basics/day01-mystring/MyString.cpp \
  week1-cpp-basics/day01-mystring/main.cpp
"$OUT/day01_test"

echo "== day06 RingBuffer =="
g++ "${CXXFLAGS[@]}" -I week1-cpp-basics/day06-ringbuffer \
  -o "$OUT/day06_test" \
  week1-cpp-basics/day06-ringbuffer/main.cpp
"$OUT/day06_test"

echo "week1 smoke: day01 + day06 OK"
