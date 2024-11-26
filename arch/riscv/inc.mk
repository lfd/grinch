CROSS_COMPILE ?= riscv64-linux-gnu-
PLATFORM ?= any

UBOOT_CFG=$(ARCH)-qemu.config
QEMU=qemu-system-$(ARCH)

ifdef ARCH_RISCV64
CFLAGS_DEF_ARCH = -DARCH_RISCV=64 -DARCH=riscv64
CFLAGS_ARCH = -mcmodel=medany -march=rv64imafdc_zifencei $(CFLAGS_DEF_ARCH)
LDFLAGS_ARCH = -melf64lriscv
QEMU_CPU = rv64
else ifdef ARCH_RISCV32
CFLAGS_DEF_ARCH = -DARCH_RISCV=32 -DARCH=riscv32
CFLAGS_ARCH = -march=rv32imafdc_zifencei -mabi=ilp32 $(CFLAGS_DEF_ARCH)
LDFLAGS_ARCH = -melf32lriscv
QEMU_CPU = rv32
endif

QEMU_MACHINE=-machine virt

QEMU_ARGS=-m 64M -smp $(QEMU_CPUS)
QEMU_ARGS+=-serial stdio -monitor telnet:127.0.0.1:55555,server,nowait
QEMU_ARGS+=$(QEMU_MACHINE) -cpu $(QEMU_CPU),h=true
QEMU_ARGS+=-device VGA

QEMU_UBOOT_ARGS=\
		-device loader,file=kernel.bin,addr=0x82000000,force-raw=on \
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
ARCH_OBJS+=vmm/vmm.o
ARCH_OBJS+=vmm/vmm_ecall.o

ARCH_OBJS := $(addprefix $(ARCH_DIR)/, $(ARCH_OBJS))

arch/riscv/built-in.a: $(ARCH_OBJS)

clean_arch:
	$(call clean_objects,arch/riscv,$(ARCH_OBJS))
