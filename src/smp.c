/*
 * Copyright © 2017-2021 Samuel Holland <samuel@sholland.org>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

#include <constants.h>
#include <smp.h>
#include <sysreg.h>

struct thread {
	void *context;
	long cpu;
	worker_fn worker;
	sem_t barrier;
	pthread_t tid;
};

static void
dummy_fold(void *accum, void *context, long cpu, void *value)
{
	(void)accum;
	(void)context;
	(void)cpu;
	(void)value;
}

static void *
worker_wrapper(void *arg)
{
	struct thread *t = arg;

	sem_wait(&t->barrier);

	return t->worker(t->context, t->cpu);
}

int
run_on_all_cpus(worker_fn worker, fold_fn fold, const pthread_attr_t *attr, void *context,
                void *result)
{
	struct thread *threads;
	size_t cpu_set_size;
	cpu_set_t *cpu_set;
	long nproc;
	int ret;

	if (!worker)
		return EINVAL;
	if (!fold)
		fold = dummy_fold;

	nproc = sysconf(_SC_NPROCESSORS_ONLN);
	if (nproc < 1)
		nproc = 1;

	cpu_set_size = CPU_ALLOC_SIZE(nproc);
	cpu_set      = CPU_ALLOC(nproc);
	threads      = calloc(nproc, sizeof(struct thread));
	if (!cpu_set || !threads)
		return ENOMEM;

	for (long cpu = 0; cpu < nproc; ++cpu) {
		struct thread *t = &threads[cpu];

		t->context = context;
		t->cpu     = cpu;
		t->worker  = worker;

		ret = sem_init(&t->barrier, 0, 0);
		if (ret) {
			ret = errno;
			break;
		}

		ret = pthread_create(&t->tid, attr, worker_wrapper, t);
		if (ret)
			break;

		CPU_ZERO_S(cpu_set_size, cpu_set);
		CPU_SET_S(cpu, cpu_set_size, cpu_set);
		ret = pthread_setaffinity_np(t->tid, cpu_set_size, cpu_set);
		sem_post(&t->barrier);
		if (ret)
			break;
	}

	for (long cpu = 0; cpu < nproc; ++cpu) {
		struct thread *t = &threads[cpu];
		void *value;

		if (t->tid && !pthread_join(t->tid, &value))
			fold(result, context, t->cpu, value);

		sem_destroy(&t->barrier);
	}

	CPU_FREE(cpu_set);
	free(threads);

	return ret;
}

int
run_on_random_cpu(worker_fn worker, const pthread_attr_t *attr, void *context, void **result)
{
	struct thread thread, *t = &thread;
	long nproc;
	int ret;

	if (!worker)
		return EINVAL;

	nproc = sysconf(_SC_NPROCESSORS_ONLN);
	if (nproc < 1)
		nproc = 1;

	t->context = context;
	t->cpu     = 0;
	t->worker  = worker;

	ret = sem_init(&t->barrier, 0, 0);
	if (ret)
		return errno;

	ret = pthread_create(&t->tid, attr, worker_wrapper, t);
	if (ret)
		goto err_destroy;

	sem_post(&t->barrier);

	if (nproc > 1) {
		size_t cpu_set_size   = CPU_ALLOC_SIZE(nproc);
		cpu_set_t *cpu_set    = CPU_ALLOC(nproc);
		struct timespec delay = {0};
		long cpu              = 0;

		if (!cpu_set)
			goto err_join;

		do {
			CPU_ZERO_S(cpu_set_size, cpu_set);
			CPU_SET_S(cpu, cpu_set_size, cpu_set);
			pthread_setaffinity_np(t->tid, cpu_set_size, cpu_set);

			cpu           = random() % nproc;
			delay.tv_nsec = random() >> 2;
			nanosleep(&delay, NULL);
		} while ((ret = pthread_tryjoin_np(t->tid, result)) == EBUSY);

		CPU_FREE(cpu_set);
	} else {
err_join:
		ret = pthread_join(t->tid, result);
	}

err_destroy:
	sem_destroy(&t->barrier);

	return ret;
}
