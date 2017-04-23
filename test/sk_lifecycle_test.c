#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>

#include <sk_lifecycle.h>

#include "test.h"

void
lifecycle_basic()
{
	sk_lifecycle_t lfc;
	sk_error_t err;

	assert_true(sk_lifecycle_init(&lfc, &err));

	/* State machine shall start at `SK_STATE_NEW` */
	assert_int_equal(sk_lifecycle_get(&lfc), SK_STATE_NEW);

	/* Can't set an invalid time */
	assert_false(sk_lifecycle_set_at_epoch(&lfc, SK_STATE_STARTING, -1, &err));
	assert_int_equal(err.code, SK_LIFECYCLE_EINVAL);

	/* Test transition matrix and epoch */
	for (size_t i = 1; i < SK_STATE_COUNT; i++) {
		assert_true(sk_lifecycle_set_at_epoch(&lfc, i, i, &err));
		assert_int_equal(sk_lifecycle_get(&lfc), i);
		assert_int_equal(sk_lifecycle_get_epoch(&lfc, i), i);

		/* Can't transition to backward state */
		for (size_t j = 0; j < i; j++) {
			err.code = 0;
			assert_false(sk_lifecycle_set(&lfc, j, &err));
			assert_int_equal(err.code, SK_LIFECYCLE_EINVAL);
		}
	}
}

struct lifecycle_worker_ctx {
	atomic_bool *thread_started;
	atomic_bool *workers_ready;
	enum sk_state state;
	sk_lifecycle_t *lfc;
};

void *
lifecycle_worker(void *opaque)
{
	struct lifecycle_worker_ctx *ctx = (struct lifecycle_worker_ctx *)opaque;
	const enum sk_state state = ctx->state;
	sk_error_t err;

	*ctx->thread_started = true;

	for (;;)
		if (*ctx->workers_ready)
			break;

	for (;;)
		if (sk_lifecycle_set_at_epoch(ctx->lfc, state, (int)state, &err))
			break;

	return NULL;
}

/* Simulates 4 workers that concurrently try to advanced the state machine
 * to their unique assigned state in {STARTING,RUNNING,STOPPING,TERMINATED}. */
void
lifecycle_threaded()
{
	sk_lifecycle_t lfc;
	const uint8_t n_threads = 4;
	pthread_t workers[n_threads];
	struct lifecycle_worker_ctx contexes[n_threads];
	atomic_bool workers_ready = false, thread_started = false;
	sk_error_t err;

	assert_true(sk_lifecycle_init(&lfc, &err));

	/* Start all threads, but stall them until ready */
	for (size_t i = 0; i < n_threads; i++) {
		thread_started = false;
		contexes[i].thread_started = &thread_started;
		contexes[i].workers_ready = &workers_ready;
		contexes[i].state = i + 1;
		contexes[i].lfc = &lfc;
		pthread_create(&workers[i], NULL, lifecycle_worker, &contexes[i]);
		while (!thread_started)
			;
	}

	/* Unlock all threads */
	workers_ready = true;

	/* Wait for completion */
	for (size_t i = 0; i < n_threads; i++) {
		pthread_join(workers[i], NULL);
	}

	for (size_t i = 0; i < n_threads; i++) {
		int state = i + 1;
		assert_int_equal(sk_lifecycle_get_epoch(&lfc, state), state);
	}

	assert_int_equal(sk_lifecycle_get(&lfc), SK_STATE_TERMINATED);
}

int
main()
{
	const struct CMUnitTest tests[] = {
	    cmocka_unit_test(lifecycle_basic), cmocka_unit_test(lifecycle_threaded),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
