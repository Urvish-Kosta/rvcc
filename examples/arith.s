// rvcc output for arith.c  (2 + 3*4 - 10/2 == 9; naive stack machine)
    .text
    .globl main
main:
    li a0, 2
    addi sp, sp, -4
    sw a0, 0(sp)
    li a0, 3
    addi sp, sp, -4
    sw a0, 0(sp)
    li a0, 4
    mv a1, a0
    lw a0, 0(sp)
    addi sp, sp, 4
    mul a0, a0, a1
    mv a1, a0
    lw a0, 0(sp)
    addi sp, sp, 4
    add a0, a0, a1
    addi sp, sp, -4
    sw a0, 0(sp)
    li a0, 10
    addi sp, sp, -4
    sw a0, 0(sp)
    li a0, 2
    mv a1, a0
    lw a0, 0(sp)
    addi sp, sp, 4
    div a0, a0, a1
    mv a1, a0
    lw a0, 0(sp)
    addi sp, sp, 4
    sub a0, a0, a1
    ret
    li a0, 0
    ret
