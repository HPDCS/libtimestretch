AC_PREREQ([2.63])
AC_INIT([timestretch], [1.0.0], [quaglia@dis.uniroma1.it,pellegrini@dis.uniroma1.it])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AC_CONFIG_MACRO_DIR([m4])

# Preliminary check: are we on Linux?
AC_CANONICAL_HOST
AC_MSG_CHECKING([for supported host Operating System])
case $host_os in
  linux*)
        # Do something specific for linux
        AC_MSG_RESULT([yes, ${host_os}])
        ;;
    *)
        #Default Case
        AC_MSG_RESULT([no, ${host_os}])
        AC_MSG_ERROR([This module runs only on Linux])
        ;;
esac

# Preliminary check: are we on x86?
AC_MSG_CHECKING([for a supported CPU architecture])
case "${host_cpu}" in

     x86_64)
        AC_MSG_RESULT([yes, ${host_cpu}])
        ;;

     *)
       AC_MSG_RESULT([no, ${host_cpu}])
       AC_MSG_ERROR([Unsupported host architecture. Currently ROOT-Sim supports only x86_64 systems.])
       ;;
esac




# Configure kernel module paths
AC_SUBST([with_kernel], [`uname -r`])
AC_SUBST([with_kernel_mod], [/lib/modules/$with_kernel/extra])
AC_SUBST([KERNEL_SRC], [/lib/modules/$with_kernel/build])
AC_SUBST([KERNEL_MOD], [$with_kernel_mod])

# Checks for programs.
AC_LANG([C])
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MKDIR_P
AC_PROG_LN_S
AC_PROG_GREP
AC_PROG_SED
AC_PROG_RANLIB
AC_PATH_PROG(RM, rm, /bin/rm, $PATH:/bin:/usr/bin:/usr/local/bin)

# Get calibration_result kernel symbol address
AC_MSG_CHECKING([for address of calibration_result in the current kernel])
calibration_result=$($GREP calibration_result /boot/System.map-$(uname -r) | $SED 's/ .*//g')
if test -z "$calibration_result"; then
	calibration_result=$($GREP lapic_timer_frequency /boot/System.map-$(uname -r) | $SED 's/ .*//g')
	if test -z "$calibration_result"; then
		AC_MSG_ERROR([Address of calibration_result not found in kernel map])
	fi
fi
AC_MSG_RESULT([found at 0x$calibration_result])
AC_SUBST([ORIGINAL_CALIBRATION], [0x$calibration_result])

# Get setup_APIC_LVTT in the current kernel
AC_MSG_CHECKING([for address of __setup_APIC_LVTT in the current kernel])
apic_lvtt=$($GREP __setup_APIC_LVTT /boot/System.map-$(uname -r) | $SED 's/ .*//g')
if test -z "$apic_lvtt"; then
        AC_MSG_ERROR([Address of __setup_APIC_LVTT not found in kernel map])
fi
AC_MSG_RESULT([found at 0x$apic_lvtt])
AC_SUBST([SETUP_APIC_LVTT], [0x$apic_lvtt])


# Get CONFIG_HZ in the current kernel
AC_MSG_CHECKING([for CONFIG_HZ in the current kernel])
config_hz=$($GREP "CONFIG_HZ=" /boot/config-$(uname -r) | $SED 's/.*=//g')
if test -z "$config_hz"; then
        AC_MSG_ERROR([CONFIG_HZ not found in kernel configuration])
fi
AC_MSG_RESULT([found, is $config_hz])
AC_SUBST([KERNEL_HZ], [$config_hz])

# Get finish_task_switch in the current kernel
AC_MSG_CHECKING([for finish_task_switch in the current kernel])
fts_line=$($GREP -n finish_task_switch /boot/System.map-$(uname -r) | $SED 's/:.*//')
finish_task_switch=$($SED "${fts_line}q;d" /boot/System.map-$(uname -r) | $SED 's/ .*//')
if test -z "$finish_task_switch"; then
        AC_MSG_ERROR([Address of finish_task_switch not found in kernel map])
fi
AC_MSG_RESULT([found at 0x$finish_task_switch])
AC_SUBST([FTS_ADDR], [0x$finish_task_switch])

# Get function after finish_task_switch in the current kernel
AC_MSG_CHECKING([the address of function next to finish_task_switch in the current kernel])
let fts_line=$fts_line+1
finish_task_switch_next=$($SED "${fts_line}q;d" /boot/System.map-$(uname -r) | $SED 's/ .*//')
if test -z "$finish_task_switch_next"; then
        AC_MSG_ERROR([Address of function aftr finish_task_switch not found in kernel map])
fi
AC_MSG_RESULT([found at 0x$finish_task_switch_next])
AC_SUBST([FTS_ADDR_NEXT], [0x$finish_task_switch_next])


#the below symbols are required to patch the APIC timer interrupt (top half)
# Get apic_timer_interrupt in the current kernel
AC_MSG_CHECKING([the address of function apic_timer_interrupt in the current kernel])
fts_line=$($GREP -n " apic_timer_interrupt" /boot/System.map-$(uname -r) | $SED 's/:.*//')
apic_timer_interrupt=$($SED "${fts_line}q;d" /boot/System.map-$(uname -r) | $SED 's/ .*//')
if test -z "$apic_timer_interrupt"; then
        AC_MSG_ERROR([Address of function apic_timer_interrupt not found in kernel map])
fi
AC_MSG_RESULT([found at 0x$apic_timer_interrupt])
AC_SUBST([APIC_TIMER_INTERRUPT], [0x$apic_timer_interrupt])

# Get local_apic_timer_interrupt in the current kernel
AC_MSG_CHECKING([the address of function local_apic_timer_interrupt in the current kernel])
fts_line=$($GREP -n "local_apic_timer_interrupt" /boot/System.map-$(uname -r) | $SED 's/:.*//')
local_apic_timer_interrupt=$($SED "${fts_line}q;d" /boot/System.map-$(uname -r) | $SED 's/ .*//')
if test -z "$local_apic_timer_interrupt"; then
        AC_MSG_ERROR([Address of function local_apic_timer_interrupt not found in kernel map])
fi
AC_MSG_RESULT([found at 0x$local_apic_timer_interrupt])
AC_SUBST([LOCAL_APIC_TIMER_INTERRUPT], [0x$local_apic_timer_interrupt])

# Get exit_idle in the current kernel
AC_MSG_CHECKING([the address of function exit_idle in the current kernel])
fts_line=$($GREP -n " exit_idle" /boot/System.map-$(uname -r) | $SED 's/:.*//')
exit_idle=$($SED "${fts_line}q;d" /boot/System.map-$(uname -r) | $SED 's/ .*//')
if test -z "$exit_idle"; then
        AC_MSG_ERROR([Address of function exit_idle not found in kernel map])
fi
AC_MSG_RESULT([found at 0x$exit_idle])
AC_SUBST([EXIT_IDLE], [0x$exit_idle])

# Get irq_enter in the current kernel
AC_MSG_CHECKING([the address of function irq_enter in the current kernel])
fts_line=$($GREP -n " irq_enter" /boot/System.map-$(uname -r) | $SED 's/:.*//')
irq_enter=$($SED "${fts_line}q;d" /boot/System.map-$(uname -r) | $SED 's/ .*//')
if test -z "$irq_enter"; then
        AC_MSG_ERROR([Address of function irq_enter not found in kernel map])
fi
AC_MSG_RESULT([found at 0x$irq_enter])
AC_SUBST([IRQ_ENTER], [0x$irq_enter])

# Get irq_exit in the current kernel
AC_MSG_CHECKING([the address of function irq_exit in the current kernel])
fts_line=$($GREP -n " irq_exit" /boot/System.map-$(uname -r) | $SED 's/:.*//')
irq_exit=$($SED "${fts_line}q;d" /boot/System.map-$(uname -r) | $SED 's/ .*//')
if test -z "$irq_exit"; then
        AC_MSG_ERROR([Address of function irq_exit not found in kernel map])
fi
AC_MSG_RESULT([found at 0x$irq_exit])
AC_SUBST([IRQ_EXIT], [0x$irq_exit])

# Are kernel headers installed?
AC_CHECK_HEADERS([linux/ioctl.h],,
       [AC_MSG_ERROR([You must install kernel-headers])])

# Final output
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
