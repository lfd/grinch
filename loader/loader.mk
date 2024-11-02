CFLAGS_LOADER = $(CFLAGS_KERNEL_COMMON)
LDFLAGS_LOADER = $(LDFLAGS_KERNEL)

LOADER_OBJS = entry.o main.o

ARCH_LOADER_DIR = loader/$(ARCH_DIR)

LOADER_OBJS := $(addprefix $(ARCH_LOADER_DIR)/, $(LOADER_OBJS))

$(ARCH_LOADER_DIR)/built-in.a: $(LOADER_OBJS) lib/string.o

$(ARCH_LOADER_DIR)/entry.o: vmgrinch.bin

$(ARCH_LOADER_DIR)/loader.o: $(ARCH_LOADER_DIR)/built-in.a
	$(QUIET) "[LD]    $@"
	$(VERBOSE) $(LD) $(LDFLAGS_LOADER) --whole-archive -relocatable -o $@ $<

$(ARCH_LOADER_DIR)/%.o: $(ARCH_LOADER_DIR)/%.c
	$(QUIET) "[CC-L]  $@"
	$(VERBOSE) $(CC) -c $(CFLAGS_LOADER) -o $@ $<

kernel.elf: loader/loader.ld $(ARCH_LOADER_DIR)/loader.o
	$(QUIET) "[LD]    $@"
	$(VERBOSE) $(LD) $(LDFLAGS_LOADER) --gc-sections -T $^ -o $@
ifdef V
	$(SZ) --format=SysV -x $@
endif

kernel.bin: kernel.elf

clean_loader:
	$(call clean_file,kernel.elf)
	$(call clean_objects,$(ARCH_LOADER_DIR),$(LOADER_OBJS) $(ARCH_LOADER_DIR)/loader.o loader/loader.ld)
