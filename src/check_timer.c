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

#define BITS(n) ((1 << (n)) - 1)

struct options {
	unsigned long duration;
};

void *
check_timer(void *context, long cpu)
{
	struct options *options = context;
	uint64_t after, before, compare, tval;
	uint64_t start  = read_cntvct();
	uint64_t rfails = 0;
	uint64_t wfails = 0;
	uint64_t iters  = 0;
	uint64_t skips  = 0;

	do {
		/* Choose a random timer value. */
		tval = random();

		/* Program the timer by writing TVAL. */
		before = read_cntvct();
		write_cntv_tval(tval);
		after = read_cntvct();

		/* Cannot test the timer if the counter is broken. */
		if (before > after) {
			++skips;
			continue;
		}

		/* Compute the counter value at the time TVAL was written. */
		compare = read_cntv_cval() - tval;

		if (compare < before || compare > after) {
			if (!wfails)
				printf("%ld: Failed after %jd TVAL writes (%.6f s)\n", cpu, iters,
				       (double)(after - start) / ONE_SECOND);
			printf("%ld: CVAL 0x%016jx → 0x%016jx → 0x%016jx (%10.3f ms)\n", cpu,
			       before, compare, after,
			       (double)(int64_t)(before + after - 2 * compare) / (2 * ONE_MSEC));
			++wfails;
		}

		/* Read back TVAL and the new counter value. */
		compare = read_cntv_tval();
		after   = read_cntvct();

		/* Cannot test the timer if the counter is broken. */
		if (before > after) {
			++skips;
			continue;
		}

		/* Fail if TVAL decreased by more than the counter increased. */
		if (compare < tval - (after - before) || compare > tval) {
			if (!rfails)
				printf("%ld: Failed after %jd TVAL reads (%.6f s)\n", cpu, iters,
				       (double)(after - start) / ONE_SECOND);
			printf("%ld: TVAL 0x%016jx → 0x%016jx → 0x%016jx\n", cpu,
			       tval - (after - before), compare, tval);
			++rfails;
		}

		++iters;
	} while (after < start + options->duration * ONE_SECOND);

	printf("%ld: Finished. %ju tries (%ju/s), %ju read fails, %ju write fails, %ju skips\n",
	       cpu, iters, iters / options->duration, rfails, wfails, skips);

	return NULL;
}

int
main(int argc, char *argv[])
{
	struct options options = {.duration = 60};
	int c, ret;

	while ((c = getopt(argc, argv, ":d:h")) > 0) {
		switch (c) {
		case 'd':
			options.duration = strtoul(optarg, NULL, 0);
			break;
		case 'h':
		default:
			printf("usage: %s [-d DURATION]\n", argv[0]);
			return c != 'h';
		}
	}

	setvbuf(stdout, NULL, _IOLBF, BUFSIZ);
	setvbuf(stderr, NULL, _IOLBF, BUFSIZ);
	srandom(4);

	ret = run_on_all_cpus(check_timer, NULL, NULL, &options, NULL);
	if (ret)
		return ret;

	return 0;
}
