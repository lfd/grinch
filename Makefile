ARCH ?= riscv

GRINCH_VER=3.14
#DEBUG_OUTPUT=1

all: kernel.bin user/initrd.cpio

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

D_UBOOT=res/u-boot/u-boot
UBOOT_BIN=$(D_UBOOT)/u-boot-nodtb.bin
MAKEARGS_UBOOT=-j 8 -C $(D_UBOOT) CROSS_COMPILE=$(CROSS_COMPILE)

ifdef V
QUIET := @true
VERBOSE :=
else
QUIET := @echo
VERBOSE := @
endif

OPT?=-O0

AFLAGS_COMMON=-D__ASSEMBLY__

CFLAGS_COMMON=-nostdinc -ffreestanding -g -ggdb $(OPT) \
              -fno-strict-aliasing -fno-stack-protector \
              -ffunction-sections \
              -Wall -Wextra -Wno-unused-parameter \
              -Wstrict-prototypes -Wtype-limits \
              -Wmissing-declarations -Wmissing-prototypes \
              -Wnested-externs -Wshadow -Wredundant-decls \
              -Wundef -Wdeprecated -Werror

ifeq ($(DEBUG_OUTPUT), 1)
CFLAGS_COMMON += -DDEBUG
endif

include scripts/kernel.mk
include user/inc.mk

%.bin: %.elf
	$(QUIET) "[OBJC]  $@"
	$(VERBOSE) $(OBJCOPY) -O binary $^ $@

%/built-in.a:
	$(QUIET) "[AR]    $@"
	$(VERBOSE) rm -f $@
	$(VERBOSE) $(AR) cDPrST $@ $^

QEMU_CMD=$(QEMU) $(QEMU_ARGS_COMMON) $(QEMU_ARGS)

QEMU_CMD_DIRECT=$(QEMU_CMD) -kernel kernel.bin -initrd user/initrd.cpio
QEMU_CMD_UBOOT=$(QEMU_CMD) -kernel $(UBOOT_BIN) $(QEMU_UBOOT_ARGS)

qemu: all
	$(QEMU_CMD_DIRECT)

qemudb: all
	$(QEMU_CMD_DIRECT) -S

qemuu: all $(UBOOT_BIN)
	$(QEMU_CMD_UBOOT)

$(UBOOT_BIN):
	cp -av res/u-boot/$(UBOOT_CFG) res/u-boot/u-boot/.config
	$(MAKE) $(MAKEARGS_UBOOT) u-boot-nodtb.bin

debug: kernel.bin
	$(GDB) $^

clean: clean_kernel clean_user
	$(RMRF) *.dtb
	$(RMRF) *.elf
	$(RMRF) *.bin

mrproper: clean
	$(MAKE) -C $(D_UBOOT) mrproper
