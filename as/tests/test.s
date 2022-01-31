    .section .text
    .global main
main:
    mov $str, %eax
    ret

    .section .data
str:
    .str "Hello, Earth!\n"
