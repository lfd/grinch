APPS = cat
APPS += echo
APPS += env
APPS += gsh
APPS += init
APPS += jittertest
APPS += ls
APPS += mkdir
APPS += sleep
APPS += test
APPS += touch
APPS += true

INCLUDES_USER = -Iuser/include -Icommon/include -Iuser/lib/$(ARCH)/include
CFLAGS_USER = $(CFLAGS_COMMON) $(CFLAGS_ARCH) $(CFLAGS_STANDALONE) $(INCLUDES_USER)
LDFLAGS_USER = $(LDFLAGS_COMMON) $(LDFLAGS_ARCH)

# libc stuff
LIBC_OBJS = $(ARCH)/entry.o
LIBC_OBJS += ctype.o
LIBC_OBJS += dirent.o
LIBC_OBJS += errno.o
LIBC_OBJS += fcntl.o
LIBC_OBJS += ioctl.o
LIBC_OBJS += salloc.o
LIBC_OBJS += stdio.o
LIBC_OBJS += sched.o
LIBC_OBJS += stat.o
LIBC_OBJS += startup.o
LIBC_OBJS += stdlib.o
LIBC_OBJS += string.o
LIBC_OBJS += strtox.o
LIBC_OBJS += time.o
LIBC_OBJS += unistd.o
LIBC_OBJS += vsprintf.o
LIBC_OBJS += wait.o

# grinch-specific stuff
LIBC_OBJS += grinch/gfb/gfb.o
LIBC_OBJS += grinch/gfb/gimg.o
LIBC_OBJS += grinch/grinch.o
LIBC_OBJS += grinch/hexdump.o

LIBC_OBJS := $(addprefix user/lib/, $(LIBC_OBJS))

LIBC_BUILTIN = user/lib/built-in.a

$(LIBC_BUILTIN): $(LIBC_OBJS)

%.gimg: ./scripts/img2gimg %.png
	$(QUIET) "[GIMG]  $@"
	$(VERBOSE) $^ $@

user/%.o: user/%.c $(SYSCALL_HEADER)
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
	$(call clean_files,user/apps/$(1),\
		$($(call UC,$(1))_OBJS)\
		user/apps/$(1)/built-in.a\
		user/apps/$(1)/$(1)_linked.o)

user/apps/$(1)/built-in.a: $(LIBC_BUILTIN) $($(call UC,$(1))_OBJS)

user/apps/$(1)/$(1)_linked.o: user/apps/$(1)/built-in.a
	$(call ld_user,$$@,$$^)

user/apps/build/$(1): user/user.ld user/apps/$(1)/$(1)_linked.o
	$(VERBOSE) mkdir -p user/apps/build
	$(call ld_app_user,$$@,$$^)
endef

$(foreach app,$(APPS),$(eval $(call define_app,$(app))))

define app_of
	user/apps/build/$(1)
endef

USER_APPS=$(foreach app,$(APPS),$(call app_of,$(app)))

IMAGES=res/logo.gimg

user/initrd.cpio: $(USER_APPS) $(IMAGES) res/test.txt kernel.bin
	$(QUIET) "[CPIO]  $@"
	$(VERBOSE) ./scripts/create_cpio $@ $^

clean_user: $(patsubst %,clean_%,$(APPS))
	$(call clean_objects,user/lib,$(LIBC_OBJS))
	$(call clean_files,user,user/user.ld user/initrd.cpio user/apps/build)
	$(call clean_files,res,$(IMAGES))
