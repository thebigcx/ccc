	.section .rodata
L0:
	.str "x == 10\n"
L1:
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
	jmp $L2
L2:
	leave
	ret
	.global main
main:
	push %rbp
	mov %rsp, %rbp
	sub $28, %rsp
	mov %edi, -4(%rbp)
	mov %rsi, -12(%rbp)
	mov $10, %r8
	mov %r8d, -16(%rbp)
	mov $100, %r8
	mov %r8d, -20(%rbp)
	mov $20, %r8
	mov %r8d, -24(%rbp)
	movsx u32 -16(%rbp), %r8
	test %r8, %r8
	jz $L4
	movsx u32 -16(%rbp), %r8
	mov $10, %r9
	cmp %r9, %r8
	setz %al
	movzx %al, %r10
	test %r10, %r10
	jz $L4
	mov $L0, %r8
	mov %r8, %rdi
	xor %rax, %rax
	call $printf
L4:
	mov $10, %r8
	mov %r8d, %edi
	call $func
	mov %rax, %r8
	mov %r8d, -28(%rbp)
	mov $L1, %r8
	mov %r8, %rdi
	movsx u32 -28(%rbp), %r8
	mov %r8d, %esi
	xor %rax, %rax
	call $printf
L3:
	leave
	ret
