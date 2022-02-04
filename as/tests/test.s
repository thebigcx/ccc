    .section .text
    .global main
    .code16
main:
    mov %cs:(%eax), %eax
    mov $0x0e, %ah
    mov $'x', %al
loop:
    jmp $loop

    .section .data
str:
    .str "Hello, Earth!\n"
