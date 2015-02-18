#include <stdio.h>

int counter = 0;

void ECS_stub(long long A, long long B){

	counter++;
	printf("overtick interrupt - params are %d - %d  -- counter is %d\n", A, B, counter); 
//	fflush(stdout);
//	while(1);
	
} 

void audit(void){

	printf("ECS: counter audit is %d\n", counter); 


}

/*
void overtick_callback(void) { // for now a simple printf of the cross-state dependency involved LP

	unsigned long __id = -1;
	void *__addr  = NULL;
	unsigned long __pgd_ds = -1;


	__asm__ __volatile__("push %rax");
	__asm__ __volatile__("push %rbx");
	__asm__ __volatile__("push %rcx");
	__asm__ __volatile__("push %rdx");
	__asm__ __volatile__("push %rdi");
	__asm__ __volatile__("push %rsi");
	__asm__ __volatile__("push %r8");
	__asm__ __volatile__("push %r9");
	__asm__ __volatile__("push %r10");
	__asm__ __volatile__("push %r11");
	__asm__ __volatile__("push %r12");
	__asm__ __volatile__("push %r13");
	__asm__ __volatile__("push %r14");
	__asm__ __volatile__("push %r15");
	//__asm__ __volatile__("movq 0x10(%%rbp), %%rax; movq %%rax, %0" : "=m"(addr) : );
	//__asm__ __volatile__("movq 0x8(%%rbp), %%rax; movq %%rax, %0" : "=m"(id) : );

	__asm__ __volatile__("movq 0x18(%%rbp), %%rax; movq %%rax, %0" : "=m"(__addr) : );
	__asm__ __volatile__("movq 0x10(%%rbp), %%rax; movq %%rax, %0" : "=m"(__id) : );
	__asm__ __volatile__("movq 0x8(%%rbp), %%rax; movq %%rax, %0" : "=m"(__pgd_ds) : );
	
	printf("rootsim callback received by the kernel need to sync, pointer passed is %p - hitted object is %u - pdg is %u\n", __addr, __id, __pgd_ds);
	ECS_stub((int)__pgd_ds,(unsigned int)__id);
	__asm__ __volatile__("pop %r15");
	__asm__ __volatile__("pop %r14");
	__asm__ __volatile__("pop %r13");
	__asm__ __volatile__("pop %r12");
	__asm__ __volatile__("pop %r11");
	__asm__ __volatile__("pop %r10");
	__asm__ __volatile__("pop %r9");
	__asm__ __volatile__("pop %r8");
	__asm__ __volatile__("pop %rsi");
	__asm__ __volatile__("pop %rdi");
	__asm__ __volatile__("pop %rdx");
	__asm__ __volatile__("pop %rcx");
	__asm__ __volatile__("pop %rbx");
	__asm__ __volatile__("pop %rax");
//	__asm__ __volatile__("addq $0x18 , %rsp ; popq %rbp ;  addq $0x8, %rsp ; retq");
//	__asm__ __volatile__("addq $0x20 , %rsp ;  popq %rbp; addq $0x10 , %rsp ; retq");
//	__asm__ __volatile__("addq $0x28 , %rsp ; popq %rbx ; popq %rbp; addq $0x10 , %rsp ; retq"); // BUONA CON LA PRINTF
//	__asm__ __volatile__("addq $0x50 , %rsp ; popq %rbx ; popq %rbp; addq $0x10 , %rsp ; retq"); // BUONA SENZA PRINTF
}
*/
