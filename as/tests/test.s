    .section .text
    .global main
    .code16
main:
    mov %cs:(%eax), %eax

    .section .data
str:
    .str "Hello, Earth!\n"
