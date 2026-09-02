# Limitations (current: M0 - M5)

rvcc is an intentionally small, honestly-scoped compiler. It is **not** a full C
implementation and makes no optimisation claims. It compiles a coherent subset:
multiple `int`/pointer/array functions with locals, nested scopes, the full set
of arithmetic/comparison/logical operators, control flow, calls and recursion
(<=8 args), and single-level int pointers and arrays with address-of/deref.

## Deliberate non-goals
- `char` and other widths; globals and a `.data`/`.bss` section; multi-dim
  arrays; pointer-to-pointer / pointer-to-array; array initialisers; pointer
  difference; `break`/`continue`, `switch`, `?:`; bitwise `& | ^`/shifts;
  compound assignment; `++`/`--`; preprocessor; stdlib; structs/unions/enums;
  casts. Inputs outside the grammar are rejected, never silently mis-compiled.
- More than 8 parameters or arguments is a clear error (stack-passed arguments
  are a clean future extension).
- Reading an uninitialised local is undefined behaviour (as in C); the corpus
  never does it.

## Code quality
- Naive, unoptimised stack machine: every intermediate spills to memory (16-byte
  slots for alignment), no register allocation, no constant folding. Larger than
  `gcc -O0` (measured by `run_tests.py --sizes`); reported, not hidden.

## Verification scope
- CI and the harness verify on **qemu-system-riscv32** (rv32im/ilp32), four-way
  (declared / native host / rvcc-on-sim / gcc-on-sim).
- Spike and the author's rv32im core run the identical ELF via the HTIF exit
  adapter, executed by the user and **not** part of this repo's automated CI.
