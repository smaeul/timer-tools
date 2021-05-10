/*
 * Copyright © 2017-2019 Samuel Holland <samuel@sholland.org>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef SYSREG_H
#define SYSREG_H

#include <stdint.h>

#define DEFINE_SYSREG_OPS(reg)                                                                    \
	static always_inline uint64_t read_##reg(void)                                            \
	{                                                                                         \
		uint64_t val;                                                                     \
		__asm__ volatile("mrs %0, " #reg "_el0" : "=r"(val));                             \
		return val;                                                                       \
	}                                                                                         \
	static always_inline void write_##reg(uint64_t val)                                       \
	{                                                                                         \
		__asm__ volatile("msr " #reg "_el0, %0" : : "r"(val));                            \
	}

DEFINE_SYSREG_OPS(cntp_ctl)
DEFINE_SYSREG_OPS(cntp_cval)
DEFINE_SYSREG_OPS(cntp_tval)
DEFINE_SYSREG_OPS(cntpct)
DEFINE_SYSREG_OPS(cntv_ctl)
DEFINE_SYSREG_OPS(cntv_cval)
DEFINE_SYSREG_OPS(cntv_tval)
DEFINE_SYSREG_OPS(cntvct)

#undef DEFINE_SYSREG_OPS

#endif /* SYSREG_H */
