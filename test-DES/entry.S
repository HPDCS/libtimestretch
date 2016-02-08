.file "entry.S"
.extern ECS_stub
.text
.globl callback_entry
.type callback_entry,@function
callback_entry:
	push %rbp
	mov  %rsp, %rbp	
	push %rax
	push %rbx
	push %rcx
	push %rdx
	push %rdi
	push %rsi
	push %r8
	push %r9
	push %r10
	push %r11
	push %r12
	push %r13
	push %r14
	push %r15
	mov  0x10(%rbp), %rdi	
	mov  0x8(%rbp), %rsi	
	call ECS_stub
	pop %r15
	pop %r14
	pop %r13
	pop %r12
	pop %r11
	pop %r10
	pop %r9
	pop %r8
	pop %rsi
	pop %rdi
	pop %rdx
	pop %rcx
	pop %rbx
	pop %rax
	leaveq
#	add $0x10, %rsp
	retq
