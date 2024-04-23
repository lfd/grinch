MM_OBJS = alloc.o
MM_OBJS += gfp.o
MM_OBJS += paging.o
MM_OBJS += salloc.o
MM_OBJS += ioremap.o
MM_OBJS += mm.o
MM_OBJS += vma.o

MM_OBJS := $(addprefix mm/, $(MM_OBJS))

mm/built-in.a: $(MM_OBJS)
