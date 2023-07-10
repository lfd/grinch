ARCH ?= riscv

GRINCH_VER=3.13
#DEBUG_OUTPUT=1

all: kernel.bin

GDB=$(CROSS_COMPILE)gdb
CC=$(CROSS_COMPILE)gcc
LD=$(CROSS_COMPILE)ld
AR=$(CROSS_COMPILE)ar
AS=$(CROSS_COMPILE)as
OBJDUMP=$(CROSS_COMPILE)objdump
OBJCOPY=$(CROSS_COMPILE)objcopy
SZ=$(CROSS_COMPILE)size

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

qemu: kernel.bin
	$(QEMU) $(QEMU_ARGS_COMMON) $(QEMU_ARGS) -kernel $< -s

qemudb: kernel.bin
	$(QEMU) $(QEMU_ARGS_COMMON) $(QEMU_ARGS) -kernel $< -s -S

debug: kernel.bin
	$(GDB) $^

clean: clean_kernel clean_user
	rm -rf *.dtb
	rm -rf *.elf
	rm -rf *.bin
	rm -rf *.ld
