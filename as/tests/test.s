    .section .text
    .global main
main:
    mov $str, %rdi
    call $puts
    mov 10, %rax
    lgdt u64 10(%rax,%rbx,2)
    lidt u64 100
    ret

    .section .data
str:
    .str "Hello, Earth!\n"
