// rvcc output for ret42.c (RV32IM, naive codegen)
    .text
    .globl main
main:
    li a0, 42
    ret
    li a0, 0
    ret
