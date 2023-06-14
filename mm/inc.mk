MM_OBJS = paging.o mm.o ioremap.o

MM_OBJS := $(addprefix mm/, $(MM_OBJS))

mm/built-in.a: $(MM_OBJS)
