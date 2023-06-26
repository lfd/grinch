KERNEL_OBJS = main.o syscall.o

KERNEL_OBJS := $(addprefix kernel/, $(KERNEL_OBJS))

kernel/built-in.a: $(KERNEL_OBJS)
