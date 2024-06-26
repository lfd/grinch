SYSCALL_HEADER = common/include/generated/syscall.h

ARCH_DIR = arch/$(ARCH)
include $(ARCH_DIR)/inc.mk
include fs/inc.mk
include kernel/inc.mk
include lib/inc.mk
include mm/inc.mk
include drivers/inc.mk
include loader/loader.mk

QEMU_ARGS_COMMON=-monitor telnet:127.0.0.1:11111,server,nowait -s

INCLUDES_KERNEL=-Iinclude/ \
		-Icommon/include \
                -Ilib/libfdt/ \
                -Iarch/$(ARCH)/include/

CFLAGS_KERNEL=$(CFLAGS_COMMON) $(CFLAGS_ARCH) $(CFLAGS_STANDALONE) $(INCLUDES_KERNEL)

LDFLAGS_KERNEL = $(LDFLAGS_COMMON) $(LDFLAGS_ARCH)
AFLAGS_KERNEL = $(AFLAGS_COMMON)

ASM_DEFINES = arch/$(ARCH)/include/asm/asm_defines.h
GENERATED = $(ASM_DEFINES) include/generated/version.h include/generated/compile.h

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

$(ASM_DEFINES): arch/$(ARCH)/asm_defines.S
	$(QUIET) "[GEN]   $@"
	$(VERBOSE) ./scripts/asm-defines.sh $^ > $@

include/generated/compile.h: scripts/mkcompile_h Makefile
	$(QUIET) "[GEN]   $@"
	$(VERBOSE) $< $@ $(CC) "$(CFLAGS_KERNEL)"

include/generated/version.h: scripts/mkversion_h Makefile
	$(QUIET) "[GEN]   $@"
	$(VERBOSE) $< $@ $(VERSION) $(PATCHLEVEL) $(EXTRAVERSION)

arch/$(ARCH)/asm_defines.S: arch/$(ARCH)/asm_defines.c
	$(QUIET) "[GEN]   $@"
	$(VERBOSE) $(CC) $(CFLAGS_KERNEL) -S -o $@ $^

vmgrinch.o: $(ARCH_DIR)/built-in.a drivers/built-in.a fs/built-in.a kernel/built-in.a lib/built-in.a mm/built-in.a
	$(QUIET) "[LD]    $@"
	$(VERBOSE) $(LD) $(LDFLAGS_KERNEL) --whole-archive -relocatable -o $@ $^

vmgrinch.elf: kernel/grinch.ld vmgrinch.o
	$(QUIET) "[LD]    $@"
	$(VERBOSE) $(LD) $(LDFLAGS_KERNEL) --gc-sections -T $^ -o $@
ifdef V
	$(VERBOSE) $(SZ) --format=SysV -x $@
endif

.SECONDARY:
%.dts: %.dts.S
	$(QUIET) "[DTS]   $@"
	$(VERBOSE) $(CC) $(CFLAGS_KERNEL) $(AFLAGS_KERNEL) -E -undef -o $@ $^
	$(VERBOSE) sed -e '/^#/d' -i $@

%.dtb: %.dts
	$(QUIET) "[DTC]   $@"
	$(VERBOSE) $(DTC) -I dts -O dtb -o $@ $^

objdk: kernel.elf
	$(OBJDUMP) -d $^ | less

objd: vmgrinch.elf
	$(OBJDUMP) -d $^ | less

objdS: vmgrinch.elf
	$(OBJDUMP) -dS $^ | less

clean_kernel:
	$(RMRF) vmgrinch.o
	$(RMRF) $(GENERATED)
	$(RMRF) arch/$(ARCH)/asm_defines.S
	$(RMRF) fs/*.{o,a} kernel/*.{o,a,ld}
	$(RMRF) lib/*.{o,a} lib/libfdt/*.{o,a}
	$(RMRF) mm/*.{o,a}
	$(RMRF) kernel/syscall_table.c
	$(RMRF) include/generated
	$(RMRF) common/include/generated
