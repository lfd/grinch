TOOLS=tools/dump_layout tools/gcov_extract

OBJ_DIRS += $(dir $(TOOLS))

CFLAGS_TOOLS = $(CFLAGS_COMMON)
ifeq ($(CONFIG_TOOLS_DEBUG), 1)
CFLAGS_TOOLS += -ggdb
endif

.PHONY: tools
tools: $(TOOLS)

tools/%.o: tools/%.c $(config_h)
	$(QUIET) "[HOSTCC]$@"
	$(VERBOSE) $(HOSTCC) $(CFLAGS_TOOLS) $(DEPFLAGS) -c -o $@ $<

tools/dump_layout: tools/dump_layout.o
	$(QUIET) "[HOSTCC]$@"
	$(VERBOSE) $(HOSTCC) $(CFLAGS_TOOLS) -o $@ $^

tools/gcov_extract: tools/gcov_extract.o
	$(QUIET) "[HOSTCC]$@"
	$(VERBOSE) $(HOSTCC) $(CFLAGS_TOOLS) -lgcov -fprofile-arcs -o $@ $^

clean_tools:
	$(call clean_files,tools,$(TOOLS) tools/dump_layout.o tools/gcov_extract.o \
		tools/dump_layout.d tools/gcov_extract.d)
