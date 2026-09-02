# crt0.S — bare-metal startup for rvcc-compiled programs.
# Responsibilities (deliberately minimal for M0):
#   1. Establish a stack pointer.
#   2. Call main().
#   3. Pass main's return value (in a0) to the target-specific __rvcc_exit.
# The exit mechanism itself is factored into a swappable adapter under
# runtime/targets/ so the same crt0 works on QEMU, Spike, or a custom core.
    .section .text.init, "ax"
    .globl _start
_start:
    la      sp, __stack_top      # top of the stack region (see link.ld)
    call    main                 # int main(void); result in a0
    call    __rvcc_exit          # never returns
1:  j       1b                   # safety net
