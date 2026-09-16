UBOOT_CFG=$(ARCH_DIR)/u-boot/$(ARCH)-qemu.config
UBOOT_ENV=$(ARCH_DIR)/u-boot/qemu.env
QEMU=qemu-system-$(ARCH)

ifdef ARCH_RISCV64
XLEN = 64
CFLAGS_ARCH = -mcmodel=medany -mabi=lp64
else ifdef ARCH_RISCV32
XLEN = 32
CFLAGS_ARCH = -mabi=ilp32
endif

ifeq ($(CONFIG_RISCV_COMPRESSED), y)
march_c := c
endif

CFLAGS_ARCH += -march=rv$(XLEN)ima$(march_c)_zicsr_zifencei
LDFLAGS_ARCH = -melf$(XLEN)lriscv

QEMU_CPU = rv$(XLEN)
QEMU_MACHINE=-machine virt
QEMU_ARGS=-m 64M
QEMU_ARGS+=-monitor telnet:127.0.0.1:55555,server,nowait
QEMU_ARGS+=$(QEMU_MACHINE) -cpu $(QEMU_CPU),h=true
# Machine mode is the kernel's own: nothing may be loaded below it.
ifeq ($(RISCV_MODE), m)
QEMU_ARGS+=-bios none
endif
QEMU_UBOOT_ARGS=\
		-kernel $(UBOOT_BIN) \
		-append '$(QEMU_APPEND)' \
		-device loader,file=grinch.bin,addr=0x82000000,force-raw=on \
		-device loader,file=user/initrd.cpio,addr=0x82800000,force-raw=on \

# The privilege level reaches config.mk as a string; the object list below
# keys on it like any other config symbol.
ifeq ($(RISCV_MODE), m)
CONFIG_RISCV_M_MODE = y
endif

ARCH_OBJS-y =arch.o
ARCH_OBJS-y+=cpu.o
ARCH_OBJS-y+=entry.o
ARCH_OBJS-y+=head.o
ARCH_OBJS-y+=isa.o
ARCH_OBJS-$(CONFIG_MMU)+=loader.o
ARCH_OBJS-$(CONFIG_MMU)+=paging.o
ARCH_OBJS-y+=platform.o
ARCH_OBJS-y+=smp.o
ARCH_OBJS-y+=stackdump.o
ARCH_OBJS-y+=task.o
ARCH_OBJS-y+=timer.o
ARCH_OBJS-y+=traps.o
ARCH_OBJS-$(CONFIG_OPENSBI)+=sbi.o
ARCH_OBJS-$(CONFIG_RISCV_M_MODE)+=native.o
ARCH_OBJS-$(CONFIG_RISCV_M_MODE)+=pmp.o
ARCH_OBJS-$(CONFIG_VMM)+=vmm/vmm.o
ARCH_OBJS-$(CONFIG_VMM)+=vmm/vmm_ecall.o
ARCH_OBJS := $(ARCH_OBJS-y)

ARCH_OBJS := $(addprefix $(ARCH_DIR)/, $(ARCH_OBJS))

OBJ_DIRS += $(dir $(ARCH_OBJS))

arch/riscv/built-in.a: $(ARCH_OBJS)

clean_arch:
	$(call clean_objects,arch/riscv,$(ARCH_OBJS))
