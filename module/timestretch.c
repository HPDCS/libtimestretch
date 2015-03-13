/**		      Copyright (C) 2014-2015 HPDCS Group
*		       http://www.dis.uniroma1.it/~hpdcs
* 
* This is free software; 
* You can redistribute it and/or modify this file under the
* terms of the GNU General Public License as published by the Free Software
* Foundation; either version 3 of the License, or (at your option) any later
* version.
* 
* This file is distributed in the hope that it will be useful, but WITHOUT ANY
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
* A PARTICULAR PURPOSE. See the GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License along with
* this file; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
* 
* @file timestretch.c 
* @brief This is the main source for the Linux Kernel Module which implements
*	 the timestrech module for modifying the quantum assigned to a process.
* @author Francesco Quaglia
* @author Alessandro Pellegrini
*
* @date February 12, 2015
*/
#define EXPORT_SYMTAB
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/kprobes.h>
#include <linux/mutex.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <asm/page.h>
#include <asm/cacheflush.h>
#include <asm/apic.h>

// This gives access to read_cr0() and write_cr0()
#if LINUX_VERSION_CODE > KERNEL_VERSION(3,3,0)
    #include <asm/switch_to.h>
#else
    #include <asm/system.h>
#endif
#ifndef X86_CR0_WP
#define X86_CR0_WP 0x00010000
#endif

// This macro was added in kernel 3.5
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,5,0)
    #define APIC_EOI_ACK 0x0 /* Docs say 0 for future compat. */
#endif


#include "timestretch.h"

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,25)
#error Unsupported Kernel Version
#endif

#define DEBUG if(0)
#define DEBUG_APIC if(0)

#define OVERTICK_SCALING_FACTOR 10 //please set a multiple of 2
#define OVERTICK_COMPENSATION_FACTOR (OVERTICK_SCALING_FACTOR / 2)//give at least twice overtick-time for APIC interrupts occurring in kernle mode 
#define COMPENSATE ((OVERTICK_SCALING_FACTOR / OVERTICK_COMPENSATION_FACTOR) - 1)

static unsigned int stretch_flag[MAX_CPUs] = {[0 ... MAX_CPUs-1] 0}; 
static int CPU_overticks[MAX_CPUs] = {[0 ... MAX_CPUs-1] 0}; 

// These variables are set up by configure
unsigned int *original_calibration = (int *)ORIGINAL_CALIBRATION;
void (*my__setup_APIC_LVTT)(unsigned int, int, int) =  (void *)SETUP_APIC_LVTT;
int Hz = KERNEL_HZ;

unsigned char *apic_timer_interrupt_start = (unsigned char *)APIC_TIMER_INTERRUPT; // this is the address starting from which we need to find the 'ret' pattern (0xe8) of the top half apic interrupt
//the below variables are function pointers to activate non-exported kernel symbols
void (*local_apic_timer_interrupt)(void) = (void *)LOCAL_APIC_TIMER_INTERRUPT;
void (*my_exit_idle)(void) = (void *)EXIT_IDLE;
void (*my_irq_enter)(void) = (void *)IRQ_ENTER;
void (*my_irq_exit)(void) = (void *)IRQ_EXIT;


unsigned char APIC_bytes_to_redirect[5];
unsigned char APIC_bytes_to_restore[5];
unsigned char *APIC_restore_addr = NULL;

static int bytes_to_patch_in_scheduler = 4;




/** FUNCTION PROTOTYPES
 * All the functions in this module must be listed here with a high alignment
 * and explicitly handled in prepare_self_patch(), so as to allow a correct self-patching
 * independently of the order of symbols emitted by the linker, which changes
 * even when small changes to the code are made.
 * Failing to do so, could prevent self-patching and therefore module usability.
 * This approach does not work only if scheduler_hook is the last function of the module,
 * or if the padding becomes smaller than 4 bytes...
 * In both cases, only a small change in the code (even placing a nop) could help doing
 * the job!
 */
void my_smp_apic_timer_interrupt(struct pt_regs*);
static void *prepare_self_patch(void);
static void time_stretch_cleanup(void);
static int time_stretch_init(void);
static void scheduler_unpatch(void);
static int scheduler_patch(void);
static void print_bytes(char *str, unsigned char *ptr, size_t s);
static int check_patch_compatibility(void);
static void scheduler_hook(void);
static long time_stretch_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
int time_stretch_release(struct inode *inode, struct file *filp);
int time_stretch_open(struct inode *inode, struct file *filp);
void timer_reset(void*);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alessandro Pellegrini <pellegrini@dis.uniroma1.it>, Francesco Quaglia <quaglia@dis.uniroma1.it>");
MODULE_DESCRIPTION("On demand stretch of the scheduling quantum");
module_init(time_stretch_init);
module_exit(time_stretch_cleanup);


/* MODULE VARIABLES */
static DEFINE_MUTEX(ts_thread_register);
static int ts_threads[TS_THREADS]={[0 ... (TS_THREADS-1)] = -1};
static int last_passage[TS_THREADS]={[0 ... (TS_THREADS-1)] = 0};

static unsigned int overtick_count[TS_THREADS]={[0 ... (TS_THREADS-1)] = 0};
static unsigned long callback[TS_THREADS]={[0 ... (TS_THREADS-1)] = 0x0};


static ulong watch_dog=0;
static DEFINE_MUTEX(ts_mutex);
static int base_time_interrupt;
static int major;
static struct class *dev_cl = NULL; // Device class being created
static struct device *device = NULL; // Device being created
static unsigned int time_cycles;
static flags *control_buffer;
static DEVICE_ATTR(multimap, S_IRUSR|S_IRGRP|S_IROTH, NULL, NULL);
static unsigned char finish_task_switch_original_bytes[6];
void *finish_task_switch = (void *)FTS_ADDR;
void *finish_task_switch_next = (void *)FTS_ADDR_NEXT;

/* File operations for the module */
struct file_operations fops = {
	open:		time_stretch_open,
	unlocked_ioctl:	time_stretch_ioctl,
	compat_ioctl:	time_stretch_ioctl, // Nothing strange is passed, so 32 bits programs should work out of the box
	release:	time_stretch_release
};


int enabled_registering = 0;


//This is copied by the kernel (in structure) and augmented with audit capabilities - however,the orignal calls (of unexported functions) are replaced via function pointers

int apic_interrupt_watch_dog= 0;
 
void my_smp_apic_timer_interrupt(struct pt_regs* regs) {

 	int i, target;
	unsigned long auxiliary_stack_pointer;
	unsigned long flags;
	unsigned int stretch_cycles;

        struct pt_regs *old_regs = set_irq_regs(regs);

	/*
	 * NOTE! We'd better ACK the irq immediately,
	 * because timer handling can be slow.
	 *
	 * update_process_times() expects us to have done irq_enter().
	 * Besides, if we don't timer interrupts ignore the global
	 * interrupt lock, which is the WrongThing (tm) to do.
	 */

	// entering_ack_irq();
	//this is replaced by the below via function pointers
	//apic->eoi_write(APIC_EOI, APIC_EOI_ACK);//eoi_write unavailable, we fall back to write
	apic->write(APIC_EOI, APIC_EOI_ACK);
	my_irq_enter();
	my_exit_idle();

/*	apic_interrupt_watch_dog++;//no problem if we do not perform this atomically, its just a periodic audit
	if (apic_interrupt_watch_dog >= 0x000000000000ffff){
		printk(KERN_DEBUG "%s: watch dog trigger for smp apic timer interrrupt %d CPU-id is %d\n", KBUILD_MODNAME, current->pid, smp_processor_id());
		apic_interrupt_watch_dog = 0;
	}
*/
	if(current->mm == NULL) goto normal_APIC_interrupt;  /* this is a kernel thread */

	//for normal (non-overtick) scenarios we only pay the extra cost of the below cycle
	for(i = 0; i < TS_THREADS; i++){

		if(ts_threads[i] == current->pid){
			DEBUG_APIC
			printk(KERN_INFO "%s: found APIC registered thread %d on CPU %d - overtick count is %d - return instruction is at address %p\n", KBUILD_MODNAME, current->pid,smp_processor_id(),overtick_count[i],regs->ip);

			target = i;
			goto overtick_APIC_interrupt;
		}	

	} // end for

normal_APIC_interrupt:

	//still based on function pointers
	if(stretch_flag[smp_processor_id()] == 0 || CPU_overticks[smp_processor_id()]<=0){
		local_apic_timer_interrupt();
	}
	my_irq_exit();
	set_irq_regs(old_regs);
	

	//again - no additional cost (except for the predicate evaluation) in non-overtick scenarios
	if(stretch_flag[smp_processor_id()] == 1){
		local_irq_save(flags);
		CPU_overticks[smp_processor_id()] = 0;
		stretch_flag[smp_processor_id()] = 0;
		stretch_cycles = (*original_calibration) ; 
ENABLE 		my__setup_APIC_LVTT(stretch_cycles, 0, 1);
   		local_irq_restore(flags);
	}

	return;

overtick_APIC_interrupt:

	//if(overtick_count[target] <= 0){// '<' should be redundant
	if( CPU_overticks[smp_processor_id()] <= 0 ){// '<' should be redundant
		CPU_overticks[smp_processor_id()] = OVERTICK_SCALING_FACTOR;
        	local_apic_timer_interrupt();
	}
	else{
		CPU_overticks[smp_processor_id()] -= 1;
	}

        my_irq_exit();
        set_irq_regs(old_regs);

	if( old_regs != NULL ){//interrupted while in kernel mode running
		goto overtick_APIC_interrupt_kernel_mode;
	}

	if((regs->ip & 0xf000000000000000 || regs->ip >= 0x7f0000000000) ){//interrupted while in kernel mode running

		goto overtick_APIC_interrupt_kernel_mode;
	}

	if(callback[target] != NULL && !(regs->ip & 0xf000000000000000) ){//check 1) callback existence and 2) no kernel mode running upon APIC timer interrupt

		auxiliary_stack_pointer = regs->sp;
		auxiliary_stack_pointer -= sizeof(regs->ip);
		//printk("stack management information : reg->sp is %p - auxiliary sp is %p\n",regs->sp,auxiliary_stack_pointer);
       	 	copy_to_user((void *)auxiliary_stack_pointer,(void *)&regs->ip, sizeof(regs->ip));	
//		auxiliary_stack_pointer--;
//       	 copy_to_user((void*)auxiliary_stack_pointer,(void*)&current->pid,8);	
//		auxiliary_stack_pointer--;
 //      	 copy_to_user((void*)auxiliary_stack_pointer,(void*)&current->pid,8);	
		//printk("stack management information : reg->sp is %p - auxiliary sp is %p - hitted objectr is %u - pgd descriptor is %u\n",regs->sp,auxiliary_stack_pointer,hitted_object,i);
		regs->sp = auxiliary_stack_pointer;
		regs->ip = callback[target];
	}

overtick_APIC_interrupt_kernel_mode:

	local_irq_save(flags);
	stretch_flag[smp_processor_id()] = 1;
	stretch_cycles = (*original_calibration) / OVERTICK_SCALING_FACTOR; 
ENABLE 	my__setup_APIC_LVTT(stretch_cycles, 0, 1);
   	local_irq_restore(flags);

	return;



/*
//	overtick_count[target] -= COMPENSATE;
	local_irq_save(flags);
	stretch_flag[smp_processor_id()] = 1;
//	stretch_cycles = (*original_calibration) / OVERTICK_SCALING_FACTOR; 
//	stretch_cycles = (*original_calibration) / OVERTICK_COMPENSATION_FACTOR; 
	stretch_cycles = (*original_calibration) ; 
ENABLE 	my__setup_APIC_LVTT(stretch_cycles, 0, 1);
   	local_irq_restore(flags);
*/
	return;
}

int time_stretch_open(struct inode *inode, struct file *filp) {

	// It's meaningless to open this device in write mode
	if (((filp->f_flags & O_ACCMODE) == O_WRONLY)
	    || ((filp->f_flags & O_ACCMODE) == O_RDWR)) {
		return -EACCES;
	}

	// Only one program at a time can use this module's facilities
	if (!mutex_trylock(&ts_mutex)) {
		return -EBUSY;
	}

	enabled_registering = 1;
	return 0;
}


int time_stretch_release(struct inode *inode, struct file *filp) {

	int i;

	mutex_lock(&ts_thread_register);

	for (i = 0; i < TS_THREADS; i++) {
		ts_threads[i] = -1;
	}

	for (i = 0; i < TS_THREADS; i++) {
		 last_passage[i] = 0;
	}

	enabled_registering = 0;

	mutex_unlock(&ts_thread_register);

	mutex_unlock(&ts_mutex);
	
	timer_reset(NULL);

	smp_call_function(timer_reset, NULL, 1);

	return 0;
}


static long time_stretch_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {

	int ret = 0;
	unsigned long fl;
	int descriptor = -1;
	int i;
	unsigned long aux_flags;
	unsigned int stretch_cycles;


	switch (cmd) {

 	case IOCTL_SETUP_BUFFER:
		control_buffer = (flags *)arg;
		printk(KERN_DEBUG "%s: control buffer setup at address %p\n", KBUILD_MODNAME, control_buffer);

		DEBUG 
		printk("thread %d - inspecting the array state\n",current->pid);


		mutex_lock(&ts_thread_register);

		DEBUG
		for (i=0;i< TS_THREADS;i++){
			printk("slot[%i] - value %d\n",i,ts_threads[i]);
		}

       		for (i = 0; i < TS_THREADS; i++) {
			ts_threads[i] = -1;
		}

		DEBUG
		for (i=0;i< TS_THREADS;i++){
			printk("slot[%i] - value %d\n",i,ts_threads[i]);
		}

		mutex_unlock(&ts_thread_register);

		break;


 	case IOCTL_SETUP_CALLBACK:

		ret = -1;

		for(i = 0; i < TS_THREADS; i++){

			if(ts_threads[i] == current->pid){
			DEBUG
			printk(KERN_INFO "%s: found registered thread entry %d - setting up callback at address %p\n", KBUILD_MODNAME, i, (void*)arg);

			callback[i] = (void*)arg;
			
			if( arg != 0x0 ){
				local_irq_save(aux_flags);
				stretch_flag[smp_processor_id()] = 1;
				stretch_cycles = (*original_calibration) / OVERTICK_SCALING_FACTOR; 
ENABLE 				my__setup_APIC_LVTT(stretch_cycles, 0, 1);
	       			local_irq_restore(aux_flags);
			}
		
			ret = 0;
//			break;
			}	
		}

		break;

 	case IOCTL_REGISTER_THREAD:
		DEBUG
		printk(KERN_INFO "%s: registering thread %d - arg is %p\n", KBUILD_MODNAME, current->pid, (void*)arg);

		mutex_lock(&ts_thread_register);

		DEBUG
		printk("thread %d - inspecting the array state\n",current->pid);

		DEBUG
		for (i=0;i< TS_THREADS;i++){
			printk("slot[%i] - value %d\n",i,ts_threads[i]);
		}


		if(enabled_registering){
			for (i = 0; i < TS_THREADS; i++) {
				if (ts_threads[i] == -1) {
					ts_threads[i] = current->pid;
					overtick_count[i] = 0;
					descriptor = i;
					goto end_register;
				}
			}
		}

		DEBUG
		printk(KERN_INFO "%s: registering thread %d - arg is %p - thi is failure descriptor is %d\n", KBUILD_MODNAME, current->pid, (void*)arg,descriptor);

		descriptor = -1;
		
end_register:
		DEBUG
		printk("thread %d - reinspecting the array state\n",current->pid);

		DEBUG
		for (i=0;i< TS_THREADS;i++){
			printk("slot[%i] - value %d\n",i,ts_threads[i]);
		}

		printk(KERN_INFO "%s: registering thread %d done\n", KBUILD_MODNAME, current->pid);
		mutex_unlock(&ts_thread_register);

		time_cycles = *original_calibration;

		ret = descriptor;

		break;

	case IOCTL_DEREGISTER_THREAD:
		
		mutex_lock(&ts_thread_register);
		
		if(ts_threads[arg] == current->pid){
			ts_threads[arg]=-1;
			callback[arg]=NULL;
		ret = 0;
		} else {
			ret = -EINVAL;
		} 

		mutex_unlock(&ts_thread_register);

		time_cycles = *original_calibration;
		local_irq_save(fl);
ENABLE 		my__setup_APIC_LVTT(time_cycles,0,1);
		stretch_flag[smp_processor_id()] = 0;
		local_irq_restore(fl);

		break;

	default:
		ret = -EINVAL;
	}

	return ret;
}

static void print_bytes(char *str, unsigned char *ptr, size_t s) {
	size_t i;

	printk(KERN_DEBUG "%s: %s: ", KBUILD_MODNAME, str);
	for(i = 0; i < s; i++)
		printk(KERN_CONT "%02x ", ptr[i]);
	printk(KERN_CONT "\n");
}


static void scheduler_hook(void) {

	unsigned long flags;
	unsigned int time_cycles;
	unsigned int stretch_cycles;
	int i;
	int cross_check;//dummy - usefull for compilation layout
	unsigned int ts_stretch;
	

	watch_dog++;

//	DEBUG
	if (watch_dog >= 0x000000000000ffff){
		printk(KERN_DEBUG "%s: watch dog trigger for thread %d CPU-id is %d\n", KBUILD_MODNAME, current->pid, smp_processor_id());
		watch_dog = 0;
	}

	for(i = 0; i < TS_THREADS; i++){

		if(ts_threads[i] == current->pid){
			DEBUG
			printk(KERN_INFO "%s: found TS thread %d on CPU %d\n", KBUILD_MODNAME, current->pid,smp_processor_id());

			local_irq_save(flags);
			stretch_flag[smp_processor_id()] = 1;
			overtick_count[i] = OVERTICK_SCALING_FACTOR;
			CPU_overticks[smp_processor_id()] = OVERTICK_SCALING_FACTOR;
			stretch_cycles = (*original_calibration) / OVERTICK_SCALING_FACTOR; 
ENABLE 			my__setup_APIC_LVTT(stretch_cycles, 0, 1);
	       		local_irq_restore(flags);

			goto hook_end;

/*
	
			if((control_buffer[i].user == 1)){ //this is a new standing request 

				DEBUG
				printk(KERN_INFO "%s: Found stretch request by %d on CPU %d\n", KBUILD_MODNAME, current->pid,smp_processor_id());
				last_passage[i] = 1;
				//set request as active

				ts_stretch = control_buffer[i].millisec;
				if(ts_stretch > MAX_STRETCH) ts_stretch = MAX_STRETCH;

				//stretch_cycles = (*original_calibration) * (control_buffer[i].millisec/base_time_interrupt); 

				DEBUG
				printk(KERN_INFO "%s: stretch cycles %u - orginal cycles %u (asked millisec are %d)\n", KBUILD_MODNAME, stretch_cycles,*original_calibration,control_buffer[i].millisec);

				stretch_cycles = (*original_calibration) * (ts_stretch/base_time_interrupt); 

				DEBUG
				printk(KERN_INFO "%s: RECHECK - stretch cycles %u - orginal cycles %u (granted millisec are %d)\n", KBUILD_MODNAME, stretch_cycles,*original_calibration,ts_stretch);


				local_irq_save(flags);
				stretch_flag[smp_processor_id()] = 1;
				//clear_tsk_need_resched(current);
ENABLE 				my__setup_APIC_LVTT(stretch_cycles, 0, 1);
	       			local_irq_restore(flags);
	
				goto hook_end;
			}// end if request standing
			
	
			if( (control_buffer[i].user == 0) && (last_passage[i] == 1)){
			   last_passage[i] = 0;
			   break;
			} //this thread needs to be realigned to the original timer 
*/

		}	
	

	} // end for
	

	//critical section
	local_irq_save(flags);
	if (stretch_flag[smp_processor_id()] == 1) {
		DEBUG
		printk(KERN_DEBUG "%s: found a stretch on  CPU-id %d is (thread is %d)\n", KBUILD_MODNAME, smp_processor_id(),current->pid);
		CPU_overticks[smp_processor_id()] = 0;
		time_cycles = *original_calibration;
ENABLE 		my__setup_APIC_LVTT(time_cycles, 0, 1);
		stretch_flag[smp_processor_id()] = 0;
	}
	local_irq_restore(flags);
		

    hook_end:

	// This gives us space for self patching. There are 5 bytes rather than 4, because
	// we might have to copy here 3 pop instructions, one two-bytes long and the other
	// one byte long.
	__asm__ __volatile__ ("nop; nop; nop; nop; nop");
	return;
}


static int check_patch_compatibility(void) {
	unsigned char *ptr;

	int matched = 0;
	int i;
	unsigned char magic[9] = {0x41, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f};
	int j = 0;

	// Below is for the APIC top half interrupt
	unsigned char *temp;
	long APIC_displacement;
	int pos = 0;



	print_bytes("Scheduler side - I will check the compatibility on these bytes", finish_task_switch, finish_task_switch_next - finish_task_switch);

	printk(KERN_DEBUG "Scanning bytes backwards for magic matching ('-' are don't care bytes): ");
	ptr = (unsigned char *)finish_task_switch_next - 1;
	while((long)ptr >= (long)finish_task_switch) {

		if(*ptr != 0xc3) {
			printk(KERN_CONT "-");
			ptr--;
			continue;
		}

		// Here we should have found the first (from the end of the function) ret
		// Check whether the pattern is one of:
		// pop pop ret
		// pop pop pop ret
		// pop pop pop pop ret
		for(j = -1; j >= -4; j--) {
			matched = 0;
			for(i = 0; i < 9; i++) {
				if(ptr[j] == magic[i]) {
					printk(KERN_CONT "%02x ", ptr[j]);
					matched = 1;
					break;
				}
			}
			if(!matched)
				break;
		}

		if(matched) {

			printk(KERN_CONT "\n");
			if(ptr[j] == 0x41) {
				printk(KERN_DEBUG "%s: first jump assumed to be one-byte, yet it's two-bytes. Self-correcting the patch.\n", KBUILD_MODNAME);
				bytes_to_patch_in_scheduler = 5;
				ptr--;
			}

			// If the pattern is found, ptr must point to the first byte *after* the ret
			ptr++;
			break;
		}

		// We didn't find the pattern. Go on scanning backwards
		ptr--;
		
	}

	if(ptr == finish_task_switch) {
		printk(KERN_CONT "no valid ret instruction found in finish_task_switch\n");
		goto failed;
	}

	// We've been lucky! Update the position where we are going to patch the scheduler
	finish_task_switch_next = ptr;
	
	if(bytes_to_patch_in_scheduler > 4) {
		printk(KERN_CONT "%02x ", ((unsigned char *)finish_task_switch_next)[-6]);
	}
	printk(KERN_CONT "%02x %02x %02x %02x %02x looks correct.\n",
			((unsigned char *)finish_task_switch_next)[-5],
			((unsigned char *)finish_task_switch_next)[-4],
			((unsigned char *)finish_task_switch_next)[-3],
			((unsigned char *)finish_task_switch_next)[-2],
			((unsigned char *)finish_task_switch_next)[-1]);

	printk(KERN_DEBUG "%s: total number of bytes to be patched is %d\n", KBUILD_MODNAME, bytes_to_patch_in_scheduler);

	// Here I check (and possibly prepare) the patch for the top half APIC timer interrupt
	temp = apic_timer_interrupt_start;
	while(temp < (apic_timer_interrupt_start + 1024)) { // The top half is minimal in size, 1024 bytes search should be enough
		if (*temp == 0xe8)
			goto found_call;
		temp++;
	}

	printk("APIC top half check: 'call' to be patched not found\n");

	return -1;

found_call:

	printk("APIC top half: the call to be patched is at address %p\n",temp);

	APIC_displacement = ((long)my_smp_apic_timer_interrupt - (long)temp);

	if(APIC_displacement != (long)(int)APIC_displacement) {
		printk(KERN_NOTICE "%s: Error: APIC patch displacement out of bounds, I cannot hijack the APIC timer interrupt\n", KBUILD_MODNAME);
		goto failed;
	}
	else{
		printk(KERN_NOTICE "%s: APIC patch displacement in bounds - value is %ld \n", KBUILD_MODNAME, APIC_displacement);
	}

	// Packing the patch
	pos = 0;
	APIC_bytes_to_redirect[pos++] = 0xe8;
	APIC_bytes_to_redirect[pos++] = (unsigned char)(APIC_displacement & 0xff);
	APIC_bytes_to_redirect[pos++] = (unsigned char)(APIC_displacement >> 8 & 0xff);
	APIC_bytes_to_redirect[pos++] = (unsigned char)(APIC_displacement >> 16 & 0xff);
	APIC_bytes_to_redirect[pos++] = (unsigned char)(APIC_displacement >> 24 & 0xff);


	print_bytes("APIC patch: assembled call", APIC_bytes_to_redirect, 5);

	APIC_restore_addr = temp;


	// save the bytes to be retored 
	memcpy(APIC_bytes_to_restore, temp, 5);
	print_bytes("APIC patch: bytes to restore for the call", APIC_bytes_to_restore, 5);

	return 0;

    failed: 

	printk(KERN_NOTICE "%s: magic check on bytes ", KBUILD_MODNAME);

/*	printk(KERN_CONT "%02x %02x %02x %02x %02x failed.\n",
			((unsigned char *)correct_pattern_position)[0],
			((unsigned char *)correct_pattern_position)[1],
			((unsigned char *)correct_pattern_position)[2],
			((unsigned char *)correct_pattern_position)[3],
			((unsigned char *)correct_pattern_position)[4]);

*/
	printk(KERN_CONT "%02x %02x %02x %02x %02x failed.\n",
			((unsigned char *)finish_task_switch_next)[-5],
			((unsigned char *)finish_task_switch_next)[-4],
			((unsigned char *)finish_task_switch_next)[-3],
			((unsigned char *)finish_task_switch_next)[-2],
			((unsigned char *)finish_task_switch_next)[-1]);
	
	return -1;
}




static int scheduler_patch(void) {

	int pos = 0;
	long displacement;
	int ret = 0;
	//int i;
	unsigned char *ptr;
	unsigned long cr0;
	unsigned char bytes_to_redirect[5];

	printk(KERN_DEBUG "%s: start patching the scheduler...\n", KBUILD_MODNAME);

	// check_patch_compatibility overrides the content of finish_task_switch_next
	// as it is adjusted to the first byte after the ret instruction of finish_task_switch.
	// Additionally, bytes is set to the number of bytes required to correctly
	// patch the end of finish_task_switch.
	ret = check_patch_compatibility();
	if(ret)
		goto out;

	// Backup the final bytes of finish_task_switch, for later unmounting and finalization of the patch
	memcpy(finish_task_switch_original_bytes, finish_task_switch_next - bytes_to_patch_in_scheduler, bytes_to_patch_in_scheduler);
	print_bytes("made a backup of bytes", finish_task_switch_original_bytes, bytes_to_patch_in_scheduler);

	// Compute the displacement for the jump to be placed at the end of the scheduler
	displacement = ((long)scheduler_hook - (long)finish_task_switch_next);
	
	if(displacement != (long)(int)displacement) {
		printk(KERN_NOTICE "%s: Error: displacement out of bounds, I cannot hijack the scheduler postamble\n", KBUILD_MODNAME);
		ret = -1;
		goto out;
	}

	// Assemble the actual jump. Thanks to little endianess, we must manually swap bytes
	pos = 0;
	bytes_to_redirect[pos++] = 0xe9;
	bytes_to_redirect[pos++] = (unsigned char)(displacement & 0xff);
	bytes_to_redirect[pos++] = (unsigned char)(displacement >> 8 & 0xff);
	bytes_to_redirect[pos++] = (unsigned char)(displacement >> 16 & 0xff);
	bytes_to_redirect[pos++] = (unsigned char)(displacement >> 24 & 0xff);

	if(bytes_to_patch_in_scheduler > 4)
		bytes_to_redirect[pos++] = 0x90; // Fill it with a nop, after the jump, to make the instrumentation cleaner

	print_bytes("assembled jump is", bytes_to_redirect, 5);

	// Find the correct place where to store backed up bytes from the original scheduler.
	// This is starting exactly at the ret (0xc3) instruction at the end of
	// scheduler hook. We start from the function after it, and for safety we check whether
	// there is enough space or not.

	print_bytes("scheduler_hook before prepare self patch", (unsigned char *)scheduler_hook, 600);
	ptr = (unsigned char *)prepare_self_patch();
	if(ptr == NULL) {
		ret = -1;
		goto out;
	}
	
	// Now do the actual patching. Clear CR0.WP to disable memory protection.
	cr0 = read_cr0();
	write_cr0(cr0 & ~X86_CR0_WP);

	// Patch the end of our scheduler_hook to execute final bytes of finish_task_switch
	print_bytes("scheduler_hook before self patching", (unsigned char *)scheduler_hook, 600);
	memcpy(ptr, finish_task_switch_original_bytes, bytes_to_patch_in_scheduler);
	print_bytes("scheduler_hook after self patching", (unsigned char *)scheduler_hook, 600);

	// Patch finish_task_switch to jump, at the end, to scheduler_hook
	print_bytes("finish_task_switch_next before self patching", (unsigned char *)finish_task_switch_next - bytes_to_patch_in_scheduler, 64);
	memcpy((unsigned char *)finish_task_switch_next - bytes_to_patch_in_scheduler, bytes_to_redirect, bytes_to_patch_in_scheduler);
	print_bytes("finish_task_switch_next after self patching", (unsigned char *)finish_task_switch_next - bytes_to_patch_in_scheduler, 64);

	//patch the APIC top half interrupt
	memcpy(APIC_restore_addr, APIC_bytes_to_redirect, 5);
		
	write_cr0(cr0);
	

	printk(KERN_INFO "%s: scheduler correctly patched...\n", KBUILD_MODNAME);

    out:
	return ret;
}


static void scheduler_unpatch(void) {
	unsigned long cr0;
	
	printk(KERN_DEBUG "%s: restoring standard scheduler...\n", KBUILD_MODNAME);

	// To unpatch, simply place back original bytes of finish_task_switch
	// and original bytes in the top half APIC interrupt
	cr0 = read_cr0();
	write_cr0(cr0 & ~X86_CR0_WP);

	// Here is for the scheduler
	memcpy((char *)finish_task_switch_next - bytes_to_patch_in_scheduler, (char *)finish_task_switch_original_bytes, bytes_to_patch_in_scheduler);

	// Here is for the APIC top half interrupt
	memcpy(APIC_restore_addr, APIC_bytes_to_restore, 5);
	print_bytes("APIC patch: bytes restored for the call", APIC_restore_addr, 5);
//
	write_cr0(cr0);

	printk(KERN_INFO "%s: standard scheduler correctly restored...\n", KBUILD_MODNAME);
}


static int time_stretch_init(void) {

	int ret = 0;

	printk(KERN_INFO "%s: mounting the module\n", KBUILD_MODNAME);
	printk(KERN_INFO "%s:calibration is %u - scaled is %u\n", KBUILD_MODNAME, *original_calibration, *original_calibration / OVERTICK_SCALING_FACTOR);

	// Initialize the device mutex
	mutex_init(&ts_mutex);

	// Dynamically allocate a major for the device
	major = register_chrdev(0, "timestretch", &fops);
	if (major < 0) {
		printk(KERN_ERR "%s: failed to register device. Error %d\n", KBUILD_MODNAME, major);
		ret = major;
		goto failed_chrdevreg;
	}
	
	// Create a class for the device
	dev_cl = class_create(THIS_MODULE, "timestretch");
	if (IS_ERR(dev_cl)) {
		printk(KERN_ERR "%s: failed to register device class\n", KBUILD_MODNAME);
		ret = PTR_ERR(dev_cl);
		goto failed_classreg;
	}
	
	// Create a device in the previously created class
	device = device_create(dev_cl, NULL, MKDEV(major, 0), NULL, "timestretch");
	if (IS_ERR(device)) {
		printk(KERN_ERR "%s: failed to create device\n", KBUILD_MODNAME);
		ret = PTR_ERR(device);
		goto failed_devreg;
	}

	// Create sysfs endpoints
	// dev_attr_multimap comes from the DEVICE_ATTR(...) at the top of this module
	// If this call succeds, then there is a new file in:
	// /sys/devices/virtual/timestretch/timestretch/multimap
	// Which can be used to dialogate with the driver
	ret = device_create_file(device, &dev_attr_multimap);
	if (ret < 0) {
		printk(KERN_WARNING "%s: failed to create write /sys endpoint. Maybe the device file /dev/timestretch must be created by hand on major %d\n", KBUILD_MODNAME, major);
	}

	base_time_interrupt = 1000/Hz;

	ret = scheduler_patch();	
	if(ret)
		goto failed_patch;
	
	printk("final audit on APIC parameters: scaling factor is %d - compensating factor is %d - compensate is %d\n",OVERTICK_SCALING_FACTOR, OVERTICK_COMPENSATION_FACTOR, COMPENSATE);

	return 0;
	
    failed_patch:
	device_remove_file(device, &dev_attr_multimap);
	device_destroy(dev_cl, MKDEV(major, 0));
    failed_devreg:
	class_unregister(dev_cl);
	class_destroy(dev_cl);
    failed_classreg:
	unregister_chrdev(major, "timestretch");
    failed_chrdevreg:
	return ret;
}


static void time_stretch_cleanup(void) {

	int i;
	unsigned long flags;
	unsigned int time_cycles;

	scheduler_unpatch();
	
	// Remove device file and class
	device_remove_file(device, &dev_attr_multimap);
	device_destroy(dev_cl, MKDEV(major, 0));
	class_unregister(dev_cl);
	class_destroy(dev_cl);
	unregister_chrdev(major, "timestretch");
	
	printk(KERN_INFO "%s: delaying module unmount for multicore scheduler safety", KBUILD_MODNAME);
	for(i=0; i < DELAY; i++) printk(KERN_CONT ".");
	printk(KERN_CONT " delay is over\n");

	local_irq_save(flags);
	stretch_flag[smp_processor_id()] = 0;
	time_cycles = *original_calibration;
ENABLE 	my__setup_APIC_LVTT(time_cycles, 0, 1);
	stretch_flag[smp_processor_id()] = 0;
	local_irq_restore(flags);
	printk(KERN_DEBUG "%s: core %d - realigning time cycles (tid is %d - CPU id is %d)\n", KBUILD_MODNAME, smp_processor_id(), current->pid, smp_processor_id());

	smp_call_function(timer_reset, NULL, 1);

	for(i = 0; i < MAX_CPUs; i++){
		while(stretch_flag[i]);
	}
	printk(KERN_INFO "%s: unmounted successfully\n", KBUILD_MODNAME);
}

void timer_reset(void* dummy){

	unsigned long flags;
	unsigned int time_cycles;

	local_irq_save(flags);
	time_cycles = *original_calibration;
ENABLE  my__setup_APIC_LVTT(time_cycles, 0, 1);
	stretch_flag[smp_processor_id()] = 0;
	local_irq_restore(flags);
	printk(KERN_DEBUG "%s: core %d - realigning time cycles (tid is %d - CPU id is %d)\n", KBUILD_MODNAME, smp_processor_id(), current->pid, smp_processor_id());

	

}

static void *prepare_self_patch(void) {
	unsigned char *next = NULL;
	// All functions in this module, excepting scheduler_hook.
	// Read the comment above function prototypes for an explanation
	void *functions[] = {	my_smp_apic_timer_interrupt,	
				prepare_self_patch,
				time_stretch_cleanup,
				time_stretch_init,
				scheduler_unpatch,
				scheduler_patch,
				print_bytes,
				check_patch_compatibility,
				time_stretch_ioctl,
				time_stretch_release,
				time_stretch_open,
				timer_reset,
				NULL};
	void *curr_f;
	int i = 0;
	unsigned char *ret_insn;
	unsigned long cr0;


	
	curr_f = functions[0];
	while(curr_f != NULL) {
		curr_f = functions[i++];
		
		if((long)curr_f > (long)scheduler_hook) {
			
			if(next == NULL || (long)curr_f < (long)next)
				next = curr_f;
		}
	}
	
	// if ptr == NULL, then scheduler_hook is last function in the kernel module...
	if(next == NULL) {
		printk(KERN_DEBUG "%s: scheduler_hook is likely the last function of the module. ", KBUILD_MODNAME);
		printk(KERN_DEBUG "%s: Scanning from the end of the page\n", KBUILD_MODNAME);

		next = (unsigned char *)scheduler_hook + 4096;
		next = (unsigned char*)((ulong) next & 0xfffffffffffff000);
		next--;

	}

	// We now look for the ret instruction. Some care must be taken here. We assume before the ret
	// there is at least one nop...
	while((long)next >= (long)scheduler_hook) {
		if(*next == 0xc3) {
			if(*(next-1) == 0x58 || *(next-1) == 0x59 || *(next-1) == 0x5a || *(next-1) == 0x5b ||
			   *(next-1) == 0x5c || *(next-1) == 0x5d || *(next-1) == 0x5d || *(next-1) == 0x5f)
				break;
		}
		next--;
	}

	printk(KERN_DEBUG "%s: Identified ret instruction byte %02x at address %p\n", KBUILD_MODNAME, *next, next);

	// Did we have luck?
	if((long)next == (long)scheduler_hook) {
		ret_insn = NULL;
		goto out;
	} else {
		ret_insn = next;
	}

	// Now scan backwards until we find the last nop that we have placed in our code
	while(*next != 0x90)
		next--;

	print_bytes("before self patching", next - (bytes_to_patch_in_scheduler  - 1), ret_insn - next + (bytes_to_patch_in_scheduler  - 1));

	// Move backwars anything from here to the ret instruction.
	// This gives us space to insert the backed up bytes
	cr0 = read_cr0();
	write_cr0(cr0 & ~X86_CR0_WP);
	memcpy(next - (bytes_to_patch_in_scheduler  - 1), next + 1, ret_insn - next - 1);
	write_cr0(cr0);
	
	print_bytes("after self patching", next - (bytes_to_patch_in_scheduler  - 1), ret_insn - next + (bytes_to_patch_in_scheduler  - 1));

	// We have moved everything 4 bytes behind, which gives us space for finalizing the patch
	ret_insn -= bytes_to_patch_in_scheduler;
    out:
	return ret_insn;
}

