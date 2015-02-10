#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xfe2565c8, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x34a118c8, __VMLINUX_SYMBOL_STR(mutex_unlock) },
	{ 0x754c1db, __VMLINUX_SYMBOL_STR(mutex_lock) },
	{ 0x216c18fa, __VMLINUX_SYMBOL_STR(mutex_trylock) },
	{ 0xd75c79df, __VMLINUX_SYMBOL_STR(smp_call_function) },
	{ 0xbd1763fe, __VMLINUX_SYMBOL_STR(current_task) },
	{ 0x7a2af7b4, __VMLINUX_SYMBOL_STR(cpu_number) },
	{ 0x78764f4e, __VMLINUX_SYMBOL_STR(pv_irq_ops) },
	{ 0x6bc3fbc0, __VMLINUX_SYMBOL_STR(__unregister_chrdev) },
	{ 0xd4868d63, __VMLINUX_SYMBOL_STR(class_destroy) },
	{ 0x36afa619, __VMLINUX_SYMBOL_STR(class_unregister) },
	{ 0x59999f52, __VMLINUX_SYMBOL_STR(device_destroy) },
	{ 0x678cc535, __VMLINUX_SYMBOL_STR(device_remove_file) },
	{ 0xcf622b5d, __VMLINUX_SYMBOL_STR(device_create_file) },
	{ 0x999806d9, __VMLINUX_SYMBOL_STR(device_create) },
	{ 0x6afc95af, __VMLINUX_SYMBOL_STR(__class_create) },
	{ 0x79bb2ea3, __VMLINUX_SYMBOL_STR(__register_chrdev) },
	{ 0xfe923a69, __VMLINUX_SYMBOL_STR(__mutex_init) },
	{ 0x69acdf38, __VMLINUX_SYMBOL_STR(memcpy) },
	{ 0xdb7305a1, __VMLINUX_SYMBOL_STR(__stack_chk_fail) },
	{ 0x216cbeb2, __VMLINUX_SYMBOL_STR(pv_cpu_ops) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "A471D2478069CFF6EA901CB");
