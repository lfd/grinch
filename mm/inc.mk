MM_OBJS-y = alloc.o
MM_OBJS-y += gfp.o
MM_OBJS-y += salloc.o
MM_OBJS-y += mm.o
MM_OBJS-y += vma.o
MM_OBJS-$(CONFIG_MMU) += asid.o
MM_OBJS-$(CONFIG_MMU) += ioremap.o
MM_OBJS-$(CONFIG_MMU) += paging.o
MM_OBJS := $(MM_OBJS-y)

MM_OBJS := $(addprefix mm/, $(MM_OBJS))

OBJ_DIRS += $(dir $(MM_OBJS))

mm/built-in.a: $(MM_OBJS)
