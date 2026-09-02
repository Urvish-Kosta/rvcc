# exit_htif.S — exit adapter for HTIF (Spike, and riscv-tests-style custom-core
# testbenches). Writing (code << 1) | 1 to the 64-bit `tohost` symbol signals
# program termination with exit status == code.
#
# NOTE: use this adapter for Spike and for your rv32im-core IF its testbench
# implements the HTIF tohost/fromhost protocol (most riscv-tests-derived rigs
# do). Confirm your core's convention before relying on it — it has NOT been
# verified in this repo's CI, only the SiFive/QEMU path has.
    .section .text, "ax"
    .globl __rvcc_exit
__rvcc_exit:                      # void __rvcc_exit(int code)  [code in a0]
    slli    a0, a0, 1
    ori     a0, a0, 1
    la      t0, tohost
    sw      a0, 0(t0)             # low word (rv32: tohost is 64-bit)
    sw      zero, 4(t0)           # high word
1:  j       1b

    .section .tohost, "aw", @progbits
    .align 6
    .globl tohost
tohost:   .dword 0
    .align 6
    .globl fromhost
fromhost: .dword 0
