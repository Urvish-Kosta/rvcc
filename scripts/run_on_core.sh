#!/usr/bin/env bash
# run_on_core.sh — compile a C file with rvcc and build a bare-metal ELF for the
# author's rv32im core (or Spike), using the HTIF exit adapter. Then hand the ELF
# to your core's simulator / lockstep harness.
#
# Usage:  scripts/run_on_core.sh path/to/prog.c [out.elf]
#
# Edit CORE_RUN below to invoke your Verilator testbench, e.g.
#   CORE_RUN="../riscv-rv32im-core/obj_dir/Vtop +elf=$ELF"
# Most riscv-tests-derived harnesses already implement HTIF tohost/fromhost, so
# the ELF built here should run unmodified. If your core halts differently, swap
# runtime/targets/exit_htif.S for exit_ecall.S (ecall) or your own adapter.
set -euo pipefail
cd "$(dirname "$0")/.."
PREFIX="${RISCV_PREFIX:-riscv64-unknown-elf-}"
command -v "${PREFIX}as" >/dev/null 2>&1 || PREFIX="riscv64-linux-gnu-"
SRC="${1:?usage: run_on_core.sh prog.c [out.elf]}"
ELF="${2:-prog.elf}"
[ -x build/rvcc ] || ./scripts/build.sh

M="-march=rv32im -mabi=ilp32"
./build/rvcc "$SRC" -o /tmp/_m.s
"${PREFIX}as" $M /tmp/_m.s                       -o /tmp/_m.o
"${PREFIX}as" $M runtime/crt0.S                  -o /tmp/_crt0.o
"${PREFIX}as" $M runtime/targets/exit_htif.S     -o /tmp/_exit.o
"${PREFIX}ld" -m elf32lriscv -T runtime/link.ld /tmp/_crt0.o /tmp/_m.o /tmp/_exit.o -o "$ELF"
echo "built $ELF (HTIF exit; tohost/fromhost symbols present)"

# --- point this at your core's harness or Spike ---
# spike --isa=rv32im "$ELF"; echo "spike exit = $?"
# CORE_RUN="../riscv-rv32im-core/obj_dir/Vtop +elf=$ELF"; eval "$CORE_RUN"
echo "next: run '$ELF' on your core's lockstep harness (see docs/core-integration.md)"
