TOOLS=tools/dump_layout

.PHONY: tools
tools: $(TOOLS)

tools/%.o: tools/%.c
	$(QUIET) "[HOSTCC]$@"
	$(VERBOSE) $(HOSTCC) $(CFLAGS_COMMON) -c -o $@ $^

tools/dump_layout: tools/dump_layout.o
	$(QUIET) "[HOSTCC]$@"
	$(VERBOSE) $(HOSTCC) $(CFLAGS_COMMON) -o $@ $^

clean_tools:
	rm -f tools/dump_layout
	rm -f tools/*.o
