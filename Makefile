ARCH ?= riscv

GRINCH_VER=3.13
DEBUG_OUTPUT=1

ARCH_DIR = arch/$(ARCH)
LIB_DIR = lib
MM_DIR = mm
DRIVERS_DIR = drivers

all: grinch.bin

include $(ARCH_DIR)/inc.mk
include $(LIB_DIR)/inc.mk
include $(MM_DIR)/inc.mk
include $(DRIVERS_DIR)/inc.mk

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

OBJS_COMMON = $(OBJS_SERIAL)

ASM_DEFINES = arch/$(ARCH)/include/asm/asm-defines.h
GENERATED = $(ASM_DEFINES)

OBJS = $(OBJS_COMMON) main.o
GUEST_OBJS = $(OBJS_COMMON) guest/main.o guest/printk_header.o guest/handlers.o

%.o: %.c $(GENERATED)
	$(CC) -c $(CFLAGS) -o $@ $<

%.o: %.S $(GENERATED)
	$(CC) -c $(AFLAGS) $(CFLAGS) -o $@ $<

%.bin: %.elf
	$(OBJCOPY) -O binary $^ $@

%/built-in.a:
	rm -f $@
	$(AR) cDPrST $@ $^


guest.dtb: guest.dts
	dtc -I dts -O dtb $^ -o $@

linkerfile.ld: linkerfile.ld.S
	$(CC) $(CFLAGS) $(AFLAGS) -E -o $@ $^
	# Remove commment lines. Required for older linkers.
	sed -e '/^#/d' -i $@

guest-data.o: guest/guest.bin guest.dtb

guest/linkerfile.ld: linkerfile.ld.S
	$(CC) $(CFLAGS) $(AFLAGS) -E -o $@ $^ -DIS_GUEST
	# Remove commment lines. Required for older linkers.
	sed -e '/^#/d' -i $@

guest/guest.elf: guest/linkerfile.ld guest/guest.o
	$(LD) $(LDFLAGS) -T $^ -o $@
	$(SZ) --format=SysV -x $@

$(ASM_DEFINES): arch/$(ARCH)/asm-defines.S
	./asm-defines.sh $^ > $@

arch/$(ARCH)/asm-defines.S: arch/$(ARCH)/asm-defines.c
	$(CC) $(CFLAGS) -S -o $@ $^

guest/guest.o: $(GUEST_OBJS)
	$(LD) $(LDFLAGS) -relocatable -o $@ $^

grinch.o: $(ARCH_DIR)/built-in.a $(LIB_DIR)/built-in.a $(MM_DIR)/built-in.a $(DRIVERS_DIR)/built-in.a $(OBJS)
	$(LD) $(LDFLAGS) --whole-archive -relocatable -o $@ $^

grinch.elf: linkerfile.ld grinch.o
	$(LD) $(LDFLAGS) --gc-sections -T $^ -o $@
	$(SZ) --format=SysV -x $@

objd: grinch.elf
	$(OBJDUMP) -d $^ | less

objdS: grinch.elf
	$(OBJDUMP) -dS $^ | less

objdg: guest/guest.elf
	$(OBJDUMP) -d $^ | less

ifeq ($(ARCH),riscv)
qemu: grinch.bin
	$(QEMU) $(QEMU_ARGS_COMMON) $(QEMU_ARGS) -kernel $< -s

qemudb: grinch.bin
	$(QEMU) $(QEMU_ARGS_COMMON) $(QEMU_ARGS) -kernel $< -s -S
endif

debug: grinch.elf
	$(GDB) $^

deploy: grinch.elf
	scp -P 33333 grinch.elf root@localhost:

clean: clean_arch
	rm -rf $(OBJS) grinch.o
	rm -rf $(GUEST_OBJS) guest/guest.o
	rm -rf $(GENERATED)
	rm -rf arch/$(ARCH)/*.{o,a} arch/$(ARCH)/asm-defines.S
	rm -rf lib/*.{o,a} lib/libfdt/*.{o,a}
	rm -rf drivers/*.{o,a} drivers/irq/*.{o,a} drivers/serial/*.{o,a}
	rm -rf mm/*.{o,a}
	rm -rf *.dtb
	rm -rf *.elf guest/*.elf
	rm -rf *.bin guest/*.bin
	rm -rf *.ld guest/*.ld
