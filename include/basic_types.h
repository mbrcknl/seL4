/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <stdint.h>
#include <arch/types.h>

/* arch/types.h is supposed to define word_t and _seL4_word_fmt */
#ifndef _seL4_word_fmt
#error "missing _seL4_word_fmt"
#endif

#define _macro_str_concat_helper2(x)    #x
#define _macro_str_concat_helper1(x,y)  _macro_str_concat_helper2(x ## y)
#define _macro_str_concat(x,y)          _macro_str_concat_helper1(x,y)

#define SEL4_PRIu_word  _macro_str_concat(_seL4_word_fmt, u)
#define SEL4_PRIx_word  _macro_str_concat(_seL4_word_fmt, x)
#define SEL4_PRI_word   SEL4_PRIu_word


enum _bool {
    false = 0,
    true  = 1
};
typedef word_t bool_t;

typedef struct region {
    pptr_t start;
    pptr_t end;
} region_t;

typedef struct p_region {
    paddr_t start;
    paddr_t end;
} p_region_t;

typedef struct v_region {
    vptr_t start;
    vptr_t end;
} v_region_t;

#define REG_EMPTY (region_t){ .start = 0, .end = 0 }
#define P_REG_EMPTY (p_region_t){ .start = 0, .end = 0 }

/* equivalent to a word_t except that we tell the compiler that we may alias with
 * any other type (similar to a char pointer) */
typedef word_t __attribute__((__may_alias__)) word_t_may_alias;
