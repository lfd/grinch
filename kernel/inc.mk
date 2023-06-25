KERNEL_OBJS = main.o syscall.o task.o uaccess.o

KERNEL_OBJS := $(addprefix kernel/, $(KERNEL_OBJS))

kernel/built-in.a: $(KERNEL_OBJS)
