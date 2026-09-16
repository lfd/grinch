VERSION=3
PATCHLEVEL=15
EXTRAVERSION=

PYTHON ?= python3

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

config_mk := $(objtree)/config.mk

no_config_goals := clean mrproper oldconfig defconfig %_defconfig menuconfig help test
goals := $(or $(MAKECMDGOALS),all)

# Building regenerates config.mk below and includes it there. Reading it here
# too would keep stale values, as a re-include cannot unset what the new file
# omits. Take only ARCH, needed before generation. Other goals do not
# regenerate and read the file as it stands.
ifeq ($(filter-out $(no_config_goals),$(goals)),)
-include $(config_mk)
else
config_arch := $(shell sed -n 's/^ARCH=//p' $(config_mk) 2>/dev/null)
endif

# config.mk is the source of truth once written; reject config tunables on
# the command line (CROSS_COMPILE, V=, QEMU_* are still fine).
ifneq ($(wildcard $(config_mk)),)
ifeq ($(filter defconfig %_defconfig,$(MAKECMDGOALS)),)
_config_overrides := $(filter ARCH=% PLATFORM=% OPT=% RISCV_MODE=% CONFIG_%,$(MAKEOVERRIDES))
ifneq ($(_config_overrides),)
$(error config.mk is locked; run 'make defconfig' to reconfigure. Refusing: $(_config_overrides))
endif
endif
endif

ARCH ?= $(or $(config_arch),riscv64)

# Architecture identification. Sets ARCH_SUPER/ARCH_DIR -- used below to
# locate the arch sources -- and per-arch flags. Needs only ARCH, so it runs
# before config generation.
ifeq ($(ARCH),riscv64)
ARCH_SUPER = riscv
UBOOT_ARCH = riscv
ARCH_RISCV = true
ARCH_RISCV64 = true
else ifeq ($(ARCH),riscv32)
ARCH_SUPER = riscv
UBOOT_ARCH = riscv
ARCH_RISCV = true
ARCH_RISCV32 = true
else ifeq ($(ARCH),arm64)
ARCH_SUPER = arm64
# U-Boot builds aarch64 under its "arm" architecture
UBOOT_ARCH = arm
ARCH_ARM64 = true
else
$(error Unsupported Architecture $(ARCH))
endif
ARCH_DIR = arch/$(ARCH_SUPER)

# Generated headers
generated_dir := include/generated
config_h      := $(generated_dir)/config.h
version_h     := $(generated_dir)/version.h
compile_h     := $(generated_dir)/compile.h

# Generates config.mk and config.h from the gConfig declarations; takes the
# command to run (oldconfig, defconfig, menuconfig) as its last argument.
gconfig = $(PYTHON) $(srctree)/scripts/gconfig.py \
          --config-mk $(config_mk) \
          --config-h $(objtree)/$(config_h)

# Seed a fresh config.mk from command-line tunables (ignored when config.mk
# already exists; --set only applies to keys not yet present).
gconfig += $(foreach ov,$(MAKEOVERRIDES),--set $(ov))

ifneq ($(filter-out $(no_config_goals),$(goals)),)
$(if $(shell $(gconfig) oldconfig),$(info [GEN]   config))
-include $(config_mk)
endif

all: grinch.bin user/initrd.cpio tools

# Delete a target whose recipe failed, so an interrupted or failed build
# never leaves a truncated file with an up-to-date timestamp behind.
.DELETE_ON_ERROR:

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
RM=rm
RMF=$(RM) -f
RMRF=$(RMF) -r

D_UBOOT=$(realpath $(srctree)/res/u-boot)
UBOOT_PFX=$(objtree)/res/u-boot/u-boot-$(ARCH)-$(PLATFORM)
UBOOT_BIN=$(UBOOT_PFX)/u-boot-nodtb.bin
MAKEARGS_UBOOT=CROSS_COMPILE=$(CROSS_COMPILE) ARCH=$(UBOOT_ARCH)

ifdef V
QUIET := @true
VERBOSE :=
else
QUIET := @echo
VERBOSE := @
endif


AFLAGS_COMMON=-D__ASSEMBLY__

CFLAGS_STANDALONE=-nostdinc -ffreestanding
ifeq ($(CONFIG_DEBUG), y)
CFLAGS_STANDALONE += -g -ggdb
endif
ifeq ($(CONFIG_INITCONST_STR), y)
CFLAGS_STANDALONE += -Wno-format-security
else
CFLAGS_STANDALONE += -Wformat-security
endif

ifeq ($(OPT),none)
OPT_FLAG := -O0
else ifeq ($(OPT),speed)
OPT_FLAG := -O2
else ifeq ($(OPT),size)
OPT_FLAG := -Os
else ifeq ($(OPT),release)
OPT_FLAG := -O3
else
OPT_FLAG := -O0
endif

CFLAGS_COMMON=$(OPT_FLAG) \
              -fno-strict-aliasing \
              -fno-omit-frame-pointer -fno-stack-protector \
              -ffunction-sections -fdata-sections \
              -Wall -Wextra -Wno-unused-parameter \
              -Wstrict-prototypes -Wtype-limits \
              -Wmissing-declarations -Wmissing-prototypes \
              -Wnested-externs -Wshadow -Wredundant-decls \
              -Wundef -Wdeprecated -Werror \
              -include $(objtree)/$(config_h)

# -MMD emits the header deps of each object to a .d file; -MP adds phony
# targets so a later-removed header doesn't break the build.
DEPFLAGS = -MMD -MP

LDFLAGS_COMMON=

define clean_objects
	$(QUIET) "[CLEAN]" $1
	$(VERBOSE) $(RMF) $(1)/built-in.a $(2) $(2:.o=.gcno) $(2:.o=.gcda) $(2:.o=.d)
endef

define clean_files
	$(QUIET) "[CLEAN]" $1
	$(VERBOSE) $(RMF) $(2)
endef

define clean_dir
	$(QUIET) "[CLEAN]" $1
	$(VERBOSE) $(RMRF) $(1)
endef

include $(srctree)/scripts/kernel.mk
include $(srctree)/user/inc.mk
include $(srctree)/tools/inc.mk

# Kept fresh at parse time above; this rule only recreates the header if it
# went missing mid-build (e.g. 'make clean all').
$(config_h):
	$(QUIET) "[GEN]   $@"
	$(VERBOSE) $(gconfig) oldconfig >/dev/null

%.bin: %.elf
	$(QUIET) "[OBJC]  $@"
	$(VERBOSE) $(OBJCOPY) -O binary $^ $@

%/built-in.a:
	$(QUIET) "[AR]    $@"
	$(VERBOSE) rm -f $@
	$(VERBOSE) $(AR) cDPrST $@ $^

QEMU_CMD=$(QEMU) $(QEMU_ARGS_COMMON) $(QEMU_ARGS)

# -append needs -kernel (qemu folds it into the DTB), so it rides with each -kernel.
QEMU_CMD_DIRECT=$(QEMU_CMD) -kernel grinch.bin -initrd user/initrd.cpio -append '$(QEMU_APPEND)'
QEMU_CMD_UBOOT=$(QEMU_CMD) $(QEMU_UBOOT_ARGS)

qemu: all
	$(QEMU_CMD_DIRECT)

qemux: QEMU_DISPLAY=sdl
qemux: qemu

# The same machine with the modes below machine mode switched off.
qemum: QEMU_CPU_FLAGS=h=false,s=false,u=false
qemum: qemu

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
	$(GDB) -nx -x $(srctree)/scripts/connect.gdb -x $<

grinch.info: grinch.dump tools/gcov_extract
	./tools/gcov_extract $<
	lcov -c -d . -o $@

gcov: grinch.info
	$(MKDIR_P) gcov
	genhtml $< -o gcov/

$(UBOOT_BIN):
	$(MKDIR_P) $(UBOOT_PFX)
	cp -av $(srctree)/$(UBOOT_CFG) $(UBOOT_PFX)/.config
	cp -av $(srctree)/$(UBOOT_ENV) $(UBOOT_PFX)/grinch.env
	$(MAKE) -C $(D_UBOOT) $(MAKEARGS_UBOOT) O=$(UBOOT_PFX) olddefconfig
	$(MAKE) -C $(D_UBOOT) $(MAKEARGS_UBOOT) O=$(UBOOT_PFX) u-boot-nodtb.bin

.PHONY: test
test:
	$(srctree)/tests/run.py

.PHONY: oldconfig defconfig
oldconfig defconfig:
	$(QUIET) "[GEN]   $(config_mk)"
	$(VERBOSE) $(gconfig) $@ >/dev/null

# Seed config.mk from a preset in configs/, then fill in the rest.
%_defconfig:
	$(QUIET) "[GEN]   $(config_mk)"
	$(VERBOSE) cp $(srctree)/configs/$@ $(config_mk)
	$(VERBOSE) $(gconfig) oldconfig >/dev/null

.PHONY: menuconfig
menuconfig:
	$(VERBOSE) $(gconfig) menuconfig

debug: grinch.elf
	$(GDB) -nx -x $(srctree)/scripts/connect.gdb -x $(srctree)/scripts/debug.gdb

clean: clean_core clean_lib clean_mm clean_fs clean_user clean_arch clean_drivers clean_kernel clean_tools
	$(call clean_files,all,grinch.bin grinch.elf grinch.dump grinch.info)
	$(call clean_dir,gcov)

mrproper: clean
	$(call clean_dir,$(UBOOT_PFX))
	$(call clean_files,config,$(config_mk))

OBJ_DIRS := $(sort $(OBJ_DIRS))
$(shell $(MKDIR_P) $(OBJ_DIRS))

# Pull in the header dependencies emitted by -MMD so that touching a
# header rebuilds every object that includes it.
-include $(wildcard $(addsuffix *.d,$(OBJ_DIRS)))

endif # in-objtree else branch
