INCLUDES_USER = -Iuser/include -Iinclude_common

CFLAGS_USER = $(CFLAGS_COMMON) $(CFLAGS_ARCH) $(INCLUDES_USER)

user/%.o: user/%.c
	$(QUIET) "[CC-U]  $@"
	$(VERBOSE) $(CC) -c $(CFLAGS_USER) -o $@ $<

user/%.o: user/%.S $(GENERATED)
	$(QUIET) "[AS-U]  $@"
	$(VERBOSE) $(CC) -c $(AFLAGS_USER) $(CFLAGS_USER) -o $@ $<

user/user.o: user/built-in.a
	$(QUIET) "[LD-U]  $@"
	$(VERBOSE) $(LD) $(LDFLAGS_USER) --whole-archive -relocatable -o $@ $<

user/user.elf: user.ld user/user.o
	$(QUIET) "[LD-U]  $@"
	$(VERBOSE) $(LD) $(LDFLAGS_USER) --gc-sections -T $^ -o $@
ifdef V
	$(SZ) --format=SysV -x $@
endif

LIBC = user/lib/stdio.o user/lib/string.o user/lib/unistd.o user/lib/sched.o
LIBC += user/lib/$(ARCH)/entry.o user/lib/$(ARCH)/syscall.o

user/lib/built-in.a: $(LIBC)

user/built-in.a: user/lib/built-in.a user/main.o

user/user.bin: user/user.elf

clean_user:
	rm -rf user/*.{elf,o,a,bin}
	rm -rf user/lib/*.{o,a}
	rm -rf user/lib/$(ARCH)/*.{o,a}
