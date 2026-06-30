VERSION=3
PATCHLEVEL=15
EXTRAVERSION=

# Supported architectures:
#  - riscv64
#  - riscv32 (no SMP)

this-makefile := $(lastword $(MAKEFILE_LIST))
abs_srctree   := $(realpath $(dir $(this-makefile)))
abs_output    := $(CURDIR)

ifneq ($(grinch_sub_make_done),1)

ifeq ("$(origin O)", "command line")
  KBUILD_OUTPUT := $(O)
endif

ifneq ($(KBUILD_OUTPUT),)
  $(shell mkdir -p $(KBUILD_OUTPUT))
  abs_output := $(realpath $(KBUILD_OUTPUT))
  $(if $(abs_output),,$(error failed to create output directory "$(KBUILD_OUTPUT)"))
  $(shell test -f $(abs_output)/Makefile || $(abs_srctree)/scripts/mkmakefile $(abs_srctree) $(abs_output))
endif

export grinch_sub_make_done := 1

endif # grinch_sub_make_done

ifneq ($(abs_output),$(CURDIR))

# Refuse to do an out-of-tree build against a polluted source tree:
# VPATH would silently satisfy targets from leftover in-tree artefacts.
ifneq ($(wildcard $(abs_srctree)/config.mk),)
$(error in-tree build artefacts found in $(abs_srctree); run 'make mrproper' there first)
endif

.PHONY: __sub-make
$(filter-out $(this-makefile), $(MAKECMDGOALS)) __all: __sub-make
	@:
__sub-make:
	@$(MAKE) -C $(abs_output) -f $(abs_srctree)/Makefile $(MAKECMDGOALS)

else # in objtree

ifeq ($(abs_srctree),$(CURDIR))
srctree := .
else
srctree := $(abs_srctree)
endif
objtree := $(CURDIR)

VPATH := $(srctree)

# Persisted build settings. Created on first invocation with current
# defaults merged with any command-line overrides; never overwritten
# after that. Hand-edit, or run 'make mrproper' to reset.
arch_vars    := ARCH CROSS_COMPILE PLATFORM
build_vars   := OPT GCOV DEBUG_OUTPUT INITCONST_STR
qemu_vars    := QEMU_CPUS QEMU_APPEND QEMU_DISPLAY
tracked_vars := $(arch_vars) $(build_vars) $(qemu_vars)
config_mk    := $(objtree)/config.mk
-include $(config_mk)

# Architecture. CROSS_COMPILE and PLATFORM defaults live in
# arch/$(ARCH)/inc.mk because they derive from ARCH.
ARCH ?= riscv64

# Build options
OPT ?= -O0
#V=1
#DEBUG_OUTPUT=1
#INITCONST_STR=1
#GCOV=1

# QEMU runtime
QEMU_CPUS ?= 2
QEMU_APPEND ?= ""
QEMU_DISPLAY ?= none

all: grinch.bin user/initrd.cpio tools

HOSTCC=gcc

DTC=dtc
GDB=$(CROSS_COMPILE)gdb
CC=$(CROSS_COMPILE)gcc
LD=$(CROSS_COMPILE)ld
AR=$(CROSS_COMPILE)ar
AS=$(CROSS_COMPILE)as
OBJDUMP=$(CROSS_COMPILE)objdump
OBJCOPY=$(CROSS_COMPILE)objcopy
SZ=$(CROSS_COMPILE)size
MKDIR=mkdir
MKDIR_P=$(MKDIR) -p
RMRF=rm -rf

D_UBOOT=$(realpath $(srctree)/res/u-boot)
UBOOT_PFX=$(objtree)/res/u-boot/u-boot-$(ARCH)-$(PLATFORM)
UBOOT_BIN=$(UBOOT_PFX)/u-boot-nodtb.bin
MAKEARGS_UBOOT=CROSS_COMPILE=$(CROSS_COMPILE) ARCH=$(ARCH_SUPER)

ifdef V
QUIET := @true
VERBOSE :=
else
QUIET := @echo
VERBOSE := @
endif


AFLAGS_COMMON=-D__ASSEMBLY__

CFLAGS_STANDALONE=-nostdinc -ffreestanding -g -ggdb
ifeq ($(INITCONST_STR), 1)
CFLAGS_STANDALONE += -Wno-format-security -DINITCONST_STR
else
CFLAGS_STANDALONE += -Wformat-security
endif

CFLAGS_COMMON=$(OPT) \
              -fno-strict-aliasing \
              -fno-omit-frame-pointer -fno-stack-protector \
              -ffunction-sections -fdata-sections \
              -Wall -Wextra -Wno-unused-parameter \
              -Wstrict-prototypes -Wtype-limits \
              -Wmissing-declarations -Wmissing-prototypes \
              -Wnested-externs -Wshadow -Wredundant-decls \
              -Wundef -Wdeprecated -Werror

LDFLAGS_COMMON=

ifeq ($(DEBUG_OUTPUT), 1)
CFLAGS_COMMON += -DDEBUG
endif

define clean_objects
	$(QUIET) "[CLEAN]" $1
	$(VERBOSE) $(RMRF) $(1)/built-in.a $(2) $(2:.o=.gcno) $(2:.o=.gcda)
endef

define clean_file
	$(QUIET) "[CLEAN]" $1
	$(VERBOSE) $(RMRF) $(1)
endef

define clean_files
	$(QUIET) "[CLEAN]" $1
	$(VERBOSE) $(RMRF) $(2)
endef

# Emit one group of persisted settings (header + assignments) for the
# config.mk writer. $(1) is the group label, $(2) is the var list.
# Single-line on purpose so it works in both $(shell) and recipes.
emit_group = echo; echo '\# $(1)'; $(foreach v,$(2),echo '$(v) := $($(v))';)

# Shell command that (re)writes config.mk in one go. Used both at parse
# time (auto-create on first invocation) and from the defconfig recipe.
config_mk_cmd = { \
	echo '\# Auto-generated. Edit to change settings; run mrproper to reset.'; \
	$(call emit_group,Architecture,$(arch_vars)) \
	$(call emit_group,Build options,$(build_vars)) \
	$(call emit_group,QEMU runtime,$(qemu_vars)) \
} > $(config_mk).new && mv -f $(config_mk).new $(config_mk)

include $(srctree)/scripts/kernel.mk
include $(srctree)/user/inc.mk
include $(srctree)/tools/inc.mk

# Auto-create config.mk for any goal that implies a real build.
# Passive goals (clean, mrproper, defconfig, help) are listed below
# and skip parse-time generation; defconfig has its own recipe.
no_config_goals := clean mrproper defconfig help
goals := $(or $(MAKECMDGOALS),all)
ifneq ($(filter-out $(no_config_goals),$(goals)),)
ifeq ($(wildcard $(config_mk)),)
$(if $(V),$(info $(config_mk_cmd)),$(info [GEN]   $(config_mk)))
$(shell $(config_mk_cmd))
endif
endif

%.bin: %.elf
	$(QUIET) "[OBJC]  $@"
	$(VERBOSE) $(OBJCOPY) -O binary $^ $@

%/built-in.a:
	$(QUIET) "[AR]    $@"
	$(VERBOSE) rm -f $@
	$(VERBOSE) $(AR) cDPrST $@ $^

QEMU_CMD=$(QEMU) $(QEMU_ARGS_COMMON) $(QEMU_ARGS) -append $(QEMU_APPEND)

QEMU_CMD_DIRECT=$(QEMU_CMD) -kernel grinch.bin -initrd user/initrd.cpio
QEMU_CMD_UBOOT=$(QEMU_CMD) -kernel $(UBOOT_BIN) $(QEMU_UBOOT_ARGS)

qemu: all
	$(QEMU_CMD_DIRECT)

qemux: QEMU_DISPLAY=sdl
qemux: qemu

qemuu: all $(UBOOT_BIN)
	$(QEMU_CMD_UBOOT)

qemudb: all
	$(QEMU_CMD_DIRECT) -S

qemuudb: all $(UBOOT_BIN)
	$(QEMU_CMD_UBOOT) -S

qemu.dts: grinch.bin user/initrd.cpio
	$(QEMU_CMD_DIRECT) $(QEMU_MACHINE),dumpdtb=/tmp/qemu_tmp.dtb
	dtc -I dtb -O dts /tmp/qemu_tmp.dtb -o $@
	rm -f /tmp/qemu_tmp.dtb

.PHONY: grinch.dump
grinch.dump: $(srctree)/scripts/grinch_dump.gdb grinch.elf
	$(GDB) -nx -x $(srctree)/.gdbinit -x $<

grinch.info: grinch.dump tools/gcov_extract
	./tools/gcov_extract $<
	lcov -c -d . -o $@

gcov: grinch.info
	$(MKDIR_P) gcov
	genhtml $< -o gcov/

$(UBOOT_BIN):
	$(MKDIR_P) $(UBOOT_PFX)
	cp -av $(srctree)/res/u-boot/$(UBOOT_CFG) $(UBOOT_PFX)/.config
	$(MAKE) -C $(D_UBOOT)/u-boot $(MAKEARGS_UBOOT) O=$(UBOOT_PFX) oldconfig
	$(MAKE) -C $(D_UBOOT)/u-boot $(MAKEARGS_UBOOT) O=$(UBOOT_PFX) u-boot-nodtb.bin

.PHONY: defconfig
defconfig:
	$(QUIET) "[GEN]   $(config_mk)"
	$(VERBOSE) $(config_mk_cmd)

debug: grinch.bin
	$(GDB) -x $(srctree)/scripts/debug.gdb $^

clean: clean_core clean_lib clean_mm clean_fs clean_user clean_arch clean_drivers clean_kernel clean_tools
	$(call clean_files,all,grinch.bin grinch.elf grinch.dump grinch.info gcov)

mrproper: clean
	$(call clean_file,$(UBOOT_PFX))
	$(call clean_file,$(config_mk))

OBJ_DIRS := $(sort $(OBJ_DIRS))
$(shell $(MKDIR_P) $(OBJ_DIRS))

endif # in-objtree else branch
