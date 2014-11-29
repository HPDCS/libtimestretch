ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -I$(abs_top_srcdir)
EXTRA_DIST = COPYING INSTALL README \
	     module/Makefile module/timestretch.c module/timestretch.h module/83-timestretch.rules \
             lib/timestretchlib.c lib/timestretch.h \
	     test/example.c test/cpu-consumer.c

udevdir = /etc/udev/rules.d
initdir = @sysconfdir@/init.d

udev_DATA = module/83-timestretch.rules

lib_LIBRARIES = libtimestretch.a
libtimestretch_a_SOURCES=lib/timestretchlib.c

include_HEADERS = lib/timestretch.h

module/timestretch.ko: module/timestretch.c
	test -f $(builddir)/module/Makefile || $(LN_S) $(abs_top_srcdir)/module/Makefile $(builddir)/module/
	test -f $(builddir)/module/timestretch.c || $(LN_S) $(abs_top_srcdir)/module/timestretch.c $(builddir)/module/
	test -f $(builddir)/module/timestretch.h || $(LN_S) $(abs_top_srcdir)/module/timestretch.h $(builddir)/module/
	$(MAKE) -C $(KERNEL_SRC) M=$(abs_top_builddir)/module EXTRA_CFLAGS="-DORIGINAL_CALIBRATION=$(ORIGINAL_CALIBRATION) \
									    -DSETUP_APIC_LVTT=$(SETUP_APIC_LVTT) -DKERNEL_HZ=$(KERNEL_HZ) \
									    -DFTS_ADDR=$(FTS_ADDR) -DFTS_ADDR_NEXT=$(FTS_ADDR_NEXT)"
all-local: module/timestretch.ko

install-exec-hook: module/timestretch.ko
	test -z "$(DESTDIR)$(KERNEL_MOD)" || $(MKDIR_P) "$(DESTDIR)$(KERNEL_MOD)"
	$(INSTALL) $(builddir)/module/timestretch.ko $(DESTDIR)$(KERNEL_MOD)/timestretch.ko
	depmod -a

uninstall-hook:
	$(RM) $(DESTDIR)$(KERNEL_MOD)/timestretch.ko
	depmod -a

clean-local:
	-$(MAKE) -C $(KERNEL_SRC) M=$(abs_top_builddir)/module clean

