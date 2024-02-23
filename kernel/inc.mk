KERNEL_OBJS = bootparam.o main.o memtest.o process.o syscall.o task.o uaccess.o
KERNEL_OBJS += smp.o timer.o

KERNEL_OBJS := $(addprefix kernel/, $(KERNEL_OBJS))

kernel/built-in.a: $(KERNEL_OBJS)

kernel/user.o: user/initrd.cpio
