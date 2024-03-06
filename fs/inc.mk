FS_OBJS = devfs.o initrd.o syscall.o vfs.o

FS_OBJS := $(addprefix fs/, $(FS_OBJS))

fs/built-in.a: $(FS_OBJS)
