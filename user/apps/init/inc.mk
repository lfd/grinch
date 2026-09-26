INIT_OBJS-y = user/apps/init/main.o
INIT_OBJS-$(CONFIG_FB) += user/apps/init/fb.o
INIT_OBJS = $(INIT_OBJS-y)
