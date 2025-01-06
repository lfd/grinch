/* SPDX-License-Identifier: GPL-2.0 */

/* Copied from the Linux kernel Sources */

#ifndef _REFCOUNT_H
#define _REFCOUNT_H

#include <grinch/compiler_attributes.h>
#include <grinch/atomic.h>
#include <grinch/limits.h>

#define REFCOUNT_INIT(n)	{ .refs = ATOMIC_INIT(n), }
#define REFCOUNT_SATURATED	(INT_MIN / 2)

enum refcount_saturation_type {
	REFCOUNT_ADD_NOT_ZERO_OVF,
	REFCOUNT_ADD_OVF,
	REFCOUNT_ADD_UAF,
	REFCOUNT_SUB_UAF,
	REFCOUNT_DEC_LEAK,
};

typedef struct refcount_struct {
	atomic_t refs;
} refcount_t;

void refcount_warn_saturate(refcount_t *r, enum refcount_saturation_type t);

static inline unsigned int refcount_read(const refcount_t *r)
{
	return atomic_read(&r->refs);
}

static inline void refcount_set(refcount_t *r, int n)
{
	atomic_set(&r->refs, n);
}

static inline /* __signed_wrap */
void __refcount_add(int i, refcount_t *r, int *oldp)
{
	int old = atomic_fetch_add_relaxed(i, &r->refs);

	if (oldp)
		*oldp = old;

	if (unlikely(!old))
		refcount_warn_saturate(r, REFCOUNT_ADD_UAF);
	else if (unlikely(old < 0 || old + i < 0))
		refcount_warn_saturate(r, REFCOUNT_ADD_OVF);
}

static inline __must_check /* __signed_wrap */
bool __refcount_add_not_zero(int i, refcount_t *r, int *oldp)
{
	int old = refcount_read(r);

	do {
		if (!old)
			break;
	} while (!atomic_try_cmpxchg_relaxed(&r->refs, &old, old + i));

	if (oldp)
		*oldp = old;

	if (unlikely(old < 0 || old + i < 0))
		refcount_warn_saturate(r, REFCOUNT_ADD_NOT_ZERO_OVF);

	return old;
}

static inline __must_check /* __signed_wrap */
bool __refcount_sub_and_test(int i, refcount_t *r, int *oldp)
{
	int old = atomic_fetch_sub_release(i, &r->refs);

	if (oldp)
		*oldp = old;

	if (old > 0 && old == i) {
		return true;
	}

	if (unlikely(old <= 0 || old - i < 0))
		refcount_warn_saturate(r, REFCOUNT_SUB_UAF);

	return false;
}

static inline __must_check bool __refcount_inc_not_zero(refcount_t *r, int *oldp)
{
	return __refcount_add_not_zero(1, r, oldp);
}

static inline __must_check bool refcount_inc_not_zero(refcount_t *r)
{
	return __refcount_inc_not_zero(r, NULL);
}

static inline __must_check bool __refcount_dec_and_test(refcount_t *r, int *oldp)
{
	return __refcount_sub_and_test(1, r, oldp);
}

static inline __must_check bool refcount_dec_and_test(refcount_t *r)
{
	return __refcount_dec_and_test(r, NULL);
}

static inline void __refcount_inc(refcount_t *r, int *oldp)
{
	__refcount_add(1, r, oldp);
}

static inline void refcount_inc(refcount_t *r)
{
	__refcount_inc(r, NULL);
}

#endif /* _REFCOUNT_H */
