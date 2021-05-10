/*
 * Copyright © 2017-2021 Samuel Holland <samuel@sholland.org>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <constants.h>
#include <smp.h>
#include <sysreg.h>
#include <timer_tools.h>

#define BITS(n)     ((1 << (n)) - 1)

#define BLOCK_START 3
#define BLOCK_SIZE  (BLOCK_START + 8192)

void *
check_counter(void *context, long cpu)
{
	struct options *options = context;
	struct timespec delay   = {0};
	uint64_t data[BLOCK_SIZE];
	uint64_t start = read_cntvct();
	uint64_t fails = 0;
	uint64_t iters = 0;
	uint64_t skips = 0;

	do {
		if (options->physical) {
			for (int i = 0; i < BLOCK_SIZE; ++i)
				data[i] = read_cntpct();
		} else {
			for (int i = 0; i < BLOCK_SIZE; ++i)
				data[i] = read_cntvct();
		}

		for (int i = BLOCK_START; i < BLOCK_SIZE; ++i) {
			uint64_t orig = data[i - 3];
			uint64_t prev = data[i - 2];
			uint64_t curr = data[i - 1];
			uint64_t next = data[i - 0];
			int bad;

			if (prev <= curr)
				continue;

			/* Either "prev" is too high, or "curr" is too low. */
			if (prev > next && orig <= curr && curr <= next)
				bad = i - 2;
			else if (curr < orig && orig <= prev && prev <= next)
				bad = i - 1;
			else
				bad = -1;

			/* Skip failures covered by the existing workaround. */
			if (options->skip && bad >= 0 && ((data[bad] + 1) & BITS(10)) <= 1) {
				++skips;
				continue;
			}

			if (!fails)
				printf("%ld: Failed after %jd reads (%.6f s)\n", cpu,
				       iters * BLOCK_SIZE + i,
				       (double)(next - start) / ONE_SECOND);
			if (likely(bad >= 0))
				printf("%ld: 0x%016jx → 0x%016jx → 0x%016jx (%10.3f ms)\n", cpu,
				       data[bad - 1], data[bad], data[bad + 1],
				       (double)(int64_t)(data[bad] - data[bad - 1]) / ONE_MSEC);
			else
				printf("%ld: 0x%016jx → 0x%016jx → 0x%016jx → 0x%016jx\n", cpu,
				       orig, prev, curr, next);

			++fails;
		}
		++iters;

		/* Add a tiny but variable delay to ensure all patterns are exercised. */
		delay.tv_nsec = random() >> 20;
		nanosleep(&delay, NULL);
	} while (read_cntvct() < start + options->duration * ONE_SECOND);

	printf("%ld: Finished. %ju tries (%ju/s), %ju fails, %ju skips\n", cpu, iters * BLOCK_SIZE,
	       iters * BLOCK_SIZE / options->duration, fails, skips);

	return NULL;
}
