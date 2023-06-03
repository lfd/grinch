GUEST_OBJS = $(OBJS_COMMON) guest/main.o guest/handlers.o

guest/built-in.a: lib/built-in.a drivers/built-in.a mm/built-in.a arch/riscv/built-in.a $(GUEST_OBJS)

guest/guest.o: guest/built-in.a
	$(LD) $(LDFLAGS) --whole-archive -relocatable -o $@ $^

guest-data.o: guest/guest.bin guest.dtb

guest/linkerfile.ld: linkerfile.ld.S
	$(CC) $(CFLAGS) $(AFLAGS) -E -o $@ $^ -DIS_GUEST
	# Remove commment lines. Required for older linkers.
	sed -e '/^#/d' -i $@

guest/guest.elf: guest/linkerfile.ld guest/guest.o
	$(LD) $(LDFLAGS) -T $^ -o $@
	$(SZ) --format=SysV -x $@

clean_guest:
	rm -rf $(GUEST_OBJS) guest/guest.o
	rm -rf guest/built-in.a
