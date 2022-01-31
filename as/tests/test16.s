    .code16
    .section .text
label:
    jmp $label
    mov $label, %ax
