// rvcc output for factorial.c  (for-loop, unique labels; naive stack machine)
    .text
    .globl main
main:
    addi sp, sp, -16
    sw ra, 12(sp)
    sw s0, 8(sp)
    addi s0, sp, 16
    li a0, 1
    sw a0, -12(s0)
    li a0, 1
    sw a0, -16(s0)
.Lfor0:
    lw a0, -16(s0)
    addi sp, sp, -4
    sw a0, 0(sp)
    li a0, 5
    mv a1, a0
    lw a0, 0(sp)
    addi sp, sp, 4
    slt a0, a1, a0
    xori a0, a0, 1
    beqz a0, .Lendfor1
    lw a0, -12(s0)
    addi sp, sp, -4
    sw a0, 0(sp)
    lw a0, -16(s0)
    mv a1, a0
    lw a0, 0(sp)
    addi sp, sp, 4
    mul a0, a0, a1
    sw a0, -12(s0)
    lw a0, -16(s0)
    addi sp, sp, -4
    sw a0, 0(sp)
    li a0, 1
    mv a1, a0
    lw a0, 0(sp)
    addi sp, sp, 4
    add a0, a0, a1
    sw a0, -16(s0)
    j .Lfor0
.Lendfor1:
    lw a0, -12(s0)
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
