# exit_sifive.S — exit adapter for the QEMU 'virt' machine (SiFive test finisher).
# Writing (code << 16) | 0x3333 to the finisher MMIO register terminates the
# emulator with exit status == code (verified for codes 0..255).
# This is the adapter used by the local verification harness and CI.
    .equ SIFIVE_TEST, 0x100000
    .section .text, "ax"
    .globl __rvcc_exit
__rvcc_exit:                      # void __rvcc_exit(int code)  [code in a0]
    slli    a0, a0, 16
    li      t0, 0x3333
    or      a0, a0, t0
    li      t1, SIFIVE_TEST
    sw      a0, 0(t1)
1:  j       1b
