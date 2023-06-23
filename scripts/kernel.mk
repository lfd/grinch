ARCH_DIR = arch/$(ARCH)
include $(ARCH_DIR)/inc.mk
include kernel/inc.mk
include lib/inc.mk
include mm/inc.mk
include drivers/inc.mk
include loader.mk

QEMU_ARGS_COMMON=-monitor telnet:127.0.0.1:11111,server,nowait

INCLUDES_KERNEL=-Iinclude/ \
                -Ilib/libfdt/ \
                -Iarch/$(ARCH)/include/ \
                -DGRINCH_VER=$(GRINCH_VER)

CFLAGS_KERNEL=$(CFLAGS_COMMON) $(CFLAGS_ARCH) $(INCLUDES_KERNEL)

LDFLAGS_KERNEL = $(ARCH_LDFLAGS)
AFLAGS_KERNEL = $(AFLAGS_COMMON)

ASM_DEFINES = arch/$(ARCH)/include/asm/asm_defines.h
GENERATED = $(ASM_DEFINES)

%.o: %.c $(GENERATED)
	$(QUIET) "[CC]    $@"
	$(VERBOSE) $(CC) -c $(CFLAGS_KERNEL) -o $@ $<

%.o: %.S $(GENERATED)
	$(QUIET) "[CC/AS] $@"
	$(VERBOSE) $(CC) -c $(AFLAGS_KERNEL) $(CFLAGS_KERNEL) -o $@ $<

%.ld: %.ld.S
	$(QUIET) "[CC/AS] $@"
	$(VERBOSE) $(CC) $(CFLAGS_KERNEL) $(AFLAGS_KERNEL) -E -o $@ $^
	$(VERBOSE) sed -e '/^#/d' -i $@

grinch.ld: grinch.ld.S

$(ASM_DEFINES): arch/$(ARCH)/asm_defines.S
	$(QUIET) "[GEN]   $@"
	$(VERBOSE) ./scripts/asm-defines.sh $^ > $@

arch/$(ARCH)/asm_defines.S: arch/$(ARCH)/asm_defines.c
	$(QUIET) "[GEN]   $@"
	$(VERBOSE) $(CC) $(CFLAGS_KERNEL) -S -o $@ $^

vmgrinch.o: $(ARCH_DIR)/built-in.a kernel/built-in.a lib/built-in.a mm/built-in.a drivers/built-in.a
	$(QUIET) "[LD]    $@"
	$(VERBOSE) $(LD) $(LDFLAGS) --whole-archive -relocatable -o $@ $^

vmgrinch.elf: grinch.ld vmgrinch.o
	$(QUIET) "[LD]    $@"
	$(VERBOSE) $(LD) $(LDFLAGS) --gc-sections -T $^ -o $@
ifdef V
	$(VERBOSE) $(SZ) --format=SysV -x $@
endif

objdk: kernel.elf
	$(OBJDUMP) -d $^ | less

objd: vmgrinch.elf
	$(OBJDUMP) -d $^ | less

objdS: vmgrinch.elf
	$(OBJDUMP) -dS $^ | less

clean_kernel: clean_loader
	rm -rf vmgrinch.o
	rm -rf $(GENERATED)
	rm -rf arch/$(ARCH)/*.{o,a} arch/$(ARCH)/asm_defines.S
	rm -rf kernel/*.{o,a}
	rm -rf lib/*.{o,a} lib/libfdt/*.{o,a}
	rm -rf drivers/*.{o,a} drivers/irq/*.{o,a} drivers/serial/*.{o,a}
	rm -rf mm/*.{o,a}
