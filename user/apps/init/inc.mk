APP=init

APP_OBJS=main.o

user/apps/$(APP)/built-in.a: $(LIBC_BUILTIN) user/apps/$(APP)/main.o

user/apps/$(APP)/$(APP)_linked.o: user/apps/$(APP)/built-in.a
	$(call ld_user)

user/apps/$(APP)/$(APP).echse: user.ld user/apps/$(APP)/$(APP)_linked.o
	$(call ld_app_user)

clean_init:
	rm -rf user/apps/$(APP)/*.{o,a,echse}
