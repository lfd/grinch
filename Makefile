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
arch_vars     := ARCH PLATFORM
compiler_vars := CROSS_COMPILE OPT
build_vars    := CONFIG_GCOV CONFIG_DEBUG_OUTPUT CONFIG_INITCONST_STR CONFIG_VMM
qemu_vars     := QEMU_CPUS QEMU_APPEND QEMU_DISPLAY QEMU_SERIAL
tracked_vars  := $(arch_vars) $(compiler_vars) $(build_vars) $(qemu_vars)
# Free-form string values: persisted in double quotes, unquoted again
# right after inclusion. Flag-style options (CONFIG_*) stay bare.
string_vars   := ARCH PLATFORM CROSS_COMPILE OPT QEMU_APPEND QEMU_DISPLAY QEMU_SERIAL
config_mk     := $(objtree)/config.mk
-include $(config_mk)
$(foreach v,$(string_vars),$(if $($(v)),$(eval $(v) := $(patsubst "%",%,$($(v))))))

ARCH ?= riscv64
PLATFORM ?= any

# Compiler
CROSS_COMPILE ?= $(ARCH)-unknown-linux-gnu-
OPT ?= -O0

# Build options
CONFIG_VMM ?= 1
#V=1
#CONFIG_DEBUG_OUTPUT=1
#CONFIG_INITCONST_STR=1
#CONFIG_GCOV=1

# QEMU runtime
QEMU_CPUS ?= 2
QEMU_APPEND ?=
QEMU_DISPLAY ?= none
QEMU_SERIAL ?= stdio

# Generated headers
generated_dir := include/generated
config_h      := $(generated_dir)/config.h
version_h     := $(generated_dir)/version.h
compile_h     := $(generated_dir)/compile.h

# Options consumed by the source world. Each NAME=VALUE entry ends up
# as '#define NAME VALUE' in $(config_h), which is force-included into
# every compilation unit instead of passing a pile of -D options.
# Makefiles below (arch, kernel, ...) append their own entries.
config_defines := CONFIG_ARCH="$(ARCH)"

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
RM=rm
RMF=$(RM) -f
RMRF=$(RMF) -r

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
ifeq ($(CONFIG_INITCONST_STR), 1)
CFLAGS_STANDALONE += -Wno-format-security
config_defines += CONFIG_INITCONST_STR=1
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
              -Wundef -Wdeprecated -Werror \
              -include $(objtree)/$(config_h)

LDFLAGS_COMMON=

ifeq ($(CONFIG_DEBUG_OUTPUT), 1)
config_defines += CONFIG_DEBUG_OUTPUT=1
endif

ifeq ($(CONFIG_VMM), 1)
config_defines += CONFIG_VMM=1
endif

define clean_objects
	$(QUIET) "[CLEAN]" $1
	$(VERBOSE) $(RMF) $(1)/built-in.a $(2) $(2:.o=.gcno) $(2:.o=.gcda)
endef

define clean_files
	$(QUIET) "[CLEAN]" $1
	$(VERBOSE) $(RMF) $(2)
endef

define clean_dir
	$(QUIET) "[CLEAN]" $1
	$(VERBOSE) $(RMRF) $(1)
endef

# Emit one group of persisted settings (header + assignments) for the
# config.mk writer. $(1) is the group label, $(2) is the var list.
# Single-line on purpose so it works in both $(shell) and recipes.
emit_group = echo; echo '\# $(1)'; $(foreach v,$(2),echo '$(v)=$(if $(filter $(v),$(string_vars)),"$($(v))",$($(v)))';)

# Shell command that (re)writes config.mk in one go. Used both at parse
# time (auto-create on first invocation) and from the defconfig recipe.
config_mk_cmd = { \
	echo '\# Auto-generated. Edit to change settings; run mrproper to reset.'; \
	$(call emit_group,Architecture,$(arch_vars)) \
	$(call emit_group,Compiler,$(compiler_vars)) \
	$(call emit_group,Build options,$(build_vars)) \
	$(call emit_group,QEMU runtime,$(qemu_vars)) \
} > $(config_mk).new && mv -f $(config_mk).new $(config_mk)

# Shell command that (re)writes the generated config header from the
# config_defines list. Runs at parse time on every real build; the file
# is only touched when its content changes, so flipping an option
# rebuilds the tree while a no-op run rebuilds nothing.
# make does not unescape \# inside function calls, hence the variable.
pound := \#
config_h_cmd = $(MKDIR_P) $(dir $(config_h)) && { \
	echo '/* Auto-generated from config.mk. Do not edit. */'; \
	$(foreach d,$(config_defines),echo '$(pound)define $(subst =, ,$(d))';) \
} > $(config_h).new && \
	if cmp -s $(config_h).new $(config_h); then $(RMF) $(config_h).new; \
	else mv -f $(config_h).new $(config_h); echo updated; fi

include $(srctree)/scripts/kernel.mk
include $(srctree)/user/inc.mk
include $(srctree)/tools/inc.mk

# Auto-create config.mk for any goal that implies a real build.
# Passive goals (clean, mrproper, defconfig, help) are listed below
# and skip parse-time generation; defconfig has its own recipe.
no_config_goals := clean mrproper defconfig help test
goals := $(or $(MAKECMDGOALS),all)
ifneq ($(filter-out $(no_config_goals),$(goals)),)
ifeq ($(wildcard $(config_mk)),)
$(if $(V),$(info $(config_mk_cmd)),$(info [GEN]   $(config_mk)))
$(shell $(config_mk_cmd))
endif
$(if $(shell $(config_h_cmd)),$(info [GEN]   $(config_h)))
endif

# Normally kept fresh at parse time above; this rule only recreates the
# header if it went missing mid-build (e.g. 'make clean all').
$(config_h):
	$(QUIET) "[GEN]   $@"
	$(VERBOSE) $(config_h_cmd) >/dev/null

%.bin: %.elf
	$(QUIET) "[OBJC]  $@"
	$(VERBOSE) $(OBJCOPY) -O binary $^ $@

%/built-in.a:
	$(QUIET) "[AR]    $@"
	$(VERBOSE) rm -f $@
	$(VERBOSE) $(AR) cDPrST $@ $^

QEMU_CMD=$(QEMU) $(QEMU_ARGS_COMMON) $(QEMU_ARGS)

# -append needs -kernel (qemu folds it into the DTB), so it rides with each -kernel.
QEMU_CMD_DIRECT=$(QEMU_CMD) -kernel grinch.bin -initrd user/initrd.cpio -append "$(QEMU_APPEND)"
QEMU_CMD_UBOOT=$(QEMU_CMD) $(QEMU_UBOOT_ARGS)

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
	$(MAKE) -C $(D_UBOOT) $(MAKEARGS_UBOOT) O=$(UBOOT_PFX) oldconfig
	$(MAKE) -C $(D_UBOOT) $(MAKEARGS_UBOOT) O=$(UBOOT_PFX) u-boot-nodtb.bin

.PHONY: test
test:
	$(srctree)/tests/run.py

.PHONY: defconfig
defconfig:
	$(QUIET) "[GEN]   $(config_mk)"
	$(VERBOSE) $(config_mk_cmd)

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

endif # in-objtree else branch
