	.file	"test-r.c"
	.comm	tid,8,8
	.globl	addr
	.data
	.align 8
	.type	addr, @object
	.size	addr, 8
addr:
	.quad	252645135
	.comm	ret,4,4
	.comm	dummy,4,4
	.section	.rodata
.LC0:
	.string	"Thread %u registered\n"
	.align 8
.LC1:
	.string	"Thread %u registered callback\n"
	.align 8
.LC2:
	.string	"Thread %u register callback error\n"
.LC3:
	.string	"Thread %u deregistered\n"
.LC4:
	.string	"Thread %u deregister error\n"
.LC5:
	.string	"Thread %u register error\n"
	.text
	.globl	RegisterThread
	.type	RegisterThread, @function
RegisterThread:
.LFB2:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$32, %rsp
	movq	%rdi, -24(%rbp)
	call	register_ts_thread
	cmpl	$2, %eax
	jne	.L2
	call	pthread_self
	movq	%rax, %rsi
	movl	$.LC0, %edi
	movl	$0, %eax
	call	printf
	movl	$callback_entry, %edi
	movl	$0, %eax
	call	register_callback
	cmpl	$7, %eax
	jne	.L3
	call	pthread_self
	movq	%rax, %rsi
	movl	$.LC1, %edi
	movl	$0, %eax
	call	printf
	jmp	.L4
.L3:
	call	pthread_self
	movq	%rax, %rsi
	movl	$.LC2, %edi
	movl	$0, %eax
	call	printf
.L4:
	movl	$0, -4(%rbp)
	jmp	.L5
.L6:
	addl	$1, -4(%rbp)
.L5:
	cmpl	$4999999, -4(%rbp)
	jle	.L6
	call	deregister_ts_thread
	cmpl	$4, %eax
	jne	.L7
	call	pthread_self
	movq	%rax, %rsi
	movl	$.LC3, %edi
	movl	$0, %eax
	call	printf
	jmp	.L9
.L7:
	call	pthread_self
	movq	%rax, %rsi
	movl	$.LC4, %edi
	movl	$0, %eax
	call	printf
	jmp	.L9
.L2:
	call	pthread_self
	movq	%rax, %rsi
	movl	$.LC5, %edi
	movl	$0, %eax
	call	printf
.L9:
	movl	$dummy, %eax
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE2:
	.size	RegisterThread, .-RegisterThread
	.section	.rodata
.LC6:
	.string	"missing  'arg'"
.LC7:
	.string	"usage: prog arg"
.LC8:
	.string	"doubleopen"
.LC9:
	.string	"Device opened once."
.LC10:
	.string	"Device open error."
	.align 8
.LC11:
	.string	"Device opened twice (should never happen)."
.LC12:
	.string	"tryregister"
	.align 8
.LC13:
	.string	"cannot create register thread for error %d\n"
	.text
	.globl	main
	.type	main, @function
main:
.LFB3:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$32, %rsp
	movl	%edi, -20(%rbp)
	movq	%rsi, -32(%rbp)
	cmpl	$1, -20(%rbp)
	jg	.L12
	movl	$.LC6, %edi
	call	puts
	movl	$.LC7, %edi
	call	puts
	movl	$-1, %eax
	jmp	.L13
.L12:
	movq	-32(%rbp), %rax
	addq	$8, %rax
	movq	(%rax), %rax
	movl	$.LC8, %esi
	movq	%rax, %rdi
	call	strcmp
	testl	%eax, %eax
	jne	.L14
	call	ts_open
	movl	%eax, ret(%rip)
	movl	ret(%rip), %eax
	cmpl	$-1, %eax
	je	.L15
	movl	$.LC9, %edi
	call	puts
	jmp	.L16
.L15:
	movl	$.LC10, %edi
	call	puts
.L16:
	call	ts_open
	movl	%eax, ret(%rip)
	movl	ret(%rip), %eax
	cmpl	$1, %eax
	jne	.L17
	movl	$.LC11, %edi
	call	puts
	jmp	.L14
.L17:
	movl	$.LC10, %edi
	call	puts
.L14:
	movq	-32(%rbp), %rax
	addq	$8, %rax
	movq	(%rax), %rax
	movl	$.LC12, %esi
	movq	%rax, %rdi
	call	strcmp
	testl	%eax, %eax
	jne	.L18
	call	ts_open
	movl	%eax, ret(%rip)
	movl	ret(%rip), %eax
	cmpl	$-1, %eax
	je	.L19
	movl	$.LC9, %edi
	call	puts
	jmp	.L20
.L19:
	movl	$.LC10, %edi
	call	puts
.L20:
	movl	$0, -4(%rbp)
	jmp	.L21
.L23:
	movl	-4(%rbp), %eax
	cltq
	salq	$3, %rax
	addq	$tid, %rax
	movl	$0, %ecx
	movl	$RegisterThread, %edx
	movl	$0, %esi
	movq	%rax, %rdi
	call	pthread_create
	movl	%eax, -8(%rbp)
	cmpl	$0, -8(%rbp)
	je	.L22
	movl	-4(%rbp), %eax
	movl	%eax, %esi
	movl	$.LC13, %edi
	movl	$0, %eax
	call	printf
	movl	$-1, %edi
	call	exit
.L22:
	addl	$1, -4(%rbp)
.L21:
	cmpl	$0, -4(%rbp)
	jle	.L23
.L18:
	movl	$3, %edi
	call	sleep
	movl	$0, %eax
.L13:
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE3:
	.size	main, .-main
	.ident	"GCC: (SUSE Linux) 4.8.3 20140627 [gcc-4_8-branch revision 212064]"
	.section	.note.GNU-stack,"",@progbits
