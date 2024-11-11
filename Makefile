ARCH ?= riscv

#DEBUG_OUTPUT=1
#INITCONST_STR=1
#GCOV=1

VERSION=3
PATCHLEVEL=15
EXTRAVERSION=

QEMU_CPUS ?= 2
QEMU_CMDLINE ?= ""

all: kernel.bin user/initrd.cpio tools

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
RMRF=rm -rf

D_UBOOT=$(realpath res/u-boot)
UBOOT_PFX=$(D_UBOOT)/u-boot-$(ARCH)-$(PLATFORM)
UBOOT_BIN=$(UBOOT_PFX)/u-boot-nodtb.bin
MAKEARGS_UBOOT=-j 8 CROSS_COMPILE=$(CROSS_COMPILE)

ifdef V
QUIET := @true
VERBOSE :=
else
QUIET := @echo
VERBOSE := @
endif

OPT?=-O0

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

include scripts/kernel.mk
include user/inc.mk
include tools/inc.mk

%.bin: %.elf
	$(QUIET) "[OBJC]  $@"
	$(VERBOSE) $(OBJCOPY) -O binary $^ $@

%/built-in.a:
	$(QUIET) "[AR]    $@"
	$(VERBOSE) rm -f $@
	$(VERBOSE) $(AR) cDPrST $@ $^

QEMU_CMD=$(QEMU) $(QEMU_ARGS_COMMON) $(QEMU_ARGS) -append $(QEMU_CMDLINE)

QEMU_CMD_DIRECT=$(QEMU_CMD) -kernel kernel.bin -initrd user/initrd.cpio
QEMU_CMD_UBOOT=$(QEMU_CMD) -kernel $(UBOOT_BIN) $(QEMU_UBOOT_ARGS)

qemu: all
	$(QEMU_CMD_DIRECT)

qemuu: all $(UBOOT_BIN)
	$(QEMU_CMD_UBOOT)

qemudb: all
	$(QEMU_CMD_DIRECT) -S

qemuudb: all $(UBOOT_BIN)
	$(QEMU_CMD_UBOOT) -S

qemu.dts: kernel.bin user/initrd.cpio
	$(QEMU_CMD_DIRECT) $(QEMU_MACHINE),dumpdtb=/tmp/qemu_tmp.dtb
	dtc -I dtb -O dts /tmp/qemu_tmp.dtb -o $@
	rm -f /tmp/qemu_tmp.dtb

.PHONY: vmgrinch.dump
vmgrinch.dump: scripts/vmgrinch_dump.gdb vmgrinch.elf kernel.elf
	$(GDB) -x $<

vmgrinch.info: vmgrinch.dump tools/gcov_extract
	./tools/gcov_extract $<
	lcov -c -d . -o $@

gcov: vmgrinch.info
	mkdir -p gcov
	genhtml $< -o gcov/

$(UBOOT_BIN):
	mkdir -p $(UBOOT_PFX)
	cp -av res/u-boot/$(UBOOT_CFG) $(UBOOT_PFX)/.config
	$(MAKE) -C $(D_UBOOT)/u-boot $(MAKEARGS_UBOOT) O=$(UBOOT_PFX) oldconfig
	$(MAKE) -C $(UBOOT_PFX) $(MAKEARGS_UBOOT) u-boot-nodtb.bin

debug: kernel.bin
	$(GDB) -x scripts/debug.gdb $^

clean: clean_core clean_lib clean_mm clean_fs clean_user clean_arch clean_drivers clean_kernel clean_loader clean_tools
	$(call clean_files,all,kernel.bin vmgrinch.bin vmgrinch.elf vmgrinch.dump vmgrinch.info gcov)

mrproper: clean
	$(MAKE) -C $(D_UBOOT) mrproper
