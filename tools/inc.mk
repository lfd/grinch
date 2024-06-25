TOOLS=tools/dump_layout tools/gcov_extract

.PHONY: tools
tools: $(TOOLS)

tools/%.o: tools/%.c
	$(QUIET) "[HOSTCC]$@"
	$(VERBOSE) $(HOSTCC) $(CFLAGS_COMMON) -ggdb -c -o $@ $^

tools/dump_layout: tools/dump_layout.o
	$(QUIET) "[HOSTCC]$@"
	$(VERBOSE) $(HOSTCC) $(CFLAGS_COMMON) -o $@ $^

tools/gcov_extract: tools/gcov_extract.o
	$(QUIET) "[HOSTCC]$@"
	$(VERBOSE) $(HOSTCC) $(CFLAGS_COMMON) -lgcov -fprofile-arcs -o $@ $^

clean_tools:
	$(call clean_files,tools,$(TOOLS) tools/dump_layout.o tools/gcov_extract.o)
