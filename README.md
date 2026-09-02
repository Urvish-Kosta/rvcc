# rvcc

**A small C compiler that targets RV32IM, verified against gcc and a golden ISA reference.**

[![ci](https://github.com/Urvish-Kosta/rvcc/actions/workflows/ci.yml/badge.svg)](https://github.com/Urvish-Kosta/rvcc/actions/workflows/ci.yml)
[![license: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

rvcc lowers a documented subset of C to RV32IM assembly with naive stack-machine
code generation. Every compiled program is checked by **four-way differential
testing** - the value declared in the source, the same program run natively, the
program compiled by rvcc and run on a RISC-V simulator, and the program compiled
by the RISC-V `gcc` and run on the same simulator. All four must agree or the
test fails.

The compiler emits assembly text and links it into a bare-metal ELF. Because the
target-specific exit mechanism is isolated into a swappable adapter, the *same*
ELF runs on QEMU, on Spike, or on a custom RV32IM core - which is the point: rvcc
is built as the software half of a design→compile→run→verify loop with a
hand-written [RV32IM core](https://github.com/Urvish-Kosta/riscv-rv32im-core).

> **Scope, stated plainly.** This is a deliberately small compiler built in
> honestly-scoped milestones. The accepted language (**M0-M5**) covers integer
> and pointer/array expressions, local variables, control flow, and functions
> with recursion - and nothing more. Code generation is naive and unoptimised.
> See [`docs/limitations.md`](docs/limitations.md) for the
> exact boundary and [`docs/grammar.md`](docs/grammar.md) for the accepted grammar.

## Why

Most hardware portfolios stop at the RTL. rvcc closes the loop: the author's own
RV32IM core is the *target* of a compiler the author also wrote, and compiled C
is verified on that core with the same lockstep retire-trace harness used to
validate the core itself. The verification discipline - differential testing
against a production compiler rather than inspecting the output by hand - is the
core value here, not the size of the language.

## Architecture

```
C source
  -> Lexer        (hand-written tokenizer)
  -> Parser       (recursive descent -> AST)
  -> CodeGen      (naive RV32IM assembly text)
  -> GNU as + ld  (assemble + link with crt0 + exit adapter -> ELF)
  -> simulator / core
        |            |               |
     native     rvcc on sim     gcc on sim      -> four-way differential check
```

See [`docs/codegen.md`](docs/codegen.md) for the pipeline and the entry/exit
convention that lets one ELF run across simulators.

## Milestones

| | Milestone | Adds |
|---|---|---|
| ✅ | **M0** | `int main(){ return <literal>; }`; full pipeline + differential harness |
| ✅ | **M1** | integer arithmetic: unary/binary operators, precedence, parentheses |
| ✅ | **M2** | local variables, assignment, stack frame (s0 frame pointer) |
| ✅ | **M3** | `if`/`else`, `while`, `for`, nested scopes, comparisons, short-circuit `&&`/`\|\|` |
| ✅ | **M4** | functions, calls, RV32 calling convention, recursion (<=8 args) |
| ✅ | **M5** | pointers, arrays, address-of/deref, element-scaled pointer arithmetic |

The project is designed to be shippable and honest after any milestone. **M0-M5 is complete**: a small but real C compiler.

## Build

Requires a C++17 compiler. `cmake` optional.

```bash
make            # or: ./scripts/build.sh   (uses cmake if present)
```

## Usage

```bash
./build/rvcc path/to/program.c -o program.s   # emit RV32IM assembly
./build/rvcc path/to/program.c                 # or write to stdout
```

Example (`examples/ret42.c` -> `examples/ret42.s`):

```c
int main() { return 42; }
```
```asm
    .text
    .globl main
main:
    li a0, 42
    ret
```

And `2 + 3 * 4 - 10 / 2` (== 9) lowers through the naive stack machine
(`examples/arith.c` -> `examples/arith.s`) - one push/pop pair per binary
operator, no optimisation.

Local variables (`examples/vars.c` -> `examples/vars.s`) use a fixed `s0` frame
pointer so slots stay stable while the stack machine moves `sp`.

Control flow (`examples/factorial.c` -> `examples/factorial.s`) lowers to
uniquely labelled branches; `&&`/`||` short-circuit (verified behaviourally).

Functions and recursion (`examples/fib.c` -> `examples/fib.s`) follow the RV32
calling convention. A full **bubble sort** (`examples/bubble_sort.c`) exercises
arrays, nested loops, in-place swaps, and pointer parameters end to end.

## Verification

The differential harness needs a RISC-V toolchain (`gcc`/`as`/`ld`) and
`qemu-system-riscv32`. On Ubuntu:

```bash
sudo apt-get install -y gcc-riscv64-linux-gnu qemu-system-misc
./scripts/verify.sh
```

Current result (M0 - M5): **82/82 pass, 0 divergences** on qemu-system-riscv32
(rv32im/ilp32). Each case agrees four ways - declared value, native host,
rvcc-on-simulator, and RISC-V gcc-on-simulator. Sample:

```
case                   expect   host   rvcc    gcc  result
------------------------------------------------------------
m1_mixed.c                  9      9      9      9  PASS   # 2 + 3*4 - 10/2
m1_paren2.c                15     15     15     15  PASS   # (2+3)*(4-1)
m1_divneg.c               253    253    253    253  PASS   # (0-7)/2 == -3
m1_bitnot.c               255    255    255    255  PASS   # ~0 == -1
...
23 passed, 0 failed, 23 total
```

Code-size honesty check (`python3 tests/run_tests.py --sizes`): naive codegen is
larger than `gcc -O0` on real expressions and this is reported, not hidden. Note
gcc constant-folds these compile-time-constant programs, so the ratio mostly
reflects rvcc's absence of constant folding at this milestone (see
[`docs/codegen.md`](docs/codegen.md)).

Running on Spike or the author's RV32IM core is self-serve: `scripts/run_on_core.sh`
builds an ELF with the HTIF exit adapter (validated: `tohost`/`fromhost` present,
canonical `(code<<1)|1` encoding), and `USE_SPIKE=1 python3 tests/run_tests.py`
folds Spike in as a second ISA reference (five-way agreement) when `spike` is
installed. See [`docs/core-integration.md`](docs/core-integration.md). Retargeting means
selecting an exit adapter (`runtime/targets/exit_htif.S`, `exit_sifive.S` or
`exit_ecall.S`) and a linker script matching the target's memory map, then
running the identical ELF through that target - see
[`docs/codegen.md`](docs/codegen.md).

## Repository layout

```
src/        lexer, parser, AST, codegen, driver
runtime/    crt0, linker script, swappable exit adapters (sifive/htif)
tests/      differential harness + C test corpus
docs/       grammar (scope boundary), codegen/convention, limitations
examples/   committed .c -> .s pair
scripts/    build.sh, verify.sh
```

## References

- RISC-V Unprivileged ISA specification (RV32I base, M extension).
- RISC-V calling convention (psABI) - used from M4 onward.
- Companion project: [riscv-rv32im-core](https://github.com/Urvish-Kosta/riscv-rv32im-core).

## License

MIT - see [LICENSE](LICENSE).

## Author

Urvish Kosta - [github.com/Urvish-Kosta](https://github.com/Urvish-Kosta)
