    .code16
    .section .text
label:
    jmp $label
    add %eax, (%ebx)
