ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -I$(abs_top_srcdir)
EXTRA_DIST = COPYING INSTALL README \
	     module/Makefile module/overtick.c module/overtick.h module/83-overtick.rules \
             lib/overticklib.c lib/overtick.h 

udevdir = /etc/udev/rules.d
initdir = @sysconfdir@/init.d

udev_DATA = module/83-overtick.rules

lib_LIBRARIES = libovertick.a
libovertick_a_SOURCES=lib/overticklib.c

include_HEADERS = lib/overtick.h

module/overtick.ko: module/overtick.c
	test -f $(builddir)/module/Makefile || $(LN_S) $(abs_top_srcdir)/module/Makefile $(builddir)/module/
	test -f $(builddir)/module/overtick.c || $(LN_S) $(abs_top_srcdir)/module/overtick.c $(builddir)/module/
	test -f $(builddir)/module/overtick.h || $(LN_S) $(abs_top_srcdir)/module/overtick.h $(builddir)/module/
	$(MAKE) -C $(KERNEL_SRC) M=$(abs_top_builddir)/module EXTRA_CFLAGS="-DORIGINAL_CALIBRATION=$(ORIGINAL_CALIBRATION) \
									    -DSETUP_APIC_LVTT=$(SETUP_APIC_LVTT) -DKERNEL_HZ=$(KERNEL_HZ) \
									    -DFTS_ADDR=$(FTS_ADDR) -DFTS_ADDR_NEXT=$(FTS_ADDR_NEXT)\
									    -DAPIC_TIMER_INTERRUPT=$(APIC_TIMER_INTERRUPT)\
									    -DLOCAL_APIC_TIMER_INTERRUPT=$(LOCAL_APIC_TIMER_INTERRUPT)\
									    -DEXIT_IDLE=$(EXIT_IDLE) -DIRQ_ENTER=$(IRQ_ENTER)\
									    -DIRQ_EXIT=$(IRQ_EXIT)"
all-local: module/overtick.ko

install-exec-hook: module/overtick.ko
	test -z "$(DESTDIR)$(KERNEL_MOD)" || $(MKDIR_P) "$(DESTDIR)$(KERNEL_MOD)"
	$(INSTALL) $(builddir)/module/overtick.ko $(DESTDIR)$(KERNEL_MOD)/overtick.ko
	depmod -a

uninstall-hook:
	$(RM) $(DESTDIR)$(KERNEL_MOD)/overtick.ko
	depmod -a

clean-local:
	-$(MAKE) -C $(KERNEL_SRC) M=$(abs_top_builddir)/module clean

