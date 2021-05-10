/*
 * Copyright © 2017-2021 Samuel Holland <samuel@sholland.org>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <smp.h>
#include <timer_tools.h>

#define BITS(n)     ((1 << (n)) - 1)

#define BLOCK_START 3
#define BLOCK_SIZE  (BLOCK_START + 8192)

int
main(int argc, char *argv[])
{
	struct options options = {.duration = 60};
	bool parallel_counter = false;
	bool parallel_timer = false;
	bool random_counter = false;
	bool random_timer = false;
	bool any_test = false;
	int c, ret;

	while ((c = getopt(argc, argv, ":Ccd:hpsTt")) > 0) {
		switch (c) {
		case 'C':
			parallel_counter = true;
			any_test = true;
			break;
		case 'c':
			random_counter = true;
			any_test = true;
			break;
		case 'd':
			options.duration = strtoul(optarg, NULL, 0);
			break;
		case 'p':
			options.physical = true;
			break;
		case 's':
			options.skip = true;
			break;
		case 'T':
			parallel_timer = true;
			any_test = true;
			break;
		case 't':
			random_timer = true;
			any_test = true;
			break;
		case 'h':
		default:
			ret = c != 'h';
			fprintf(ret ? stderr : stdout,
				"usage: %s -CcTt [-d DURATION] [-p] [-s]\n\n"
				"  -C  run parallel counter test\n"
				"  -c  run random counter test\n"
				"  -T  run parallel timer test\n"
				"  -t  run random timer test\n\n"
				"  -d  test duration (seconds)\n"
				"  -p  use physical counter\n"
				"  -s  skip errors caught by kernel workaround\n",
				argv[0]);
			return ret;
		}
	}

	if (!any_test) {
		printf("error: At least one test (-CcTt) is required\n");
		return 1;
	}

	setvbuf(stdout, NULL, _IOLBF, BUFSIZ);
	setvbuf(stderr, NULL, _IOLBF, BUFSIZ);
	srandom(4);

	if (parallel_counter) {
		puts("Running parallel counter test...");
		ret = run_on_all_cpus(check_counter, NULL, NULL, &options, NULL);
		if (ret)
			return ret;
	}

	if (random_counter) {
		puts("Running random counter test...");
		ret = run_on_random_cpu(check_counter, NULL, &options, NULL);
		if (ret)
			return ret;
	}

	if (parallel_timer) {
		puts("Running parallel timer test...");
		ret = run_on_all_cpus(check_timer, NULL, NULL, &options, NULL);
		if (ret)
			return ret;
	}

	if (random_timer) {
		puts("Running random timer test...");
		ret = run_on_random_cpu(check_timer, NULL, &options, NULL);
		if (ret)
			return ret;
	}

	return 0;
}
