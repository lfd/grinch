LIB_OBJS = bitmap.o ctype.o errno.o fdt.o hexdump.o panic.o printk.o
LIB_OBJS += ringbuf.o string.o strtox.o vsprintf.o
LIBFDT_OBJS = libfdt/fdt.o libfdt/fdt_ro.o libfdt/fdt_addresses.o

LIB_OBJS := $(addprefix lib/, $(LIB_OBJS))
LIBFDT_OBJS := $(addprefix lib/, $(LIBFDT_OBJS))


lib/libfdt/built-in.a: $(LIBFDT_OBJS)

lib/built-in.a: lib/libfdt/built-in.a $(LIB_OBJS)
