TOOLS=tools/dump_layout tools/gcov_extract

OBJ_DIRS += $(dir $(TOOLS))

CFLAGS_TOOLS = $(CFLAGS_COMMON)

.PHONY: tools
tools: $(TOOLS)

tools/%.o: tools/%.c $(config_h)
	$(QUIET) "[HOSTCC]$@"
	$(VERBOSE) $(HOSTCC) $(CFLAGS_TOOLS) $(DEPFLAGS) -ggdb -c -o $@ $<

tools/dump_layout: tools/dump_layout.o
	$(QUIET) "[HOSTCC]$@"
	$(VERBOSE) $(HOSTCC) $(CFLAGS_TOOLS) -o $@ $^

tools/gcov_extract: tools/gcov_extract.o
	$(QUIET) "[HOSTCC]$@"
	$(VERBOSE) $(HOSTCC) $(CFLAGS_TOOLS) -lgcov -fprofile-arcs -o $@ $^

clean_tools:
	$(call clean_files,tools,$(TOOLS) tools/dump_layout.o tools/gcov_extract.o)
