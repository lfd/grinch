ARCH ?= riscv

GRINCH_VER=3.13
DEBUG_OUTPUT=1

ARCH_DIR = arch/$(ARCH)
LIB_DIR = lib
MM_DIR = mm
DRIVERS_DIR = drivers

all: kernel.bin

include $(ARCH_DIR)/inc.mk
include $(LIB_DIR)/inc.mk
include $(MM_DIR)/inc.mk
include $(DRIVERS_DIR)/inc.mk
include guest/inc.mk
include loader.mk

GDB=$(CROSS_COMPILE)gdb
CC=$(CROSS_COMPILE)gcc
LD=$(CROSS_COMPILE)ld
AR=$(CROSS_COMPILE)ar
AS=$(CROSS_COMPILE)as
OBJDUMP=$(CROSS_COMPILE)objdump
OBJCOPY=$(CROSS_COMPILE)objcopy
SZ=$(CROSS_COMPILE)size

QEMU_ARGS_COMMON=-monitor telnet:127.0.0.1:11111,server,nowait

CFLAGS=-nostdinc -ffreestanding -O0 -g -ggdb \
       -fno-strict-aliasing -fno-stack-protector \
       -ffunction-sections \
       -Wall -Wextra -Wno-unused-parameter \
       -Wstrict-prototypes -Wtype-limits \
       -Wmissing-declarations -Wmissing-prototypes \
       -Wnested-externs -Wshadow -Wredundant-decls \
       -Wundef -Wdeprecated \
       -Iinclude/ \
       -Ilib/libfdt/ \
       -Iarch/$(ARCH)/include/ \
       -DGRINCH_VER=$(GRINCH_VER)

CFLAGS += $(CFLAGS_ARCH)

ifeq ($(DEBUG_OUTPUT), 1)
CFLAGS += -DDEBUG
endif

LDFLAGS = $(ARCH_LDFLAGS)
AFLAGS = -D__ASSEMBLY__

ASM_DEFINES = arch/$(ARCH)/include/asm/asm-defines.h
GENERATED = $(ASM_DEFINES)

OBJS = main.o

%.o: %.c $(GENERATED)
	$(CC) -c $(CFLAGS) -o $@ $<

%.o: %.S $(GENERATED)
	$(CC) -c $(AFLAGS) $(CFLAGS) -o $@ $<

%.bin: %.elf
	$(OBJCOPY) -O binary $^ $@

%.ld: %.ld.S
	$(CC) $(CFLAGS) $(AFLAGS) -E -o $@ $^
	# Remove commment lines. Required for older linkers.
	sed -e '/^#/d' -i $@

%/built-in.a:
	rm -f $@
	$(AR) cDPrST $@ $^

guest.dtb: guest.dts
	dtc -I dts -O dtb $^ -o $@

grinch.ld: grinch.ld.S

$(ASM_DEFINES): arch/$(ARCH)/asm-defines.S
	./asm-defines.sh $^ > $@

arch/$(ARCH)/asm-defines.S: arch/$(ARCH)/asm-defines.c
	$(CC) $(CFLAGS) -S -o $@ $^

vmgrinch.o: $(ARCH_DIR)/built-in.a $(LIB_DIR)/built-in.a $(MM_DIR)/built-in.a $(DRIVERS_DIR)/built-in.a $(OBJS)
	$(LD) $(LDFLAGS) --whole-archive -relocatable -o $@ $^

vmgrinch.elf: grinch.ld vmgrinch.o
	$(LD) $(LDFLAGS) --gc-sections -T $^ -o $@
	$(SZ) --format=SysV -x $@

objdk: kernel.elf
	$(OBJDUMP) -d $^ | less

objd: vmgrinch.elf
	$(OBJDUMP) -d $^ | less

objdS: vmgrinch.elf
	$(OBJDUMP) -dS $^ | less

objdg: guest/guest.elf
	$(OBJDUMP) -d $^ | less

qemu: kernel.bin
	$(QEMU) $(QEMU_ARGS_COMMON) $(QEMU_ARGS) -kernel $< -s

qemudb: kernel.bin
	$(QEMU) $(QEMU_ARGS_COMMON) $(QEMU_ARGS) -kernel $< -s -S

debug: kernel.bin
	$(GDB) $^

clean: clean_arch clean_arch_loader clean_guest
	rm -rf $(OBJS) vmgrinch.o
	rm -rf $(GENERATED)
	rm -rf arch/$(ARCH)/*.{o,a} arch/$(ARCH)/asm-defines.S
	rm -rf lib/*.{o,a} lib/libfdt/*.{o,a}
	rm -rf drivers/*.{o,a} drivers/irq/*.{o,a} drivers/serial/*.{o,a}
	rm -rf mm/*.{o,a}
	rm -rf *.dtb
	rm -rf *.elf guest/*.elf
	rm -rf *.bin guest/*.bin
	rm -rf *.ld guest/*.ld
