#include <grinch/string.h>
#include <grinch/alloc.h>

#define STRDUP		kstrdup
#define STRNDUP		kstrndup
#define ALLOCATOR	kmalloc

#include "../common/src/string.c"
