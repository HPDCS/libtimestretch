/**                       Copyright (C) 2014 HPDCS Group
*                       http://www.dis.uniroma1.it/~hpdcs
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
* @date November 11, 2014
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

#include "timestretch.h"


#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,25)
#error Unsupported Kernel Version
#endif

#define DEBUG if(0)

#ifndef X86_CR0_WP
#define X86_CR0_WP 0x00010000
#endif


static unsigned int stretch_flag[MAX_CPUs] = {[0 ... MAX_CPUs-1] 0}; 

// These variables are set up by configure
unsigned int * original_calibration = (int*)ORIGINAL_CALIBRATION;
void (*my__setup_APIC_LVTT)(unsigned int, int, int) =  (void*)SETUP_APIC_LVTT;
int Hz = KERNEL_HZ;



/** FUNCTION PROTOTYPES
 * All the functions in this module must be listed here with a high alignment
 * and explicitly handled in prepare_self_patch(), so as to allow a correct self-patching
 * independently of the order of symbols emitted by the linker, which changes
 * even when small changes to the code are made.
 * Failing to do so, could prevent self-patching and therefore module usability.
 * This approach does not work only if scheduler_hook is the last function of the module,
 * or if the padding become smaller than 4 bytes...
 * In both cases, only a small change in the code (even placing a nop) could help doing
 * the job!
 */
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

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alessandro Pellegrini <pellegrini@dis.uniroma1.it>, Francesco Quaglia <quaglia@dis.uniroma1.it>");
MODULE_DESCRIPTION("On demand stretch of the scheduling quantum");
module_init(time_stretch_init);
module_exit(time_stretch_cleanup);


/* MODULE VARIABLES */
static DEFINE_MUTEX(ts_thread_register);
static int ts_threads[TS_THREADS]={[0 ... (TS_THREADS-1)] = -1};
static int last_passage[TS_THREADS]={[0 ... (TS_THREADS-1)] = 0};
static ulong watch_dog=0;
static DEFINE_MUTEX(ts_mutex);
static int base_time_interrupt;
static int major;
static struct class *dev_cl = NULL; // Device class being created
static struct device *device = NULL; // Device being created
static unsigned int time_cycles;
static flags *control_buffer;
static DEVICE_ATTR(multimap, S_IRUSR|S_IRGRP|S_IROTH, NULL, NULL);
static unsigned char finish_task_switch_original_bytes[5];
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

	return 0;
}


static long time_stretch_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {

	int ret = 0;
	unsigned long fl;
	int descriptor = -1;
	int i;


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

		}	
	

        }// end for 
	

	//critical section
        local_irq_save(flags);
	if (stretch_flag[smp_processor_id()] == 1) {
		DEBUG
		printk(KERN_DEBUG "%s: found a stretch on  CPU-id %d is (thread is %d)\n", KBUILD_MODNAME, smp_processor_id(),current->pid);
		time_cycles = *original_calibration;
ENABLE 		my__setup_APIC_LVTT(time_cycles, 0, 1);
		stretch_flag[smp_processor_id()] = 0;
	}
        local_irq_restore(flags);
		

    hook_end:

	// This gives us space for self patching
	asm volatile("nop;nop;nop;nop");
	return;
}


static int check_patch_compatibility(void) {
	unsigned char *ptr;

	int matched = 0;
	int i;
	unsigned char magic[9] = {0x41, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f};
	int j = 0;


	print_bytes("I will check the compatibility on these bytes", finish_task_switch, finish_task_switch_next - finish_task_switch);


	printk(KERN_DEBUG "Scanning bytes backwards for magic matching: ");
	ptr = (unsigned char *)finish_task_switch_next - 1;
	while((long)ptr >= (long)finish_task_switch) {

		if(*ptr != 0xc3) {
			printk("#");
			ptr--;
			continue;
		}

		// Here we should have found th first (from the end of the function) ret
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
			// If the pattern is found, ptr must point to the first byte *after* the ret
			ptr++;
			break;
		}

		// We didn't find the pattern. Go on scanning backwards
		ptr--;
		
	}
	printk(KERN_CONT "\n");

	if(ptr == finish_task_switch) {
		printk(KERN_INFO "%s: no valid ret instruction found in finish_task_switch\n", KBUILD_MODNAME);
		goto failed;
	}

	// We've been lucky!
	finish_task_switch_next = ptr;
	
	printk("supposed target address is %p - compile time one is %p - double check\n",ptr,finish_task_switch_next);

	printk(KERN_CONT "%02x %02x %02x %02x %02x looks correct.\n",
                        ((unsigned char *)finish_task_switch_next)[-5],
                        ((unsigned char *)finish_task_switch_next)[-4],
                        ((unsigned char *)finish_task_switch_next)[-3],
                        ((unsigned char *)finish_task_switch_next)[-2],
                        ((unsigned char *)finish_task_switch_next)[-1]);

/*	j=0;
	while((long)(correct_pattern_position + j) < (long)finish_task_switch_next){
		printk(KERN_CONT "%02x\n",
                        ((unsigned char *)correct_pattern_position)[j]);
	
		j++;
	}

	finish_task_switch_next = correct_pattern_position + 1;

	ptr = (unsigned char *)finish_task_switch_next - 1;
	
	for(pos = 0; pos < 5; pos++) {
		matched = 0;
		
		for(i = 0; i < 9; i++) {
			if(*ptr == magic[i]) {
				matched = 1;
				break;
			}
		}

		if(!matched)
			goto failed;

		ptr--;
	}
*/
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
	int i;
	unsigned char *ptr;
	unsigned long cr0;
	unsigned char bytes_to_redirect[5];

	printk(KERN_DEBUG "%s: start patching the scheduler...\n", KBUILD_MODNAME);

	// check_patch_compatibility overrides the content of finish_task_switch_next
	// as it is adjusted to the first byte after the ret instruction of finish_task_switch
	ret = check_patch_compatibility();
	if(ret)
		goto out;


	// Backup the final bytes of finish_task_switch, for later unmounting and finalization of the patch
	memcpy(finish_task_switch_original_bytes, finish_task_switch_next - 5, 5);
	print_bytes("made a backup of bytes", finish_task_switch_original_bytes, 5);

	// Compute the displacement for the jump to be placed at the end of the scheduler
	displacement = ((long)scheduler_hook - (long)finish_task_switch_next);
	
	if(displacement != (long)(int)displacement) {
		printk(KERN_NOTICE "%s: Error: displacement out of bounds, I cannot hijack the scheduler postamble\n", KBUILD_MODNAME);
		ret = -1;
		goto out;
	}

	// Assemble the actual jump. Thank to little endianess, we must manually swap bytes
	pos = 0;
	bytes_to_redirect[pos++] = 0xe9;
	bytes_to_redirect[pos++] = (unsigned char)(displacement & 0xff);
        bytes_to_redirect[pos++] = (unsigned char)(displacement >> 8 & 0xff);
        bytes_to_redirect[pos++] = (unsigned char)(displacement >> 16 & 0xff);
        bytes_to_redirect[pos++] = (unsigned char)(displacement >> 24 & 0xff);

	print_bytes("assembled jump is", bytes_to_redirect, 5);

	// Find the correct place where to store backed up bytes from the original scheduler.
	// This is starting exactly at the ret (0xc3) instruction at the end of
	// scheduler hook. We start from the function after it, and for safety we check whether
	// there is enough space or not.
	ptr = (unsigned char *)prepare_self_patch();
	if(ptr == NULL) {
		ret = -1;
		goto out;
	}
	
/*	i = 0;
	while(*ptr != 0xc3) {
		i++;
		ptr--;
		// This is a sanity check. If i grows too much, we're surely gonna patch something completely wrong!
		// 16 is not a random number: is the highest alignment specified by the __attribute__ to the next function
		if(i > 16) {
			printk(KERN_NOTICE "%s: I'm unable to patch myself... %s:%d\n", KBUILD_MODNAME, __FILE__, __LINE__);
			ret = -1;
			goto out;
		}
	}
	if(i < 5) {
		printk(KERN_NOTICE "%s: not enough space to patch my own scheduler_hook function. Aborting...\n", KBUILD_MODNAME);
		ret = -1;
		goto out;
	}
*/
	// Now do the actual patching. Clear CR0.WP to disable memory protection.
	cr0 = read_cr0();
	write_cr0(cr0 & ~X86_CR0_WP);

	print_bytes("scheduler_hook before self patching", scheduler_hook, 512);

	// Patch the end of our scheduler_hook to execute final bytes of finish_task_switch
	memcpy(ptr, finish_task_switch_original_bytes, 5);

	// Patch finish_task_switch to jump, at the end, to scheduler_hook
	memcpy((unsigned char *)finish_task_switch_next - 5, bytes_to_redirect, 5);
		
	write_cr0(cr0);
	
	print_bytes("scheduler_hook after self patching", scheduler_hook, 512);
	printk(KERN_INFO "%s: scheduler correctly patched...\n", KBUILD_MODNAME);

    out:
	return ret;
}


static void scheduler_unpatch(void) {
	unsigned long cr0;
	
	printk(KERN_DEBUG "%s: restoring standard scheduler...\n", KBUILD_MODNAME);

	// To unpatch, simply place back original bytes of finish_task_switch
	cr0 = read_cr0();
	write_cr0(cr0 & ~X86_CR0_WP);
	memcpy((char *)finish_task_switch_next - 5, (char *)finish_task_switch_original_bytes, 5);
	write_cr0(cr0);

	printk(KERN_INFO "%s: standard scheduler correctly restored...\n", KBUILD_MODNAME);
}


static int time_stretch_init(void) {

	int ret = 0;

	printk(KERN_INFO "%s: mounting the module\n", KBUILD_MODNAME);

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

	stretch_flag[smp_processor_id()] = 0;
	time_cycles = *original_calibration;
        local_irq_save(flags);
ENABLE 	my__setup_APIC_LVTT(time_cycles, 0, 1);
	stretch_flag[smp_processor_id()] = 0;
        local_irq_restore(flags);
	printk(KERN_DEBUG "%s: realigning time cycles (tid is %d - CPU id is %d)\n", KBUILD_MODNAME, current->pid, smp_processor_id());

	for(i = 0; i < MAX_CPUs; i++){
		while(stretch_flag[i]);
	}
	printk(KERN_INFO "%s: unmounted successfully\n", KBUILD_MODNAME);
}


static void *prepare_self_patch(void) {
	unsigned char *next = NULL;
	// All functions in this module, excepting scheduler_hook.
	// Read the comment above function prototypes for an explanation
	void *functions[] = {	prepare_self_patch,
				time_stretch_cleanup,
				time_stretch_init,
				scheduler_unpatch,
				scheduler_patch,
				print_bytes,
				check_patch_compatibility,
				time_stretch_ioctl,
				time_stretch_release,
				time_stretch_open,
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
		printk(KERN_CONT "Scanning from the end of the page\n");

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

	printk("Identified ret instruction byte %02x at address %p\n", *next, next);

	// Did we have luck?
	if((long)next == (long)scheduler_hook) {
		ret_insn = NULL;
		goto out;
	} else {
		ret_insn = next;
	}

	// Now scan backwards until we find the last nop we have placed in our code
	while(*next != 0x90)
		next--;

	print_bytes("before self patching", next - 3, ret_insn - next + 3);

	// Move backwars anything from here to the ret instruction
	cr0 = read_cr0();
	write_cr0(cr0 & ~X86_CR0_WP);
	memcpy(next - 3, next + 1, ret_insn - next - 1);
	write_cr0(cr0);
	
	print_bytes("after self patching", next - 3, ret_insn - next + 3);

	// We have moved everything 4 bytes behind, which gives us space for finalizing the patch
	ret_insn -= 4;
    out:
	return (void *)ret_insn;
}

