CROSS_COMPILE ?= riscv64-linux-gnu-

QEMU=qemu-system-riscv64
QEMU_ARGS=-m 128M -smp 2 -serial stdio -nographic -monitor telnet:127.0.0.1:55555,server,nowait -machine virt -cpu rv64,h=true

CFLAGS_ARCH = -mcmodel=medany -march=rv64imafdc -DARCH_RISCV=1
LDFLAGS_ARCH = -melf64lriscv

ARCH_OBJS = entry.o exception.o guest-data.o sbi.o traps.o cpu.o
ARCH_OBJS += handlers.o smp.o paging.o arch.o irqchip.o

ARCH_OBJS := $(addprefix $(ARCH_DIR)/, $(ARCH_OBJS))

#$(ARCH_OBJS)/guest-data.o: guest.dtb guest/guest.bin

arch/riscv/built-in.a: $(ARCH_OBJS)
