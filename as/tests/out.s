    .section .rodata
L0:
    .str "%d\n"
    .section .data
    .section .text
func:
    push %rbp
    mov %rsp, %rbp
    sub $4, %rsp
    mov %edi, -4(%rbp)
    mov $40, %r8
    movsx u32 -4(%rbp), %r9
    imul %r9, %r8
    mov %r8d, %eax
    jmp $L1
L1:
    leave
    ret

    .global main
main:
    push %rbp
    mov %rsp, %rbp
    sub $24, %rsp
    mov %edi, -4(%rbp)
    mov %rsi, -12(%rbp)
    mov $10, %r8
    mov %r8d, -16(%rbp)
    mov $100, %r8
    mov %r8d, -20(%rbp)
    mov $20, %r8
    mov %r8d, -24(%rbp)
    mov %r8, %rdi
    mov $10, %r8
    mov %r8d, %edi
    call $func
    mov %rax, %r8
    mov $L0, %rdi
    mov %r8d, %esi
    xor %rax, %rax
    call $printf
L2:
    leave
    ret
