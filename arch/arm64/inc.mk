UBOOT_CFG=$(ARCH_DIR)/u-boot/arm64-$(PLATFORM).config
UBOOT_ENV=$(ARCH_DIR)/u-boot/$(PLATFORM).env

config_defines += CONFIG_ARCH_ARM64=64
# Kernel uses no FP/SIMD (its state is never saved); also bars -O3 NEON codegen.
CFLAGS_ARCH = -mgeneral-regs-only
LDFLAGS_ARCH =

# Route the early debug console through the QEMU semihosting interface
ifeq ($(CONFIG_ARM64_SEMIHOSTING),1)
config_defines += CONFIG_ARM64_SEMIHOSTING=1
QEMU_ARGS_SEMIHOSTING = -semihosting
endif

ARCH_OBJS = arch.o cpu.o entry.o head.o loader.o paging.o platform.o
ARCH_OBJS += psci.o smp.o stackdump.o task.o timer.o traps.o

QEMU = qemu-system-aarch64
ifeq ($(PLATFORM),virt)
QEMU_MACHINE = -machine virt -m 256M
QEMU_ARGS_PLATFORM = -cpu cortex-a57
QEMU_KERNEL_ADDR = 0x41000000
QEMU_INITRD_ADDR = 0x48000000
QEMU_LOAD_UBOOT = -bios $(UBOOT_BIN)
endif

QEMU_ARGS = $(QEMU_ARGS_PLATFORM) $(QEMU_MACHINE) $(QEMU_ARGS_SEMIHOSTING)
QEMU_UBOOT_ARGS = \
	$(QEMU_LOAD_UBOOT) \
	-device loader,file=grinch.bin,addr=$(QEMU_KERNEL_ADDR),force-raw=on \
	-device loader,file=user/initrd.cpio,addr=$(QEMU_INITRD_ADDR),force-raw=on

ARCH_OBJS := $(addprefix $(ARCH_DIR)/, $(ARCH_OBJS))

OBJ_DIRS += $(dir $(ARCH_OBJS))

arch/arm64/built-in.a: $(ARCH_OBJS)

clean_arch:
	$(call clean_objects,arch/arm64,$(ARCH_OBJS))
