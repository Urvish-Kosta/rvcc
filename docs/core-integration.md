# Running rvcc output on Spike or a custom RV32IM core

rvcc emits bare-metal RV32IM. The *only* target-specific piece is how a program
signals "done, here is the exit code" — factored into a swappable exit adapter in
`runtime/targets/`. Pick the adapter that matches your simulator/core; everything
else (compiler, crt0, linker script) is identical.

## Choose an exit adapter

| Your target | Adapter | How it halts |
|---|---|---|
| QEMU `virt` (default, CI) | `exit_sifive.S` | writes `(code<<16)\|0x3333` to the SiFive test finisher at `0x100000`. **Verified in CI.** |
| Spike / riscv-tests-style core | `exit_htif.S` | writes `(code<<1)\|1` to the 64-bit `tohost` symbol (HTIF). Artifact validated (symbols present, correct encoding); run by you. |
| Core that traps `ecall` | `exit_ecall.S` | `a7=93; ecall`; your testbench reads `a0` and stops. Template — adjust to your trap handler. |

If your core halts some other way (e.g. writing the result to a magic MMIO
address the testbench watches), copy one of the adapters and replace the halt
sequence — that is the whole change.

## Build an ELF for your core

```bash
scripts/run_on_core.sh path/to/prog.c prog.elf     # uses the HTIF adapter
```

This produces `prog.elf` with `tohost`/`fromhost` symbols. Then run it on your
lockstep harness, e.g. `../riscv-rv32im-core/obj_dir/Vtop +elf=prog.elf`, and
compare the retired result against what the differential harness already
confirmed on QEMU and gcc.

## Add Spike as a second reference in the test harness

If you have `spike` installed, the differential harness can run every program on
Spike too (via `exit_htif.S`) and fold it into the agreement check:

```bash
USE_SPIKE=1 python3 tests/run_tests.py
```

Each case then must agree five ways — declared / native host / rvcc-on-QEMU /
gcc-on-QEMU / rvcc-on-Spike. (This repo's CI runs QEMU only; the Spike column is
opt-in because Spike is not installed in CI.)

## Why one ELF runs everywhere

`sp` is kept 16-byte aligned at every instruction, the image is linked at
`0x80000000` (both QEMU `virt` and Spike default DRAM base), and the entry is
`_start` in `crt0.S`. Only the final store that signals completion differs, and
that is exactly what the adapter isolates.
