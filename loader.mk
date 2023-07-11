LOADER_OBJS = entry.o main.o

ARCH_LOADER_DIR = $(ARCH_DIR)/loader

LOADER_OBJS := $(addprefix $(ARCH_LOADER_DIR)/, $(LOADER_OBJS))

$(ARCH_LOADER_DIR)/built-in.a: $(LOADER_OBJS) lib/string.o

$(ARCH_LOADER_DIR)/entry.o: vmgrinch.bin

$(ARCH_LOADER_DIR)/loader.o: $(ARCH_LOADER_DIR)/built-in.a
	$(QUIET) "[LD]    $@"
	$(VERBOSE) $(LD) $(LDFLAGS) --whole-archive -relocatable -o $@ $<

kernel.elf: loader.ld $(ARCH_LOADER_DIR)/loader.o
	$(QUIET) "[LD]    $@"
	$(VERBOSE) $(LD) $(LDFLAGS) --gc-sections -T $^ -o $@
ifdef V
	$(SZ) --format=SysV -x $@
endif

kernel.bin: kernel.elf

clean_loader:
	$(RMRF) kernel.elf
	$(RMRF) $(LOADER_OBJS)
	$(RMRF) $(ARCH_LOADER_DIR)/built-in.a $(ARCH_LOADER_DIR)/loader.o
