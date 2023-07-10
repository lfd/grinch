INCLUDES_USER = -Iuser/include -Iinclude_common -Iuser/lib/$(ARCH)/include

CFLAGS_USER = $(CFLAGS_COMMON) $(CFLAGS_ARCH) $(INCLUDES_USER)

LIBC_OBJS = user/lib/stdio.o user/lib/string.o user/lib/unistd.o user/lib/sched.o
LIBC_OBJS += user/lib/$(ARCH)/entry.o

LIBC_BUILTIN = user/lib/built-in.a

$(LIBC_BUILTIN): $(LIBC_OBJS)

user/%.o: user/%.c
	$(QUIET) "[CC-U]  $@"
	$(VERBOSE) $(CC) -c $(CFLAGS_USER) -o $@ $<

user/%.o: user/%.S $(GENERATED)
	$(QUIET) "[AS-U]  $@"
	$(VERBOSE) $(CC) -c $(AFLAGS_USER) $(CFLAGS_USER) -o $@ $<

define ld_app_user
	$(QUIET) "[LD-APP]$(1)"
	$(VERBOSE) $(LD) $(LDFLAGS_USER) --gc-sections -T $(2) -o $(1)
	$(if $(V), $(SZ) --format=SysV -x $@)
endef

define ld_user
	$(QUIET) "[LD-U]  $(1)"
	$(VERBOSE) $(LD) $(LDFLAGS_USER) --whole-archive -relocatable -o $(1) $(2)
endef

APPS=init hello

include $(patsubst %,user/apps/%/inc.mk,$(APPS))

UC = $(shell echo '$1' | tr '[:lower:]' '[:upper:]')

define define_app
clean_$(1):
	rm -rf user/apps/$(1)/*.{o,a,echse}

user/apps/$(1)/built-in.a: $(LIBC_BUILTIN) $($(call UC,$(1))_OBJS)

user/apps/$(1)/$(1)_linked.o: user/apps/$(1)/built-in.a
	$(call ld_user,$$@,$$^)

user/apps/$(1)/$(1).echse: user.ld user/apps/$(1)/$(1)_linked.o
	$(call ld_app_user,$$@,$$^)
endef

$(foreach app,$(APPS),$(eval $(call define_app,$(app))))

clean_user: $(patsubst %,clean_%,$(APPS))
	rm -rf user/*.{elf,o,a}
	rm -rf user/lib/*.{o,a}
	rm -rf user/lib/$(ARCH)/*.{o,a}
