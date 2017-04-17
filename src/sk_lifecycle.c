#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include <ck_pr.h>
#include <sk_lifecycle.h>

static const char *state_labels[] = {
    "new", "starting", "running", "stopping", "terminated", "failed",
};

const char *
sk_state_str(enum sk_state state)
{
	if (state > SK_LIFECYCLE_MAX)
		return NULL;

	return state_labels[state];
}

static_assert(sizeof(sk_lifecycle_t) % 64 == 0,
    "sk_lifecycle_t must be multiple of a cacheline");

bool
sk_lifecycle_init(sk_lifecycle_t *lfc)
{
	if (!lfc)
		return false;

	memset(lfc, 0, sizeof(*lfc));

	lfc->state = SK_STATE_NEW;
	time_t now = time(NULL);
	if (now == -1)
		return false;
	lfc->epochs[SK_STATE_NEW] = now;

	return true;
}

static bool
valid_transition(enum sk_state old, enum sk_state new);

static_assert(sizeof(time_t) == sizeof(uint64_t),
    "required due to the usage of ck_pr_store_64");

bool
sk_lifecycle_set_at_epoch(
    sk_lifecycle_t *lfc, enum sk_state new_state, time_t epoch)
{

	if (!lfc || epoch < 0)
		return false;

	for (;;) {
		const enum sk_state current_state = sk_lifecycle_get(lfc);
		if (!valid_transition(current_state, new_state))
			return false;

		/* Only update the timing & state if we transition */
		if (current_state == new_state)
			break;

		/* A concurrent writer updated the state, the 2 previous checks in the
		 * next
		 * iteration will break the loop */
		if (ck_pr_cas_int((int *)&lfc->state, current_state, new_state)) {
			ck_pr_store_64(
			    (uint64_t *)&lfc->epochs[new_state], (uint64_t)epoch);
			break;
		}
	}

	return true;
}

bool
sk_lifecycle_set(sk_lifecycle_t *lfc, enum sk_state new_state)
{
	time_t now = time(NULL);
	if (now == -1)
		return false;

	return sk_lifecycle_set_at_epoch(lfc, new_state, now);
}

static_assert(sizeof(enum sk_state) == sizeof(int),
    "sizeof(enum sk_state) must sizeof(int)");

enum sk_state
sk_lifecycle_get(const sk_lifecycle_t *lfc)
{
	if (!lfc)
		return SK_STATE_NEW;

	return ck_pr_load_int((int *)&lfc->state);
}

time_t
sk_lifecycle_get_epoch(const sk_lifecycle_t *lfc, enum sk_state state)
{
	if (!lfc)
		return -1;

	const enum sk_state current_state = sk_lifecycle_get(lfc);
	if (state > current_state)
		return 0;

	return ck_pr_load_64((uint64_t *)&lfc->epochs[state]);
}

static inline bool
valid_transition(enum sk_state from, enum sk_state to)
{
	switch (to) {
	case SK_STATE_NEW:
		return from == SK_STATE_NEW;
	case SK_STATE_STARTING:
		return (from == SK_STATE_NEW || from == SK_STATE_STARTING);
	case SK_STATE_RUNNING:
		return (from == SK_STATE_STARTING || from == SK_STATE_RUNNING);
	case SK_STATE_STOPPING:
		return (from == SK_STATE_RUNNING || from == SK_STATE_STOPPING);
	case SK_STATE_TERMINATED:
		return (from == SK_STATE_STOPPING || from == SK_STATE_TERMINATED);
	case SK_STATE_FAILED:
		return true;
	default:
		return false;
	}
}
