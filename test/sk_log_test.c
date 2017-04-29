#include <sk_log.h>
#include <sk_logger_drv.h>

#include "test.h"

static void
logger_basic()
{
	sk_logger_t *logger;
	sk_error_t error;
	size_t drained = 0;

	sk_logger_drv_set_default(sk_logger_drv_builder_tally, NULL);

	assert_non_null(
	    (logger = sk_logger_create("basic_logger", 4, NULL, &error)));
	assert_int_equal(sk_logger_get_level(logger), SK_LOG_DEFAULT_LEVEL);

	for (int i = 0; i < SK_LOG_COUNT; i++) {
		assert_true(sk_logger_set_level(logger, i));
		assert_int_equal(sk_logger_get_level(logger), i);
		assert_true(sk_log(logger, i, sk_debug, "hello (line %d)", i));
	}

	assert_true(sk_logger_drain(logger, &drained));
	assert_int_equal(drained, SK_LOG_COUNT);

	const sk_logger_drv_tally_ctx_t *tally_ctx = logger->driver.ctx;
	for (int i = 0; i < SK_LOG_COUNT; i++)
		assert_int_equal(tally_ctx->counters[i], 1);

	sk_logger_destroy(logger);
}

static void
logger_lazy_level()
{
	sk_error_t error;
	sk_logger_t *logger;
	size_t drained = 0;

	sk_logger_drv_set_default(sk_logger_drv_builder_tally, NULL);

	assert_non_null(
	    (logger = sk_logger_create("lazy_logger", 4, NULL, &error)));
	assert_int_equal(sk_logger_get_level(logger), SK_LOG_DEFAULT_LEVEL);

	for (int i = 0; i < SK_LOG_COUNT; i++) {
		assert_true(sk_logger_set_level(logger, i));
		for (int j = 0; j < SK_LOG_COUNT; j++)
			assert_true(sk_log(logger, j, sk_debug, "%d", j));
		assert_true(sk_logger_drain(logger, &drained));
		assert_int_equal(drained, i + 1);
	}

	assert_true(sk_logger_drain(logger, &drained));
	assert_int_equal(drained, 0);

	const sk_logger_drv_tally_ctx_t *tally_ctx = logger->driver.ctx;
	for (int i = 0; i < SK_LOG_COUNT; i++)
		assert_int_equal(tally_ctx->counters[i], SK_LOG_COUNT - i);

	sk_logger_destroy(logger);
}

int
main()
{
	const struct CMUnitTest tests[] = {
	    cmocka_unit_test(logger_basic), cmocka_unit_test(logger_lazy_level),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
