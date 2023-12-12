KERNEL_OBJS = main.o process.o syscall.o task.o uaccess.o user.o vfs.o

KERNEL_OBJS := $(addprefix kernel/, $(KERNEL_OBJS))

kernel/built-in.a: $(KERNEL_OBJS)

kernel/user.o: user/initrd.cpio
