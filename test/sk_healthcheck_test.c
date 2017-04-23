#include <ck_pr.h>
#include <sk_healthcheck.h>

#include "test.h"

enum sk_health_state
bool_healthcheck(const void *opaque, sk_error_t *err)
{
	if (opaque == NULL)
		return SK_HEALTH_STATE_UNKNOWN;

	int *state = (int *)opaque;

	if (ck_pr_load_int(state)) {
		return SK_HEALTH_STATE_OK;
	} else {
		sk_error_msg_code(err, "bool is in a critical state", 1);
		return SK_HEALTH_STATE_CRITICAL;
	}
}

void
healthcheck_test_basic()
{
	sk_healthcheck_t hc;
	int state = 1;

	assert_false(sk_healthcheck_init(NULL, NULL, NULL, 0, NULL, NULL));

	assert_true(
	    sk_healthcheck_init(&hc, "basic", "", 0, bool_healthcheck, &state));

	enum sk_health_state health = SK_HEALTH_STATE_UNKNOWN;
	sk_error_t error;

	assert_true(sk_healthcheck_poll(&hc, &health, &error));
	assert_int_equal(health, SK_HEALTH_STATE_OK);

	ck_pr_store_int(&state, 0);
	assert_true(sk_healthcheck_poll(&hc, &health, &error));
	assert_int_equal(health, SK_HEALTH_STATE_CRITICAL);

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
