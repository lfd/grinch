MM_OBJS = alloc.o paging.o ioremap.o kmm.o pmm.o mm.o vma.o

MM_OBJS := $(addprefix mm/, $(MM_OBJS))

mm/built-in.a: $(MM_OBJS)
