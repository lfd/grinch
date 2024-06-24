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
	$(call clean_files,tools,$(TOOLS) tools/dump_layout.o)
