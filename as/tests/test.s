    .section .text
    .global main
    .code16
main:
    mov %cs:(%eax), %eax
    mov $0x0e, %ah

    .section .data
str:
    .str "Hello, Earth!\n"
