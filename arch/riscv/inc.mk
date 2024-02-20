CROSS_COMPILE ?= riscv64-linux-gnu-

UBOOT_CFG=riscv-qemu.config

QEMU=qemu-system-riscv64
QEMU_MACHINE=-machine virt
QEMU_ARGS=-m 64M -smp 2 -serial stdio -nographic -monitor telnet:127.0.0.1:55555,server,nowait $(QEMU_MACHINE) -cpu rv64,h=true
QEMU_UBOOT_ARGS=\
		-device loader,file=kernel.bin,addr=0x82000000,force-raw=on \
		-device loader,file=user/initrd.cpio,addr=0x82800000,force-raw=on \

CFLAGS_ARCH = -mcmodel=medany -march=rv64imafdc_zifencei -DARCH_RISCV=1
LDFLAGS_ARCH = -melf64lriscv

ARCH_OBJS = entry.o head.o sbi.o traps.o cpu.o task.o
ARCH_OBJS += smp.o paging.o arch.o irqchip.o isa.o stackdump.o timer.o

ARCH_OBJS += vmm/vmm.o vmm/vmm_ecall.o

ARCH_OBJS := $(addprefix $(ARCH_DIR)/, $(ARCH_OBJS))

arch/riscv/built-in.a: $(ARCH_OBJS)

qemu.dts:
	$(QEMU_CMD_DIRECT) $(QEMU_MACHINE),dumpdtb=/tmp/qemu_tmp.dtb
	dtc -I dtb -O dts /tmp/qemu_tmp.dtb -o $@
	rm -f /tmp/qemu_tmp.dtb

clean_arch:
	$(RMRF) $(ARCH_DIR)/*.{o,a}
	$(RMRF) $(ARCH_DIR)/vmm/*.{o,a}
