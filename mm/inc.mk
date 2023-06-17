MM_OBJS = paging.o ioremap.o kmm.o vma.o mm.o

MM_OBJS := $(addprefix mm/, $(MM_OBJS))

mm/built-in.a: $(MM_OBJS)
