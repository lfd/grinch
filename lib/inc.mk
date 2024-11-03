LIB_OBJS = bitmap.o
LIB_OBJS += ctype.o
LIB_OBJS += errno.o
LIB_OBJS += fdt.o
LIB_OBJS += hexdump.o
LIB_OBJS += panic.o
LIB_OBJS += printk.o
LIB_OBJS += ringbuf.o
LIB_OBJS += string.o
LIB_OBJS += strtox.o
LIB_OBJS += time.o
LIB_OBJS += vsprintf.o

LIBFDT_OBJS = libfdt/fdt.o
LIBFDT_OBJS += libfdt/fdt_ro.o
LIBFDT_OBJS += libfdt/fdt_strerror.o
LIBFDT_OBJS += libfdt/fdt_sw.o
LIBFDT_OBJS += libfdt/fdt_addresses.o

LIB_OBJS := $(addprefix lib/, $(LIB_OBJS))
LIBFDT_OBJS := $(addprefix lib/, $(LIBFDT_OBJS))


lib/libfdt/built-in.a: $(LIBFDT_OBJS)

lib/built-in.a: lib/libfdt/built-in.a $(LIB_OBJS)

clean_lib:
	$(call clean_objects,lib/libfdt,$(LIBFDT_OBJS))
	$(call clean_objects,lib,$(LIB_OBJS))
