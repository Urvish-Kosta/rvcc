// rvcc output for vars.c  (locals via s0 frame pointer; naive stack machine)
    .text
    .globl main
main:
    addi sp, sp, -32
    sw ra, 28(sp)
    sw s0, 24(sp)
    addi s0, sp, 32
    li a0, 3
    sw a0, -12(s0)
    lw a0, -12(s0)
    addi sp, sp, -4
    sw a0, 0(sp)
    lw a0, -12(s0)
    mv a1, a0
    lw a0, 0(sp)
    addi sp, sp, 4
    mul a0, a0, a1
    sw a0, -16(s0)
    lw a0, -12(s0)
    addi sp, sp, -4
    sw a0, 0(sp)
    lw a0, -16(s0)
    mv a1, a0
    lw a0, 0(sp)
    addi sp, sp, 4
    add a0, a0, a1
    sw a0, -20(s0)
    lw a0, -20(s0)
    lw ra, -4(s0)
    lw t1, -8(s0)
    mv sp, s0
    mv s0, t1
    ret
    li a0, 0
    lw ra, -4(s0)
    lw t1, -8(s0)
    mv sp, s0
    mv s0, t1
    ret
