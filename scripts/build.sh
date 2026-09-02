#!/usr/bin/env bash
# Build rvcc. Prefers cmake; falls back to a plain make/g++ build.
set -euo pipefail
cd "$(dirname "$0")/.."
if command -v cmake >/dev/null 2>&1; then
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build -j
else
    make
fi
echo "built: build/rvcc"
