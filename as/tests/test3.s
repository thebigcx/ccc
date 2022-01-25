    .section .text
    .global main
main:
    mov $str, %rdi
    call puts
    ret

    .section .data
str:
    .string "Hello, world!\n"
