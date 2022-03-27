#ifndef __COMPILER_ATTRIBUTES_H
#define __COMPILER_ATTRIBUTES_H

#define __always_inline inline __attribute__((always_inline))

#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)

#endif /* __COMPILER_ATTRIBUTES_H */
