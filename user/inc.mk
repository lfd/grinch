APPS = cat
APPS += echo
APPS += env
APPS += gsh
APPS += init
APPS += jittertest
APPS += ls
APPS += mkdir
APPS += reboot
APPS += sleep
APPS += test
APPS += touch
APPS += true

INCLUDES_USER_SRC = -I$(srctree)/common/include \
		    -I$(srctree)/user/libc/include \
		    -I$(srctree)/user/libc/include/$(ARCH_SUPER)/ \
		    -I$(srctree)/user/libgrinch/include \
		    -I$(srctree)/common/include/arch/$(ARCH_SUPER)/

INCLUDES_USER_GEN = -I$(objtree)/common/include

INCLUDES_USER = $(INCLUDES_USER_SRC) $(INCLUDES_USER_GEN)

CFLAGS_USER = $(CFLAGS_COMMON) $(CFLAGS_ARCH) $(CFLAGS_STANDALONE) $(INCLUDES_USER)
AFLAGS_USER = $(AFLAGS_COMMON)
LDFLAGS_USER = $(LDFLAGS_COMMON) $(LDFLAGS_ARCH)

DIR_USER_BINARIES = user/apps/build

# libc stuff
LIBC_OBJS = $(ARCH_SUPER)/entry.o
LIBC_OBJS += ctype.o
LIBC_OBJS += dirent.o
LIBC_OBJS += div64.o
LIBC_OBJS += errno.o
LIBC_OBJS += fcntl.o
LIBC_OBJS += getauxval.o
LIBC_OBJS += ioctl.o
LIBC_OBJS += libgcc.o
LIBC_OBJS += reboot.o
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

LIBC_OBJS := $(addprefix user/libc/src/, $(LIBC_OBJS))

OBJ_DIRS += $(dir $(LIBC_OBJS))
LIBC_BUILTIN = user/libc/built-in.a

$(LIBC_BUILTIN): $(LIBC_OBJS)

# libgrinch
LIBGRINCH_OBJS += grinch.o
LIBGRINCH_OBJS += hexdump.o
# libgrinch framebuffer library
LIBGRINCH_OBJS += gfb/gfb.o
LIBGRINCH_OBJS += gfb/gimg.o
LIBGRINCH_OBJS += gfb/gpaint.o
LIBGRINCH_OBJS += gfb/font.o
LIBGRINCH_OBJS += gfb/fonts/font_acorn_8x8.o
LIBGRINCH_OBJS += gfb/fonts/font_6x8.o
LIBGRINCH_OBJS += gfb/fonts/font_6x10.o
LIBGRINCH_OBJS += gfb/fonts/font_7x14.o
LIBGRINCH_OBJS += gfb/fonts/font_10x18.o
LIBGRINCH_OBJS += gfb/fonts/font_mini_4x6.o
LIBGRINCH_OBJS += gfb/fonts/font_pearl_8x8.o
LIBGRINCH_OBJS += gfb/fonts/font_sun_12x22.o
LIBGRINCH_OBJS += gfb/fonts/font_sun_8x16.o
LIBGRINCH_OBJS += gfb/fonts/font_ter_16x32.o
LIBGRINCH_OBJS += gfb/fonts/font_vga_6x11.o
LIBGRINCH_OBJS += gfb/fonts/font_vga_8x8.o
LIBGRINCH_OBJS += gfb/fonts/font_vga_8x16.o

LIBGRINCH_OBJS := $(addprefix user/libgrinch/src/, $(LIBGRINCH_OBJS))

OBJ_DIRS += $(dir $(LIBGRINCH_OBJS))
LIBGRINCH_BUILTIN = user/libgrinch/built-in.a

$(LIBGRINCH_BUILTIN): $(LIBGRINCH_OBJS)

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

include $(patsubst %,$(srctree)/user/apps/%/inc.mk,$(APPS))

UC = $(shell echo '$1' | tr '[:lower:]' '[:upper:]')

define define_app
clean_$(1):
	$(call clean_files,user/apps/$(1),\
		$($(call UC,$(1))_OBJS)\
		user/apps/$(1)/built-in.a\
		user/apps/$(1)/$(1)_linked.o)

user/apps/$(1)/built-in.a: $(LIBC_BUILTIN) $(LIBGRINCH_BUILTIN) $($(call UC,$(1))_OBJS)

user/apps/$(1)/$(1)_linked.o: user/apps/$(1)/built-in.a
	$(call ld_user,$$@,$$^)

$(DIR_USER_BINARIES)/$(1): user/user.ld user/apps/$(1)/$(1)_linked.o
	$(call ld_app_user,$$@,$$^)
endef

$(foreach app,$(APPS),$(eval $(call define_app,$(app))))

OBJ_DIRS += $(foreach app,$(APPS),$(dir $($(call UC,$(app))_OBJS)))
OBJ_DIRS += $(DIR_USER_BINARIES)/

define app_of
	$(DIR_USER_BINARIES)/$(1)
endef

USER_APPS=$(foreach app,$(APPS),$(call app_of,$(app)))

IMAGES=res/logo.gimg

OBJ_DIRS += $(dir $(IMAGES))

user/initrd.cpio: $(USER_APPS) $(IMAGES) $(srctree)/res/test.txt grinch.bin
	$(QUIET) "[CPIO]  $@"
	$(VERBOSE) $(srctree)/scripts/create_cpio $@ $(DIR_USER_BINARIES) -- $(IMAGES) $(srctree)/res/test.txt grinch.bin

clean_user: $(patsubst %,clean_%,$(APPS))
	$(call clean_objects,user/libc,$(LIBC_OBJS))
	$(call clean_objects,user/libgrinch,$(LIBGRINCH_OBJS))
	$(call clean_files,user,user/user.ld user/initrd.cpio $(DIR_USER_BINARIES))
	$(call clean_files,res,$(IMAGES))
