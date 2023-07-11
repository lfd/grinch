LIB_OBJS = panic.o printk.o string.o bitmap.o fdt.o strtox.o ctype.o
LIBFDT_OBJS = libfdt/fdt.o libfdt/fdt_ro.o libfdt/fdt_addresses.o

LIB_OBJS := $(addprefix lib/, $(LIB_OBJS))
LIBFDT_OBJS := $(addprefix lib/, $(LIBFDT_OBJS))


lib/libfdt/built-in.a: $(LIBFDT_OBJS)

lib/built-in.a: lib/libfdt/built-in.a $(LIB_OBJS)
