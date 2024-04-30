KERNEL_OBJS = bootparam.o console.o main.o memtest.o platform.o process.o
KERNEL_OBJS += smp.o syscall.o task.o timer.o uaccess.o

KERNEL_OBJS := $(addprefix kernel/, $(KERNEL_OBJS))

kernel/built-in.a: $(KERNEL_OBJS)

kernel/user.o: user/initrd.cpio

kernel/syscall.o: kernel/syscall_table.c $(SYSCALL_HEADER)

$(SYSCALL_HEADER): tools/mksyscalltbl syscall.tbl
	$(VERBOSE) mkdir -p common/include/generated
	$(QUIET) "[SYSCL]" $@
	$(VERBOSE) $^ header $@

kernel/syscall_table.c: tools/mksyscalltbl syscall.tbl
	$(QUIET) "[SYSCL]" $@
	$(VERBOSE) $^ source $@
