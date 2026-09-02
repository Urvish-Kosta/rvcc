// rvcc output for fib.c  (recursion; RV32 calling convention)
    .text
    .globl fib
fib:
    addi sp, sp, -16
    sw ra, 12(sp)
    sw s0, 8(sp)
    addi s0, sp, 16
    sw a0, -12(s0)
    lw a0, -12(s0)
    addi sp, sp, -16
    sw a0, 0(sp)
    li a0, 2
    mv a1, a0
    lw a0, 0(sp)
    addi sp, sp, 16
    slt a0, a0, a1
    beqz a0, .Lendif1
    lw a0, -12(s0)
    lw ra, -4(s0)
    lw t1, -8(s0)
    mv sp, s0
    mv s0, t1
    ret
.Lendif1:
    lw a0, -12(s0)
    addi sp, sp, -16
    sw a0, 0(sp)
    li a0, 1
    mv a1, a0
    lw a0, 0(sp)
    addi sp, sp, 16
    sub a0, a0, a1
    addi sp, sp, -16
    sw a0, 0(sp)
    lw a0, 0(sp)
    addi sp, sp, 16
    call fib
    addi sp, sp, -16
    sw a0, 0(sp)
    lw a0, -12(s0)
    addi sp, sp, -16
    sw a0, 0(sp)
    li a0, 2
    mv a1, a0
    lw a0, 0(sp)
    addi sp, sp, 16
    sub a0, a0, a1
    addi sp, sp, -16
    sw a0, 0(sp)
    lw a0, 0(sp)
    addi sp, sp, 16
    call fib
    mv a1, a0
    lw a0, 0(sp)
    addi sp, sp, 16
    add a0, a0, a1
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
    .globl main
main:
    addi sp, sp, -16
    sw ra, 12(sp)
    sw s0, 8(sp)
    addi s0, sp, 16
    li a0, 10
    addi sp, sp, -16
    sw a0, 0(sp)
    lw a0, 0(sp)
    addi sp, sp, 16
    call fib
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
