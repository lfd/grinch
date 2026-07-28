SYSCALL_HEADER = common/include/generated/syscall.h

ifeq ($(ARCH),riscv64)
	ARCH_SUPER = riscv
	UBOOT_ARCH = riscv
	ARCH_RISCV = true
	ARCH_RISCV64 = true
else ifeq ($(ARCH),riscv32)
	ARCH_SUPER = riscv
	UBOOT_ARCH = riscv
	ARCH_RISCV = true
	ARCH_RISCV32 = true
else ifeq ($(ARCH),arm64)
	ARCH_SUPER = arm64
	# U-Boot builds aarch64 under its "arm" architecture
	UBOOT_ARCH = arm
	ARCH_ARM64 = true
else
$(error Unsupported Architecture $(ARCH))
endif

ARCH_DIR = arch/$(ARCH_SUPER)

include $(srctree)/$(ARCH_DIR)/inc.mk
include $(srctree)/fs/inc.mk
include $(srctree)/kernel/inc.mk
include $(srctree)/lib/inc.mk
include $(srctree)/mm/inc.mk
include $(srctree)/drivers/inc.mk

QEMU_ARGS_COMMON=-monitor telnet:127.0.0.1:11111,server,nowait -s
QEMU_ARGS_COMMON+=-device VGA -display $(QEMU_DISPLAY)
QEMU_ARGS_COMMON+=-smp $(QEMU_CPUS)
QEMU_ARGS_COMMON+=-serial $(QEMU_SERIAL)

INCLUDES_KERNEL_SRC = -I$(srctree)/include/ \
                      -I$(srctree)/common/include \
                      -I$(srctree)/lib/libfdt/ \
                      -I$(srctree)/$(ARCH_DIR)/include/ \
                      -I$(srctree)/common/include/arch/$(ARCH_SUPER)/

INCLUDES_KERNEL_GEN = -I$(objtree)/include/ \
                      -I$(objtree)/common/include \
                      -I$(objtree)/$(ARCH_DIR)/include/ \
                      -I$(objtree)/kernel/

INCLUDES_KERNEL = $(INCLUDES_KERNEL_SRC) $(INCLUDES_KERNEL_GEN)

CFLAGS_KERNEL_COMMON=$(CFLAGS_COMMON) $(CFLAGS_ARCH) $(CFLAGS_STANDALONE) $(INCLUDES_KERNEL)

CFLAGS_KERNEL = $(CFLAGS_KERNEL_COMMON)
LDFLAGS_KERNEL = $(LDFLAGS_COMMON) $(LDFLAGS_ARCH)
AFLAGS_KERNEL = $(AFLAGS_COMMON)

ifeq ($(CONFIG_GCOV), 1)
    CFLAGS_KERNEL += -fprofile-arcs -ftest-coverage
    config_defines += CONFIG_GCOV=1
    ifdef ARCH_RISCV64
        CFLAGS_KERNEL += -fprofile-update=atomic
    endif
endif

ASM_DEFINES = $(ARCH_DIR)/include/asm/asm_defines.h
GENERATED = $(ASM_DEFINES) $(config_h) $(version_h) $(compile_h)

%.o: %.c $(GENERATED)
	$(QUIET) "[CC]    $@"
	$(VERBOSE) $(CC) -c $(CFLAGS_KERNEL) -o $@ $<

%.o: %.S $(GENERATED)
	$(QUIET) "[CC/AS] $@"
	$(VERBOSE) $(CC) -c $(AFLAGS_KERNEL) $(CFLAGS_KERNEL) -o $@ $<

%.ld: %.ld.S
	$(QUIET) "[CC/AS] $@"
	$(VERBOSE) $(CC) $(CFLAGS_KERNEL) $(AFLAGS_KERNEL) -DLINKER_SCRIPT -E -o $@ $^
	$(VERBOSE) sed -e '/^#/d' -i $@

$(ASM_DEFINES): $(ARCH_DIR)/asm_defines.S
	$(QUIET) "[GEN]   $@"
	$(VERBOSE) $(MKDIR_P) $(dir $@)
	$(VERBOSE) $(srctree)/scripts/asm-defines.sh $^ > $@

$(compile_h): $(srctree)/scripts/mkcompile_h $(srctree)/Makefile
	$(QUIET) "[GEN]   $@"
	$(VERBOSE) $(MKDIR_P) $(dir $@)
	$(VERBOSE) $< $@ $(CC) "$(CFLAGS_KERNEL)"

$(version_h): $(srctree)/scripts/mkversion_h $(srctree)/Makefile
	$(QUIET) "[GEN]   $@"
	$(VERBOSE) $(MKDIR_P) $(dir $@)
	$(VERBOSE) $< $@ $(VERSION) $(PATCHLEVEL) $(EXTRAVERSION)

$(ARCH_DIR)/asm_defines.S: $(srctree)/$(ARCH_DIR)/asm_defines.c
	$(QUIET) "[GEN]   $@"
	$(VERBOSE) $(MKDIR_P) $(dir $@)
	$(VERBOSE) $(CC) $(CFLAGS_KERNEL_COMMON) -S -o $@ $^

grinch.o: $(ARCH_DIR)/built-in.a drivers/built-in.a fs/built-in.a kernel/built-in.a lib/built-in.a mm/built-in.a
	$(QUIET) "[LD]    $@"
	$(VERBOSE) $(LD) $(LDFLAGS_KERNEL) --whole-archive -relocatable -o $@ $^

grinch.elf: kernel/grinch.ld grinch.o
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

objd: grinch.elf
	$(OBJDUMP) -d $^ | less

objdS: grinch.elf
	$(OBJDUMP) -dS $^ | less

clean_core:
	$(call clean_files,generated,grinch.o $(ARCH_DIR)/asm_defines.S kernel/syscall_table.c $(GENERATED) $(SYSCALL_HEADER))
