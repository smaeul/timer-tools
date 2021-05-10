/*
 * Copyright © 2021 Samuel Holland <samuel@sholland.org>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef TIMER_TOOLS_H
#define TIMER_TOOLS_H

struct options {
	unsigned long duration; /**< Test duration in seconds. */
	bool physical;          /**< Test the physical counter, else the virtual one. */
	bool skip;              /**< Skip errors that should be caught by the workaround. */
};

/**
 * Run a test to verify counter reads are monotonically increasing.
 */
void *check_counter(void *context, long cpu);

/**
 * Run a test to verify consistency between the counter, CVAL, and TVAL.
 */
void *check_timer(void *context, long cpu);

#endif /* TIMER_TOOLS_H */
