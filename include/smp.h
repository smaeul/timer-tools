/*
 * Copyright © 2020-2021 Samuel Holland <samuel@sholland.org>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef SMP_H
#define SMP_H

#include <pthread.h>

typedef void (*fold_fn)(void *accum, void *context, long cpu, void *value);
typedef void *(*worker_fn)(void *context, long cpu);

/**
 * Run `worker` on each CPU, passing it the context and CPU number; then run `fold` across the
 * returned values, using `result` as the accumulator.
 *
 * Returns 0 on success or errno on failure.
 */
int run_on_all_cpus(worker_fn worker, fold_fn fold, const pthread_attr_t *attr, void *context,
                    void *result);

#endif /* SMP_H */
