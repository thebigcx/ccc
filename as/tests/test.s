    .section .text
    .type main, func
    .global main
main:
    movzx u8 -4(%rbp), %rax
    mov $str, %rdi
    call $puts
    ret

    .section .data
str:
    .str "Hello, Earth!\n"
