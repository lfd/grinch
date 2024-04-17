FS_OBJS = devfs.o
FS_OBJS += initrd.o
FS_OBJS += syscall.o
FS_OBJS += vfs.o
FS_OBJS += util.o

FS_OBJS := $(addprefix fs/, $(FS_OBJS))

fs/built-in.a: $(FS_OBJS)
