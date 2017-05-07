#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>

#include <sk_listener.h>

#include "test.h"

bool
sum_cb(void *user_ctx, void *event_ctx, sk_error_t *error)
{
	(void)error;

	size_t *count = user_ctx;
	size_t *i = event_ctx;

	*count += *i;

	return true;
}

static void
listener_basic()
{
	char buf[16];
	const size_t n_lists = 16;
	struct {
		size_t *count;
		size_t result;
		sk_listener_t *listener;

	} contexes[n_lists];

	sk_listeners_t *listeners;
	sk_error_t error;

	assert_non_null((listeners = calloc(1, sizeof *listeners)));
	assert_true(sk_listeners_init(listeners, &error));

	for (size_t i = 0; i < n_lists; i++) {
		snprintf(buf, sizeof(buf), "%zu", i);
		contexes[i].count = calloc(1, sizeof(size_t));
		assert_non_null(contexes[i].listener = sk_listeners_register(
							listeners, buf, sum_cb, contexes[i].count, &error));
	}

	for (size_t i = 0; i < n_lists; i++) {
		assert_true(sk_listeners_observe(listeners, &i, &error));
		/* count will be freed by the next unregister call */;
		contexes[i].result = *contexes[i].count;
		sk_listeners_unregister(listeners, contexes[i].listener);
	}

	/* i-th listener should have summed 1 to i. */
	for (size_t i = 0; i < n_lists; i++) {
		assert_int_equal(contexes[i].result, i * (i + 1) / 2);
	}

	sk_listeners_destroy(listeners);
}

int
main()
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(listener_basic),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
