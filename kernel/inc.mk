KERNEL_OBJS = bootparam.o main.o memtest.o platform.o process.o syscall.o task.o
KERNEL_OBJS += uaccess.o smp.o timer.o

KERNEL_OBJS := $(addprefix kernel/, $(KERNEL_OBJS))

kernel/built-in.a: $(KERNEL_OBJS)

kernel/user.o: user/initrd.cpio
