/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <assert.h>
#include <stdint.h>
#include <util.h>

/*
 * memzero needs a custom type that allows us to use a word
 * that has the aliasing properties of a char.
 */
typedef unsigned long __attribute__((__may_alias__)) ulong_alias;

/*
 * Zero 'n' bytes of memory starting from 's'.
 *
 * 'n' and 's' must be word aligned.
 */
void memzero(void *s, unsigned long n)
{
    uint8_t *p = s;

    /* Ensure alignment constraints are met. */
    assert((unsigned long)s % sizeof(unsigned long) == 0);
    assert(n % sizeof(unsigned long) == 0);

    /* We will never memzero an area larger than the largest current
       live object */
    /** GHOSTUPD: "(gs_get_assn cap_get_capSizeBits_'proc \<acute>ghost'state = 0
        \<or> \<acute>n <= gs_get_assn cap_get_capSizeBits_'proc \<acute>ghost'state, id)" */

    /* Write out words. */
    while (n != 0) {
        *(ulong_alias *)p = 0;
        p += sizeof(ulong_alias);
        n -= sizeof(ulong_alias);
    }
}

void *VISIBLE memset(void *s, unsigned long c, unsigned long n)
{
    uint8_t *p;

    /*
     * If we are only writing zeros and we are word aligned, we can
     * use the optimized 'memzero' function.
     */
    if (likely(c == 0 && ((unsigned long)s % sizeof(unsigned long)) == 0 && (n % sizeof(unsigned long)) == 0)) {
        memzero(s, n);
    } else {
        /* Otherwise, we use a slower, simple memset. */
        for (p = (uint8_t *)s; n > 0; n--, p++) {
            *p = (uint8_t)c;
        }
    }

    return s;
}

void *VISIBLE memcpy(void *ptr_dst, const void *ptr_src, unsigned long n)
{
    uint8_t *p;
    const uint8_t *q;

    for (p = (uint8_t *)ptr_dst, q = (const uint8_t *)ptr_src; n; n--, p++, q++) {
        *p = *q;
    }

    return ptr_dst;
}

int PURE strncmp(const char *s1, const char *s2, int n)
{
    word_t i;
    int diff;

    for (i = 0; i < n; i++) {
        diff = ((unsigned char *)s1)[i] - ((unsigned char *)s2)[i];
        if (diff != 0 || s1[i] == '\0') {
            return diff;
        }
    }

    return 0;
}

long CONST char_to_long(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    } else if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    return -1;
}

long PURE str_to_long(const char *str)
{
    unsigned int base;
    long res;
    long val = 0;
    char c;

    /*check for "0x" */
    if (*str == '0' && (*(str + 1) == 'x' || *(str + 1) == 'X')) {
        base = 16;
        str += 2;
    } else {
        base = 10;
    }

    if (!*str) {
        return -1;
    }

    c = *str;
    while (c != '\0') {
        res = char_to_long(c);
        if (res == -1 || res >= base) {
            return -1;
        }
        val = val * base + res;
        str++;
        c = *str;
    }

    return val;
}

// The following implementations of CLZ (count leading zeros) and CTZ (count
// trailing zeros) perform a binary search for the first 1 bit from the
// beginning (resp. end) of the input. Initially, the focus is the whole input.
// Then, each iteration determines whether there are any 1 bits set in the
// upper (resp. lower) half of the current focus. If there are (resp. are not),
// then the upper half is shifted into the lower half. Either way, the lower
// half of the current focus becomes the new focus for the next iteration.
// After enough iterations (6 for 64-bit inputs, 5 for 32-bit inputs), the
// focus is reduced to a single bit, and the total number of bits shifted can
// be used to determine the number of zeros before (resp. after) the first 1
// bit.
//
// Although the details vary, the general approach is used in several library
// implementations, including in LLVM and GCC. Wikipedia has some references:
// https://en.wikipedia.org/wiki/Find_first_set
//
// This implementation avoids branching. The test that determines whether the
// upper (resp. lower) half contains any ones directly produces a number which
// can be used for an unconditional shift. If the upper (resp. lower) half is
// all zeros, the test produces a zero, and the shift is a no-op.  A branchless
// implementation has the disadvantage that it requires more instructions to
// execute than one which branches, but the advantage is that none will be
// mispredicted branches. Whether this is a good tradeoff depends on the branch
// predictability and the architecture's pipeline depth. The most critical use
// of clzl in the kernel is in the scheduler priority queue. In the absence of
// a concrete application and hardware implementation to evaluate the tradeoff,
// we somewhat arbitrarily choose a branchless implementation.

// Equivalent to BIT(1 << i) and MASK(1 << i), also parameterised by type.
#define BIT_CZ(typ, i) (((typ)1) << (1 << (i)))
#define MASK_CZ(typ, i) (BIT_CZ(typ, i) - ((typ)1))

// Check some assumptions made by the implementations.
compile_assert(clz_ulong_32_or_64, sizeof(unsigned long) == 4 || sizeof(unsigned long) == 8);
compile_assert(clz_ullong_64, sizeof(unsigned long long) == 8);

// Count leading zeros.
#ifndef CONFIG_CLZ_BUILTIN
// Each iteration i (counting backwards) considers the least significant
// 2^(i+1) bits of x as the current focus. It assumes x contains no 1 bits
// outside the focus. The test determines whether there are any 1 bits in the
// upper half (2^i bits) of the focus, setting `bits` to 2^i if there are, or
// zero if not. Shifting by `bits` then narrows the focus to the lower 2^i bits
// and satisfies the assumption for the next iteration. Since `bits` is always
// a power of two, we use logical OR to accumulate a count of the number of
// bits shifted.  At the last iteration, the most significant 1 bit of x has
// been shifted to bit 0, and (w - 1 - count) gives the number of leading
// zeros, where w is the word size, and count is the total shift.
#define CLZ_STEP(typ, i) \
    bits = ((int)(BIT_CZ(typ, i) <= x)) << (i); \
    x >>= bits; \
    count |= bits;

static inline int clz32(uint32_t x)
{
    // The result is undefined if all bits are zero.
    int count = 0, bits;
    CLZ_STEP(uint32_t, 4);
    CLZ_STEP(uint32_t, 3);
    CLZ_STEP(uint32_t, 2);
    CLZ_STEP(uint32_t, 1);
    // For shift is not necessary in the final iteration.
    return 31 - (count | (x >> 1));
}

static inline int clz64(uint64_t x)
{
    // The result is undefined if all bits are zero.
    int count = 0, bits;
    CLZ_STEP(uint64_t, 5);
    CLZ_STEP(uint64_t, 4);
    CLZ_STEP(uint64_t, 3);
    CLZ_STEP(uint64_t, 2);
    CLZ_STEP(uint64_t, 1);
    return 63 - (count | (x >> 1));
}
#undef CLZ_STEP

static int clzl_impl(unsigned long x)
{
    return sizeof(unsigned long) == 8 ? clz64(x) : clz32(x);
}

static int clzll_impl(unsigned long long x)
{
    return clz64(x);
}
#endif // CONFIG_CLZ_BUILTIN

// Count trailing zeros.
#if !defined(CONFIG_CTZ_BUILTIN) && !defined(CONFIG_CLZ_BUILTIN)
// Each iteration i (counting backwards) considers the least significant
// 2^(i+1) bits of x as the current focus. The test determines whether there
// are any 1 bits in the lower half (2^i bits) of the focus, setting `bits` to
// zero if there are, or 2^i if not. Shifting by `bits` then narrows the focus
// to the lower 2^i bits for the next iteration. Since `bits` is always a power
// of two, we use logical OR to accumulate a count of the number of bits
// shifted. At the last iteration, the least significant 1 bit of x has been
// shifted to bit 0, and the total shift gives the number of trailing zeros.
#define CTZ_STEP(typ, i) \
    bits = ((int)((x & MASK_CZ(typ, i)) == 0)) << (i); \
    x >>= bits; \
    count |= bits;

static inline int ctz32(uint32_t x)
{
    // The result is undefined if all bits are zero.
    int count = 0, bits;
    CTZ_STEP(uint32_t, 4);
    CTZ_STEP(uint32_t, 3);
    CTZ_STEP(uint32_t, 2);
    CTZ_STEP(uint32_t, 1);
    return count | ((x & 1) == 0);
}

static inline int ctz64(uint64_t x)
{
    // The result is undefined if all bits are zero.
    int count = 0, bits;
    CTZ_STEP(uint64_t, 5);
    CTZ_STEP(uint64_t, 4);
    CTZ_STEP(uint64_t, 3);
    CTZ_STEP(uint64_t, 2);
    CTZ_STEP(uint64_t, 1);
    return count | ((x & 1) == 0);
}
#undef CTZ_STEP

static int ctzl_impl(unsigned long x)
{
    return sizeof(unsigned long) == 8 ? ctz64(x) : ctz32(x);
}

static int ctzll_impl(unsigned long long x)
{
    return ctz64(x);
}
#endif // CONFIG_CTZ_BUILTIN || CONFIG_CLZ_BUILTIN

#undef MASK_CZ
#undef BIT_CZ
