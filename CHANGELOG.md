# Changelog

All notable changes to this project are documented here.
Format follows [Keep a Changelog](https://keepachangelog.com/); the project uses
milestone-based scoping (see README).

## [Unreleased] - Core & Spike integration

### Added
- `runtime/targets/exit_ecall.S`: third exit adapter for cores that trap `ecall`.
- `scripts/run_on_core.sh`: build a bare-metal HTIF ELF from any C file for a
  custom core or Spike.
- `docs/core-integration.md`: how to pick an exit adapter and run rvcc output on
  Spike or a custom RV32IM core.
- Optional Spike second reference in the harness (`USE_SPIKE=1`): five-way
  agreement (declared / host / rvcc-QEMU / gcc-QEMU / rvcc-Spike). CI stays
  QEMU-only; Spike is opt-in.

### Notes
- The HTIF exit artifact is validated (symbols + `(code<<1)|1` encoding); Spike
  was not built in the development sandbox, so the Spike column is opt-in rather
  than part of CI.

## [M5] - Pointers and arrays

### Added
- Minimal type system (int, pointer-to-int, array-of-int) inferred by sema and
  annotated onto expressions.
- Address-of (`&`), dereference (`*`) as rvalue and store target, local `int`
  arrays with indexing (`a[i]` desugars to `*(a+i)`), and element-scaled pointer
  arithmetic (`p+i`, `p[i]`). Arrays decay to a base-address pointer as values.
- Generalised assignment to any lvalue (`*p = x`, `a[i] = x`).
- 11 pointer/array test cases including an in-place bubble sort capstone.

### Verified
- 82/82 programs pass with 0 divergences on qemu-system-riscv32 (rv32im/ilp32).

## [M4] - Functions and the calling convention

### Added
- Multiple functions, parameters, and calls under the RV32 calling convention
  (args in a0-a7, result in a0, ra/s0 saved). Recursion and mutual recursion
  (signatures collected first, so calls may forward-reference).
- Arity/undeclared-call checks; up to 8 params/args (more is a clear error).
- Machine-stack slots widened to 16 bytes so sp stays 16-byte aligned at every
  call site.
- 12 function/recursion test cases (fib, factorial, gcd, 8-arg, mutual).

### Verified
- 71/71 programs pass with 0 divergences at this milestone.

## [M3] - Control flow

### Added
- `if`/`else`, `while`, `for` (with optional init/cond/post), nested `{}` blocks,
  and the empty statement.
- Six comparison operators (`== != < <= > >=`) and short-circuit `&&`/`||`,
  yielding normalised 0/1.
- Lexical scoping: `sema` resolves names through a scope stack with shadowing,
  scopes `for`-init to its loop, and rejects out-of-scope use and same-scope
  redeclaration. Resolved slots are annotated onto the AST.
- Control-flow lowering with uniquely numbered labels.
- 24 control-flow test cases, including behavioural short-circuit proofs and
  scope-boundary error checks.

### Verified
- 59/59 programs pass with 0 divergences on qemu-system-riscv32 (rv32im/ilp32),
  four-way (declared / native host / rvcc-on-sim / gcc-on-sim).

## [M2] - Local variables

### Added
- Local `int` declarations (with optional initialiser), assignment as an
  expression (right-associative; `a = b = 7` and `return (x = 5)` work), variable
  references, and expression statements.
- Semantic-analysis pass (`sema`): per-function symbol table, s0-relative slot
  assignment, declare-before-use and redeclaration checks with line numbers.
- Real stack frames: prologue/epilogue with a fixed `s0` frame pointer, so locals
  are stable while the stack machine moves `sp` during evaluation.
- 12 local-variable test cases.

### Verified
- 35/35 programs pass with 0 divergences on qemu-system-riscv32 (rv32im/ilp32),
  four-way (declared / native host / rvcc-on-sim / gcc-on-sim).
- The independent host oracle caught a mis-transcribed `// expect` value in a
  test (rvcc/gcc/host agreed; the annotation was wrong), which is exactly its
  purpose — no expected value is trusted by hand.

## [M1] - Integer arithmetic

### Added
- Expression grammar: unary (`- ~ !`) and binary (`+ - * / %`) operators with C
  precedence, left-associativity, and parentheses.
- Naive stack-machine code generation (no register allocation, no constant
  folding).
- `--sizes` harness mode: static instruction-count comparison of rvcc vs
  `gcc -O0` for `main`.
- 18 arithmetic/unary test cases, including signed division/modulo with negative
  operands (verifies truncation-toward-zero matches C and gcc).

### Verified
- 23/23 programs pass with 0 divergences on qemu-system-riscv32 (rv32im/ilp32),
  four-way (declared / native host / rvcc-on-sim / gcc-on-sim).

## [M0] - Walking skeleton

### Added
- Lexer, recursive-descent parser, and naive RV32IM code generator for the
  minimal subset `int main() { return <int-literal>; }`.
- Bare-metal runtime: `crt0.S`, linker script, and two swappable exit adapters
  (SiFive/QEMU and HTIF/Spike).
- Differential test harness verifying four-way agreement (declared expectation,
  native host, rvcc-on-simulator, RISC-V gcc-on-simulator).
- Build via `make` or `cmake`; `scripts/build.sh` and `scripts/verify.sh`.

### Verified
- 5/5 M0 programs pass with 0 divergences on qemu-system-riscv32 (rv32im/ilp32).
