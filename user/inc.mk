APPS=init jittertest hello

INCLUDES_USER = -Iuser/include -Icommon/include -Iuser/lib/$(ARCH)/include
CFLAGS_USER = $(CFLAGS_COMMON) $(CFLAGS_ARCH) $(CFLAGS_STANDALONE) $(INCLUDES_USER)
LDFLAGS_USER = $(LDFLAGS_COMMON) $(LDFLAGS_ARCH)

LIBC_OBJS = user/lib/ctype.o user/lib/errno.o user/lib/stdio.o
LIBC_OBJS += user/lib/sched.o user/lib/string.o user/lib/unistd.o
LIBC_OBJS += user/lib/vsprintf.o
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
	$(if $(V), $(SZ) --format=SysV -x $(1))
endef

define ld_user
	$(QUIET) "[LD-U]  $(1)"
	$(VERBOSE) $(LD) $(LDFLAGS_USER) --whole-archive -relocatable -o $(1) $(2)
endef

include $(patsubst %,user/apps/%/inc.mk,$(APPS))

UC = $(shell echo '$1' | tr '[:lower:]' '[:upper:]')

define define_app
clean_$(1):
	$(RMRF) user/apps/$(1)/*.{o,a,echse}

user/apps/$(1)/built-in.a: $(LIBC_BUILTIN) $($(call UC,$(1))_OBJS)

user/apps/$(1)/$(1)_linked.o: user/apps/$(1)/built-in.a
	$(call ld_user,$$@,$$^)

user/apps/$(1)/$(1).echse: user/user.ld user/apps/$(1)/$(1)_linked.o
	$(call ld_app_user,$$@,$$^)
endef

$(foreach app,$(APPS),$(eval $(call define_app,$(app))))

clean_user: $(patsubst %,clean_%,$(APPS))
	$(RMRF) user/user.ld
	$(RMRF) user/*.{elf,o,a}
	$(RMRF) user/lib/*.{o,a}
	$(RMRF) user/lib/$(ARCH)/*.{o,a}
	$(RMRF) user/initrd.cpio
	$(RMRF) user/dts/*.dts user/dts/*.dtb

define echse_of
	user/apps/$(1)/$(1).echse
endef

APP_ECHSES=$(foreach app,$(APPS),$(call echse_of,$(app)))

user/initrd.cpio: $(APP_ECHSES) kernel.bin user/dts/freechips,lfd-rocket.dtb user/dts/riscv-virtio,qemu.dtb
	$(QUIET) "[CPIO]  $@"
	$(VERBOSE) ./scripts/create_cpio $@ $^
