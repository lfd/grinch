CROSS_COMPILE ?= riscv64-linux-gnu-
PLATFORM ?= any

UBOOT_CFG=riscv-qemu.config

QEMU=qemu-system-riscv64

QEMU_MACHINE=-machine virt

QEMU_ARGS=-m 64M -smp $(QEMU_CPUS)
QEMU_ARGS+=-serial stdio -monitor telnet:127.0.0.1:55555,server,nowait
QEMU_ARGS+=$(QEMU_MACHINE) -cpu rv64,h=true
QEMU_ARGS+=-device VGA

QEMU_UBOOT_ARGS=\
		-device loader,file=kernel.bin,addr=0x82000000,force-raw=on \
		-device loader,file=user/initrd.cpio,addr=0x82800000,force-raw=on \

CFLAGS_ARCH = -mcmodel=medany -march=rv64imafdc_zifencei -DARCH_RISCV=1
LDFLAGS_ARCH = -melf64lriscv

ARCH_OBJS = arch.o cpu.o entry.o head.o isa.o paging.o platform.o
ARCH_OBJS += sbi.o smp.o stackdump.o task.o timer.o traps.o

ARCH_OBJS += vmm/vmm.o vmm/vmm_ecall.o

ARCH_OBJS := $(addprefix $(ARCH_DIR)/, $(ARCH_OBJS))

arch/riscv/built-in.a: $(ARCH_OBJS)

clean_arch:
	$(call clean_objects,arch/riscv,$(ARCH_OBJS))
