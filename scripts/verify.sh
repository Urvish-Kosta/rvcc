#!/usr/bin/env bash
# One command that runs the whole M0 verification story:
# rvcc-compiled programs vs native host vs RISC-V gcc, all diffed on the simulator.
set -euo pipefail
cd "$(dirname "$0")/.."
[ -x build/rvcc ] || ./scripts/build.sh
python3 tests/run_tests.py
