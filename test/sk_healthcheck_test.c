#include <ck_pr.h>
#include <sk_healthcheck.h>

#include "test.h"

enum sk_health
bool_healthcheck(const void *opaque, sk_error_t *err)
{
	if (opaque == NULL)
		return SK_HEALTH_UNKNOWN;

	int *state = (int *)opaque;

	if (ck_pr_load_int(state)) {
		return SK_HEALTH_OK;
	} else {
		sk_error_msg_code(err, "int is in a critical state", 1);
		return SK_HEALTH_CRITICAL;
	}
}

void
healthcheck_test_basic()
{
	enum sk_health health = SK_HEALTH_UNKNOWN;
	sk_healthcheck_t hc;
	sk_error_t error;
	int state = 1;

	assert_true(sk_healthcheck_init(
	    &hc, "basic", "", 0, bool_healthcheck, &state, &error));

	assert_true(sk_healthcheck_poll(&hc, &health, &error));
	assert_int_equal(health, SK_HEALTH_OK);

	ck_pr_store_int(&state, 0);
	assert_true(sk_healthcheck_poll(&hc, &health, &error));
	assert_int_equal(health, SK_HEALTH_CRITICAL);

	/* Can't poll a disabled check */
	sk_healthcheck_disable(&hc);
	assert_false(sk_healthcheck_poll(&hc, &health, &error));
}

int
main()
{
	const struct CMUnitTest tests[] = {
	    cmocka_unit_test(healthcheck_test_basic),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
