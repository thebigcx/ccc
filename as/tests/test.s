    .section .text
    .global main
main:
    mov $str, %rdi
    call $puts
    ret

    .section .data
str:
    .str "Hello, Earth!\n"
