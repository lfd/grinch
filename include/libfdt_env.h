#ifndef _LIBFDT_ENV_H
#define _LIBFDT_ENV_H

#include <grinch/types.h>
#include <grinch/string.h>

#define fdt32_to_cpu(x) __be32_to_cpu(x)
#define cpu_to_fdt32(x) __cpu_to_be32(x)
#define fdt64_to_cpu(x) __be64_to_cpu(x)
#define cpu_to_fdt64(x) __cpu_to_be64(x)

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
static inline u32 __swab32(const u32 x)
{
	const u8 *_x = (u8*)&x;
	union {
		u8 raw[sizeof(u32)];
		u32 ret;
	} ret;

	ret.raw[0] = _x[3];
	ret.raw[1] = _x[2];
	ret.raw[2] = _x[1];
	ret.raw[3] = _x[0];

	return ret.ret;
}

static inline u64 __swab64(const u64 x)
{
	const u32 *_x = (u32*)&x;
	u64 ret;

	ret = ((u64)__swab32(_x[0]) << 32) | __swab32(_x[1]);

	return ret;
}

#define __be32_to_cpu(x) __swab32((u32)(x))
#define __cpu_to_be32(x) __swab32((u32)(x))

#define __be64_to_cpu(x) __swab64((u64)(x))
#define __cpu_to_be64(x) __swab64((u64)(x))

#else
#error "NOT IMPLEMENTED"
#endif

typedef u8 uint8_t;
typedef u16 uint16_t;
typedef u32 uint32_t;
typedef u64 uint64_t;

typedef s32 int32_t;

typedef uint16_t fdt16_t;
typedef uint32_t fdt32_t;
typedef uint64_t fdt64_t;

#endif /* _LIBFDT_ENV_H */
