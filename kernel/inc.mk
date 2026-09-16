KERNEL_OBJS-y = bootparam.o
KERNEL_OBJS-y += console.o
KERNEL_OBJS-y += main.o
KERNEL_OBJS-y += memtest.o
KERNEL_OBJS-y += platform.o
KERNEL_OBJS-y += process.o
KERNEL_OBJS-y += reboot.o
KERNEL_OBJS-y += smp.o
KERNEL_OBJS-y += syscall.o
KERNEL_OBJS-y += task.o
KERNEL_OBJS-y += timer.o
KERNEL_OBJS-y += uaccess.o
KERNEL_OBJS-$(CONFIG_GCOV) += gcov.o
KERNEL_OBJS-$(CONFIG_GRINCH_GUEST) += guest.o
KERNEL_OBJS := $(KERNEL_OBJS-y)

KERNEL_OBJS := $(addprefix kernel/, $(KERNEL_OBJS))

OBJ_DIRS += $(dir $(KERNEL_OBJS))

kernel/built-in.a: $(KERNEL_OBJS)

kernel/syscall.o: kernel/syscall_table.c $(SYSCALL_HEADER)

$(SYSCALL_HEADER): $(srctree)/tools/mksyscalltbl $(srctree)/syscall.tbl
	$(QUIET) "[SYSCL]" $@
	$(VERBOSE) $(MKDIR_P) $(dir $@)
	$(VERBOSE) $^ header $@

kernel/syscall_table.c: $(srctree)/tools/mksyscalltbl $(srctree)/syscall.tbl
	$(QUIET) "[SYSCL]" $@
	$(VERBOSE) $(MKDIR_P) $(dir $@)
	$(VERBOSE) $^ source $@
