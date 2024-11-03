TOOLS=tools/dump_layout tools/gcov_extract

CFLAGS_TOOLS = $(CFLAGS_COMMON) $(CFLAGS_DEF_ARCH)

.PHONY: tools
tools: $(TOOLS)

tools/%.o: tools/%.c
	$(QUIET) "[HOSTCC]$@"
	$(VERBOSE) $(HOSTCC) $(CFLAGS_TOOLS) -ggdb -c -o $@ $^

tools/dump_layout: tools/dump_layout.o
	$(QUIET) "[HOSTCC]$@"
	$(VERBOSE) $(HOSTCC) $(CFLAGS_TOOLS) -o $@ $^

tools/gcov_extract: tools/gcov_extract.o
	$(QUIET) "[HOSTCC]$@"
	$(VERBOSE) $(HOSTCC) $(CFLAGS_TOOLS) -lgcov -fprofile-arcs -o $@ $^

clean_tools:
	$(call clean_files,tools,$(TOOLS) tools/dump_layout.o tools/gcov_extract.o)
