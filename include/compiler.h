/*
 * Copyright © 2017-2019 Samuel Holland <samuel@sholland.org>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef COMPILER_H
#define COMPILER_H

/* Attributes */
#define always_inline __attribute__((__always_inline__)) inline
#define noinline      __attribute__((__noinline__))

/* Barriers */
#define barrier()     __asm__ volatile("" : : : "memory")

/* Builtins */
#define likely(e)     __builtin_expect(!!(e), 1)
#define unlikely(e)   __builtin_expect(e, 0)
#define unreachable() __builtin_unreachable()

#endif /* COMPILER_H */
