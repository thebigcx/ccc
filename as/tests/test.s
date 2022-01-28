    .section .text
    .skip 100
    .type main, func
    .global main
main:
    movzx u8 -4(%rbp), %rax
    lea str(%rip), %rdi
    call $puts
    ret

    .section .data
str:
    .str "Hello, Earth!\n"
