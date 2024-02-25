#include <errno.h>

#include "../../common/src/errno.c"

/*
 * FIXME: Whenever we get userland threads, this must be located in
 * thread-local storage
 */
int errno;
