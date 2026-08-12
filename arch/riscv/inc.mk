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

ifeq ($(CONFIG_RISCV_COMPRESSED), 1)
march_c := c
endif

CFLAGS_ARCH += -march=rv$(XLEN)ima$(march_c)_zicsr_zifencei
LDFLAGS_ARCH = -melf$(XLEN)lriscv

QEMU_CPU = rv$(XLEN)
QEMU_MACHINE=-machine virt
QEMU_ARGS=-m 64M
QEMU_ARGS+=-monitor telnet:127.0.0.1:55555,server,nowait
QEMU_ARGS+=$(QEMU_MACHINE) -cpu $(QEMU_CPU),h=true
QEMU_UBOOT_ARGS=\
		-kernel $(UBOOT_BIN) \
		-append '$(QEMU_APPEND)' \
		-device loader,file=grinch.bin,addr=0x82000000,force-raw=on \
		-device loader,file=user/initrd.cpio,addr=0x82800000,force-raw=on \

ARCH_OBJS =arch.o
ARCH_OBJS+=cpu.o
ARCH_OBJS+=entry.o
ARCH_OBJS+=head.o
ARCH_OBJS+=isa.o
ARCH_OBJS+=loader.o
ARCH_OBJS+=paging.o
ARCH_OBJS+=platform.o
ARCH_OBJS+=sbi.o
ARCH_OBJS+=smp.o
ARCH_OBJS+=stackdump.o
ARCH_OBJS+=task.o
ARCH_OBJS+=timer.o
ARCH_OBJS+=traps.o
ifeq ($(CONFIG_VMM), 1)
ARCH_OBJS+=vmm/vmm.o
ARCH_OBJS+=vmm/vmm_ecall.o
endif

ARCH_OBJS := $(addprefix $(ARCH_DIR)/, $(ARCH_OBJS))

OBJ_DIRS += $(dir $(ARCH_OBJS))

arch/riscv/built-in.a: $(ARCH_OBJS)

clean_arch:
	$(call clean_objects,arch/riscv,$(ARCH_OBJS))
