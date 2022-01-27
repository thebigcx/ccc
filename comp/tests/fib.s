	.section .rodata
L0:
	.str "%lu\n"
	.section .data
	.section .text
	.global main
main:
	push %rbp
	mov %rsp, %rbp
	sub $24, %rsp
	mov $0, %r8
	mov %r8, -8(%rbp)
	mov $1, %r8
	mov %r8, -16(%rbp)
L2:
	mov -16(%rbp), %r8
	mov $1000000, %r9
	cmp %r9, %r8
	setl %al
	movzx %al, %r10
	test %r10, %r10
	jz $L3
	mov $L0, %r8
	mov %r8, %rdi
	mov -16(%rbp), %r8
	mov %r8, %rsi
	xor %rax, %rax
	call $printf
	mov -16(%rbp), %r8
	mov %r8, -24(%rbp)
	mov -16(%rbp), %r8
	mov -8(%rbp), %r9
	add %r9, %r8
	mov %r8, -16(%rbp)
	mov -24(%rbp), %r8
	mov %r8, -8(%rbp)
	jmp $L2
L3:
L1:
	leave
	ret
