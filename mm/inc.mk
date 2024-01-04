MM_OBJS = alloc.o gfp.o paging.o ioremap.o mm.o vma.o

MM_OBJS := $(addprefix mm/, $(MM_OBJS))

mm/built-in.a: $(MM_OBJS)
