LOADER_OBJS = entry.o main.o

ARCH_LOADER_DIR = $(ARCH_DIR)/loader

LOADER_OBJS := $(addprefix $(ARCH_LOADER_DIR)/, $(LOADER_OBJS))

$(ARCH_LOADER_DIR)/built-in.a: $(LOADER_OBJS) lib/string.o

$(ARCH_LOADER_DIR)/entry.o: vmgrinch.bin

$(ARCH_LOADER_DIR)/loader.o: $(ARCH_LOADER_DIR)/built-in.a
	$(LD) $(LDFLAGS) --whole-archive -relocatable -o $@ $<

kernel.elf: loader.ld $(ARCH_LOADER_DIR)/loader.o
	$(LD) $(LDFLAGS) --gc-sections -T $^ -o $@
	$(SZ) --format=SysV -x $@

kernel.bin: kernel.elf

clean_arch_loader:
	rm -rf kernel.elf
	rm -rf $(LOADER_OBJS)
	rm -rf $(ARCH_LOADER_DIR)/built-in.a $(ARCH_LOADER_DIR)/loader.o
