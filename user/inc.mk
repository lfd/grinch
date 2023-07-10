INCLUDES_USER = -Iuser/include -Iinclude_common -Iuser/lib/$(ARCH)/include

CFLAGS_USER = $(CFLAGS_COMMON) $(CFLAGS_ARCH) $(INCLUDES_USER)

LIBC_OBJS = user/lib/stdio.o user/lib/string.o user/lib/unistd.o user/lib/sched.o
LIBC_OBJS += user/lib/$(ARCH)/entry.o

user/lib/built-in.a: $(LIBC_OBJS)

user/%.o: user/%.c
	$(QUIET) "[CC-U]  $@"
	$(VERBOSE) $(CC) -c $(CFLAGS_USER) -o $@ $<

user/%.o: user/%.S $(GENERATED)
	$(QUIET) "[AS-U]  $@"
	$(VERBOSE) $(CC) -c $(AFLAGS_USER) $(CFLAGS_USER) -o $@ $<

define ld_app_user
	$(QUIET) "[LD-APP]$@"
	$(VERBOSE) $(LD) $(LDFLAGS_USER) --gc-sections -T $^ -o $@
	$(if $(V), $(SZ) --format=SysV -x $@)
endef

define ld_user
	$(QUIET) "[LD-U]  $@"
	$(VERBOSE) $(LD) $(LDFLAGS_USER) --whole-archive -relocatable -o $@ $<
endef

user/built-in.a: user/lib/built-in.a user/main.o

user/user.o: user/built-in.a
	$(call ld_user)

user/user.elf: user.ld user/user.o
	$(call ld_app_user)

clean_user:
	rm -rf user/*.{elf,o,a}
	rm -rf user/lib/*.{o,a}
	rm -rf user/lib/$(ARCH)/*.{o,a}
