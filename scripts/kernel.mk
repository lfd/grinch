SYSCALL_HEADER = common/include/generated/syscall.h

ifeq ($(ARCH),riscv)
	ARCH_SUPER = riscv
	ARCH_RISCV = true
	ARCH_RISCV64 = true
else
$(error Unsupported Architecture $(ARCH))
endif

ARCH_DIR = arch/$(ARCH_SUPER)

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
                -I$(ARCH_DIR)/include/ \
                -Icommon/include/arch/$(ARCH)/


CFLAGS_KERNEL_COMMON=$(CFLAGS_COMMON) $(CFLAGS_ARCH) $(CFLAGS_STANDALONE) $(INCLUDES_KERNEL)

CFLAGS_KERNEL = $(CFLAGS_KERNEL_COMMON)
LDFLAGS_KERNEL = $(LDFLAGS_COMMON) $(LDFLAGS_ARCH)
AFLAGS_KERNEL = $(AFLAGS_COMMON)

ifdef GCOV
    CFLAGS_KERNEL += -fprofile-arcs -ftest-coverage -fprofile-update=atomic -DGCOV=1
endif

ASM_DEFINES = $(ARCH_DIR)/include/asm/asm_defines.h
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

$(ASM_DEFINES): $(ARCH_DIR)/asm_defines.S
	$(QUIET) "[GEN]   $@"
	$(VERBOSE) ./scripts/asm-defines.sh $^ > $@

include/generated/compile.h: scripts/mkcompile_h Makefile
	$(QUIET) "[GEN]   $@"
	$(VERBOSE) $< $@ $(CC) "$(CFLAGS_KERNEL)"

include/generated/version.h: scripts/mkversion_h Makefile
	$(QUIET) "[GEN]   $@"
	$(VERBOSE) $< $@ $(VERSION) $(PATCHLEVEL) $(EXTRAVERSION)

$(ARCH_DIR)/asm_defines.S: $(ARCH_DIR)/asm_defines.c
	$(QUIET) "[GEN]   $@"
	$(VERBOSE) $(CC) $(CFLAGS_KERNEL_COMMON) -S -o $@ $^

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

clean_core:
	$(call clean_file,vmgrinch.o)
	$(call clean_files,generated,$(GENERATED))
	$(call clean_file,$(ARCH_DIR)/asm_defines.S)
	$(call clean_file,kernel/syscall_table.c)
	$(call clean_file,include/generated)
	$(call clean_file,common/include/generated)
