CROSS_COMPILE=riscv64-linux-gnu-

GDB=$(CROSS_COMPILE)gdb
CC=$(CROSS_COMPILE)gcc
LD=$(CROSS_COMPILE)ld
AS=$(CROSS_COMPILE)as
OBJDUMP=$(CROSS_COMPILE)objdump
OBJCOPY=$(CROSS_COMPILE)objcopy
SZ=$(CROSS_COMPILE)size

# qemu-system-riscv64 -m 128M -smp 1 -serial stdio -nographic -monitor telnet:127.0.0.1:55555,server,nowait -enable-kvm -machine virt -kernel grinch.elf

QEMU=qemu-system-riscv64
QEMU_ARGS=-m 128M -smp 2 -serial stdio -nographic -monitor telnet:127.0.0.1:55555,server,nowait -machine virt -cpu rv64,h=true -kernel grinch.bin

GRINCH_VER=3.13

CFLAGS=-nostdinc -ffreestanding -O0 -g -ggdb \
       -fno-strict-aliasing -fno-stack-protector \
       -ffunction-sections \
       -Wall -Wextra -Wno-unused-parameter \
       -Wstrict-prototypes -Wtype-limits \
       -Wmissing-declarations -Wmissing-prototypes \
       -Wnested-externs -Wshadow -Wredundant-decls \
       -Wundef -Wdeprecated \
       -mcmodel=medany -march=rv64imafdc \
       -Iinclude/ \
       -Ilibfdt/ \
       -DGRINCH_VER=$(GRINCH_VER)

LDFLAGS=-melf64lriscv --gc-sections
AFLAGS=-D__ASSEMBLY__

all: grinch.bin

OBJS_FDT = libfdt/fdt.o libfdt/fdt_ro.o libfdt/fdt_addresses.o
OBJS_SERIAL = serial/serial.o serial/uart-8250.o serial/uart-sbi.o
OBJS_SERIAL += serial/uart-apbuart.o
OBJS_MM = mm/mm.o mm/paging.o mm/ioremap.o mm/mmu.o
OBJS_COMMON = $(OBJS_MM) $(OBJS_SERIAL) $(OBJS_FDT)
OBJS_COMMON += entry.o printk.o lib/string.o lib/fdt.o traps.o
OBJS_COMMON += exception.o cpu.o pie.o lib/bitmap.o

ASM_DEFINES = include/grinch/asm-defines.h
GENERATED = $(ASM_DEFINES)

OBJS=$(OBJS_COMMON) main.o printk_header.o sbi.o handlers.o plic.o
OBJS+= smp.o
GUEST_OBJS=$(OBJS_COMMON) guest/main.o guest/printk_header.o guest/handlers.o

%.o: %.c $(GENERATED)
	$(CC) -c $(CFLAGS) -o $@ $<

%.o: %.S $(GENERATED)
	$(CC) -c $(AFLAGS) $(CFLAGS) -o $@ $<

%.bin: %.elf
	$(OBJCOPY) -O binary $^ $@

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

$(ASM_DEFINES): asm-defines.S
	./asm-defines.sh $^ > $@

asm-defines.S: asm-defines.c
	$(CC) $(CFLAGS) -S -o $@ $^

guest/guest.o: $(GUEST_OBJS)
	$(LD) $(LDFLAGS) -relocatable -o $@ $^

grinch.o: $(OBJS)
	$(LD) $(LDFLAGS) -relocatable -o $@ $^

grinch.elf: linkerfile.ld grinch.o
	$(LD) $(LDFLAGS) -T $^ -o $@
	$(SZ) --format=SysV -x $@

objd: grinch.elf
	$(OBJDUMP) -d $^ | less

objdS: grinch.elf
	$(OBJDUMP) -dS $^ | less

objdg: guest/guest.elf
	$(OBJDUMP) -d $^ | less

qemu: grinch.bin
	$(QEMU) $(QEMU_ARGS) -s

qemudb: grinch.bin
	$(QEMU) $(QEMU_ARGS) -s -S

debug: grinch.elf
	$(GDB) $^

deploy: grinch.elf
	scp -P 33333 grinch.elf root@localhost:

clean:
	rm -rf $(OBJS) grinch.o
	rm -rf $(GUEST_OBJS) guest/guest.o
	rm -rf $(GENERATED)
	rm -rf asm-defines.S
	rm -rf libfdt/*.o
	rm -rf *.dtb
	rm -rf *.elf guest/*.elf
	rm -rf *.bin guest/*.bin
	rm -rf *.ld guest/*.ld
