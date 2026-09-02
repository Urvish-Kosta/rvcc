# exit_ecall.S — exit adapter for cores/testbenches that trap `ecall` to halt.
# Convention: exit code in a0, syscall number 93 ("exit") in a7, then ecall.
# A bare-metal testbench typically traps the ecall, reads a0 as the exit status,
# and stops the clock. Adjust the syscall number or mechanism to match your core.
# NOT exercised by this repo's CI (which uses QEMU/SiFive); provided for cores
# whose harness halts on ecall.
    .section .text, "ax"
    .globl __rvcc_exit
__rvcc_exit:                      # void __rvcc_exit(int code)  [code in a0]
    li      a7, 93
    ecall
1:  j       1b
