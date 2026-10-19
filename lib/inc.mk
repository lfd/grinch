LIB_OBJS-y = bitmap.o
LIB_OBJS-y += div64.o
LIB_OBJS-y += ctype.o
LIB_OBJS-y += errno.o
LIB_OBJS-y += fdt.o
LIB_OBJS-y += hexdump.o
LIB_OBJS-y += libgcc.o
LIB_OBJS-y += panic.o
LIB_OBJS-y += printk.o
LIB_OBJS-y += refcount.o
LIB_OBJS-y += ringbuf.o
LIB_OBJS-y += string.o
LIB_OBJS-y += strtox.o
LIB_OBJS-y += time.o
LIB_OBJS-y += ttp.o
LIB_OBJS-y += vsprintf.o

ifneq ($(BUILTIN_DTB),)
LIB_OBJS-y += dtb.o
lib/dtb.o: $(BUILTIN_DTB)
endif

LIB_OBJS := $(LIB_OBJS-y)

LIBFDT_OBJS = libfdt/fdt.o
LIBFDT_OBJS += libfdt/fdt_ro.o
LIBFDT_OBJS += libfdt/fdt_rw.o
LIBFDT_OBJS += libfdt/fdt_strerror.o
LIBFDT_OBJS += libfdt/fdt_sw.o
LIBFDT_OBJS += libfdt/fdt_addresses.o

LIB_OBJS := $(addprefix lib/, $(LIB_OBJS))
LIBFDT_OBJS := $(addprefix lib/, $(LIBFDT_OBJS))

OBJ_DIRS += $(dir $(LIB_OBJS) $(LIBFDT_OBJS))

lib/libfdt/built-in.a: $(LIBFDT_OBJS)

lib/built-in.a: lib/libfdt/built-in.a $(LIB_OBJS)
