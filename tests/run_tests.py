#!/usr/bin/env python3
"""
rvcc differential test harness.

For every tests/cases/*.c program (each carrying a `// expect: <int>` line), this
verifies that four independently produced exit codes agree:

  1. host       - the same C compiled and run natively (the ground-truth oracle)
  2. rvcc+sim   - rvcc -> .s -> assemble/link -> run on the RISC-V simulator
  3. gcc+sim    - the same C compiled by the RISC-V gcc, same runtime, same sim
  4. expect     - the value declared in the source comment

Any disagreement fails the test. This is the correctness backbone: rvcc's codegen
is differentially checked against a production compiler and a native reference.

Simulator: defaults to qemu-system-riscv32 on the 'virt' machine (used in CI).
The identical ELF also runs on Spike or a custom rv32im core by swapping the exit
adapter in runtime/targets/ (see docs/codegen.md); those targets are run by the
user and are not exercised here.

Environment overrides:
  RVCC          path to the rvcc binary            (default: ../build/rvcc)
  RISCV_PREFIX  cross-tool prefix                  (auto-detected)
  QEMU          simulator binary                   (default: qemu-system-riscv32)
"""
import os, re, shutil, subprocess, sys, tempfile
from pathlib import Path

ROOT      = Path(__file__).resolve().parent.parent
CASES     = ROOT / "tests" / "cases"
RUNTIME   = ROOT / "runtime"
RVCC      = Path(os.environ.get("RVCC", ROOT / "build" / "rvcc"))
QEMU      = os.environ.get("QEMU", "qemu-system-riscv32")
SPIKE     = os.environ.get("SPIKE", "spike")
USE_SPIKE = bool(int(os.environ.get("USE_SPIKE", "0"))) and shutil.which(SPIKE) is not None
MARCH, MABI = "rv32im", "ilp32"

def detect_prefix():
    if os.environ.get("RISCV_PREFIX"):
        return os.environ["RISCV_PREFIX"]
    for p in ("riscv32-unknown-elf-", "riscv64-unknown-elf-", "riscv64-linux-gnu-", "riscv32-linux-gnu-"):
        if shutil.which(p + "gcc") or shutil.which(p + "as"):
            return p
    sys.exit("error: no RISC-V toolchain found; set RISCV_PREFIX")

PREFIX = detect_prefix()
AS, LD, GCC = PREFIX + "as", PREFIX + "ld", PREFIX + "gcc"
OBJDUMP = PREFIX + "objdump"

def run(cmd, **kw):
    return subprocess.run(cmd, capture_output=True, text=True, **kw)

def expected_of(src: str):
    m = re.search(r"//\s*expect:\s*(-?\d+)", src)
    if not m:
        raise ValueError("no `// expect: <int>` annotation")
    return int(m.group(1)) & 0xFF

def host_exit(cfile: Path, d: Path) -> int:
    exe = d / "host.out"
    r = run(["cc", str(cfile), "-o", str(exe)])
    if r.returncode != 0:
        raise RuntimeError("host cc failed: " + r.stderr)
    return run([str(exe)]).returncode & 0xFF

def assemble(src_asm: Path, obj: Path):
    r = run([AS, f"-march={MARCH}", f"-mabi={MABI}", str(src_asm), "-o", str(obj)])
    if r.returncode != 0:
        raise RuntimeError(f"assemble {src_asm.name} failed: {r.stderr}")

def link(objs, elf: Path):
    r = run([LD, "-m", "elf32lriscv", "-T", str(RUNTIME / "link.ld"),
             *map(str, objs), "-o", str(elf)])
    if r.returncode != 0:
        raise RuntimeError("link failed: " + r.stderr)

def qemu_exit(elf: Path) -> int:
    r = run([QEMU, "-machine", "virt", "-bios", "none", "-nographic",
             "-kernel", str(elf)], timeout=10)
    return r.returncode & 0xFF

def build_runtime_objs(d: Path):
    crt0 = d / "crt0.o"; exit_o = d / "exit.o"
    assemble(RUNTIME / "crt0.S", crt0)
    assemble(RUNTIME / "targets" / "exit_sifive.S", exit_o)
    return [crt0, exit_o]

def rvcc_sim_exit(cfile: Path, d: Path, rt_objs) -> int:
    asm = d / "main.s"; obj = d / "main.o"; elf = d / "rvcc.elf"
    r = run([str(RVCC), str(cfile), "-o", str(asm)])
    if r.returncode != 0:
        raise RuntimeError("rvcc failed: " + r.stderr)
    assemble(asm, obj)
    link([*rt_objs, obj], elf)
    return qemu_exit(elf)

def gcc_sim_exit(cfile: Path, d: Path, rt_objs) -> int:
    obj = d / "gccmain.o"; elf = d / "gcc.elf"
    r = run([GCC, f"-march={MARCH}", f"-mabi={MABI}", "-nostdlib",
             "-ffreestanding", "-O0", "-c", str(cfile), "-o", str(obj)])
    if r.returncode != 0:
        raise RuntimeError("riscv gcc failed: " + r.stderr)
    link([*rt_objs, obj], elf)
    return qemu_exit(elf)


def main_insn_count(elf) -> int:
    """Count instructions in the `main` symbol region of an ELF (via objdump)."""
    r = run([OBJDUMP, "-d", str(elf)])
    if r.returncode != 0:
        raise RuntimeError("objdump failed: " + r.stderr)
    lines = r.stdout.splitlines()
    count, in_main = 0, False
    for ln in lines:
        if ln.endswith("<main>:"):
            in_main = True; continue
        if in_main:
            # a new symbol label like "80000abc <foo>:" ends the region
            if ln.strip().endswith(":") and "<" in ln and ">" in ln and "\t" not in ln:
                break
            # instruction lines look like "   80000000:\t<hex>\t<mnemonic> ..."
            if ":\t" in ln:
                count += 1
    return count

def rvcc_elf(cfile, d, rt_objs):
    asm = d / "s.s"; obj = d / "s.o"; elf = d / "rv.elf"
    r = run([str(RVCC), str(cfile), "-o", str(asm)])
    if r.returncode != 0: raise RuntimeError("rvcc failed: " + r.stderr)
    assemble(asm, obj); link([*rt_objs, obj], elf); return elf

def gcc_elf(cfile, d, rt_objs):
    obj = d / "g.o"; elf = d / "g.elf"
    r = run([GCC, f"-march={MARCH}", f"-mabi={MABI}", "-nostdlib", "-ffreestanding",
             "-O0", "-c", str(cfile), "-o", str(obj)])
    if r.returncode != 0: raise RuntimeError("riscv gcc failed: " + r.stderr)
    link([*rt_objs, obj], elf); return elf

def report_sizes():
    cases = sorted(CASES.glob("*.c"))
    print(f"static instruction count in main() -- rvcc (naive) vs gcc -O0")
    print(f"{'case':22} {'rvcc':>6} {'gcc':>6} {'ratio':>7}")
    print("-" * 45)
    tot_rv = tot_gcc = 0
    with tempfile.TemporaryDirectory() as td:
        d = Path(td); rt = build_runtime_objs(d)
        for c in cases:
            try:
                rv = main_insn_count(rvcc_elf(c, d, rt))
                gc = main_insn_count(gcc_elf(c, d, rt))
                tot_rv += rv; tot_gcc += gc
                print(f"{c.name:22} {rv:6} {gc:6} {rv/gc:6.2f}x")
            except Exception as e:
                print(f"{c.name:22} {'--':>6} {'--':>6}   ERROR: {e}")
    print("-" * 45)
    if tot_gcc:
        print(f"{'TOTAL':22} {tot_rv:6} {tot_gcc:6} {tot_rv/tot_gcc:6.2f}x")
    print("\nNote: rvcc uses a naive stack machine (no register allocation, no")
    print("optimisation). Larger output is expected and is reported, not hidden.")


def build_htif_objs(d: Path):
    crt0 = d / "crt0h.o"; exit_o = d / "exith.o"
    assemble(RUNTIME / "crt0.S", crt0)
    assemble(RUNTIME / "targets" / "exit_htif.S", exit_o)
    return [crt0, exit_o]

def spike_exit(cfile: Path, d: Path, rt_objs) -> int:
    """Run the rvcc-compiled program on Spike (HTIF exit). Second ISA reference.
    Enabled with USE_SPIKE=1 when `spike` is installed; not run in this repo's CI."""
    asm = d / "sp.s"; obj = d / "sp.o"; elf = d / "spike.elf"
    r = run([str(RVCC), str(cfile), "-o", str(asm)])
    if r.returncode != 0: raise RuntimeError("rvcc failed: " + r.stderr)
    assemble(asm, obj); link([*rt_objs, obj], elf)
    r = run([SPIKE, "--isa=rv32im", str(elf)], timeout=15)
    return r.returncode & 0xFF

def main():
    cases = sorted(CASES.glob("*.c"))
    if not cases:
        sys.exit("no test cases found in " + str(CASES))
    sim = f"{QEMU}" + (f" + {SPIKE}" if USE_SPIKE else "")
    print(f"toolchain: {PREFIX}*   simulator: {sim}   ({MARCH}/{MABI})")
    spk_hdr = f"{'spike':>6}" if USE_SPIKE else ""
    print(f"{'case':22} {'expect':>6} {'host':>6} {'rvcc':>6} {'gcc':>6} {spk_hdr}  result")
    print("-" * (60 + (7 if USE_SPIKE else 0)))
    passed = failed = 0
    with tempfile.TemporaryDirectory() as td:
        d = Path(td)
        rt = build_runtime_objs(d)
        rt_h = build_htif_objs(d) if USE_SPIKE else None
        for c in cases:
            src = c.read_text()
            try:
                exp  = expected_of(src)
                host = host_exit(c, d)
                rv   = rvcc_sim_exit(c, d, rt)
                gcc  = gcc_sim_exit(c, d, rt)
                ok = (exp == host == rv == gcc)
                spk_col = ""
                if USE_SPIKE:
                    spk = spike_exit(c, d, rt_h)
                    ok = ok and (spk == exp)
                    spk_col = f"{spk:6}"
                print(f"{c.name:22} {exp:6} {host:6} {rv:6} {gcc:6} {spk_col}  {'PASS' if ok else 'FAIL'}")
                passed += ok; failed += (not ok)
            except Exception as e:
                print(f"{c.name:22} {'--':>6} {'--':>6} {'--':>6} {'--':>6}  ERROR: {e}")
                failed += 1
    print("-" * 60)
    print(f"{passed} passed, {failed} failed, {len(cases)} total")
    sys.exit(1 if failed else 0)

if __name__ == "__main__":
    if "--sizes" in sys.argv:
        report_sizes()
    else:
        main()
