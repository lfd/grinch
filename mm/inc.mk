MM_OBJS = paging.o mm.o ioremap.o

MM_OBJS := $(addprefix $(MM_DIR)/, $(MM_OBJS))

mm/built-in.a: $(MM_OBJS)
