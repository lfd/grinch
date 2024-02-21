LIB_OBJS = bitmap.o ctype.o fdt.o hexdump.o panic.o printk.o string.o strtox.o
LIB_OBJS += vsprintf.o
LIBFDT_OBJS = libfdt/fdt.o libfdt/fdt_ro.o libfdt/fdt_addresses.o

LIB_OBJS := $(addprefix lib/, $(LIB_OBJS))
LIBFDT_OBJS := $(addprefix lib/, $(LIBFDT_OBJS))


lib/libfdt/built-in.a: $(LIBFDT_OBJS)

lib/built-in.a: lib/libfdt/built-in.a $(LIB_OBJS)
