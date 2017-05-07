#include <assert.h>
#include <error.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <ck_pr.h>

#include <sk_lifecycle.h>

// clang-format off
static const char *state_labels[] =
{
	[SK_STATE_NEW] = "new",
	[SK_STATE_STARTING] = "starting",
	[SK_STATE_RUNNING] = "running",
	[SK_STATE_STOPPING] = "stopping",
	[SK_STATE_TERMINATED] = "terminated",
	[SK_STATE_FAILED] = "failed",
};
// clang-format on

static_assert(sk_array_size(state_labels) == SK_STATE_COUNT,
	"state labels size not matching enum");

const char *
sk_state_str(enum sk_state state)
{
	return (state < SK_STATE_COUNT) ? state_labels[state] : NULL;
}

static_assert(sizeof(sk_lifecycle_t) % 64 == 0,
	"sk_lifecycle_t must be multiple of a cacheline");

bool
sk_lifecycle_init(sk_lifecycle_t *lfc, sk_error_t *error)
{
	memset(lfc, 0, sizeof(*lfc));

	if ((lfc->listeners = calloc(1, sizeof(sk_listeners_t))) == NULL)
		return sk_error_msg_code(
			error, "calloc listeners failed", SK_LIFECYCLE_ENOMEM);

	if (!sk_listeners_init(lfc->listeners, error)) {
		sk_listeners_destroy(lfc->listeners);
		return false;
	}
	pthread_mutex_init(&lfc->lock, NULL);

	time_t now = time(NULL);
	if (now == -1)
		return sk_error_msg_code(error, "time(2) failed", SK_LIFECYCLE_EFAULT);

	static_assert(SK_STATE_NEW == 0, "implicitely set with memset");
	ck_pr_store_64((uint64_t *)&lfc->epochs[SK_STATE_NEW], (uint64_t)now);

	return true;
}

void
sk_lifecycle_destroy(sk_lifecycle_t *lfc)
{
	pthread_mutex_lock(&lfc->lock);
	sk_listeners_destroy(lfc->listeners);
	pthread_mutex_unlock(&lfc->lock);
	pthread_mutex_destroy(&lfc->lock);

	free(lfc);
}

static bool
valid_transition(enum sk_state old, enum sk_state new);

static_assert(sizeof(time_t) == sizeof(uint64_t),
	"required due to the usage of ck_pr_store_64");

bool
sk_lifecycle_set_at_epoch(
	sk_lifecycle_t *lfc, enum sk_state new_state, time_t epoch, sk_error_t *err)
{
	if (epoch <= 0)
		return sk_error_msg_code(
			err, "epoch lower than 0", SK_LIFECYCLE_EINVAL);

	pthread_mutex_lock(&lfc->lock);

	const enum sk_state current_state = sk_lifecycle_get(lfc);
	if (!valid_transition(current_state, new_state)) {
		pthread_mutex_unlock(&lfc->lock);
		return sk_error_msg_code(
			err, "state machine advanced", SK_LIFECYCLE_EINVAL);
	}

	/*
	 * Since lifecycle_get verify the state first, we commit the timestamp
	 * before the state for linearizability
	 */
	ck_pr_store_64((uint64_t *)&lfc->epochs[new_state], (uint64_t)epoch);
	ck_pr_fence_store();
	ck_pr_store_int((int *)&lfc->state, new_state);

	pthread_mutex_unlock(&lfc->lock);

	sk_lifecycle_listener_ctx_t ctx = {new_state, epoch};
	return sk_listeners_observe(lfc->listeners, &ctx, err);
}

bool
sk_lifecycle_set(sk_lifecycle_t *lfc, enum sk_state new_state, sk_error_t *err)
{
	time_t now = time(NULL);
	if (now == -1)
		return sk_error_msg_code(err, "time(2) failed", SK_LIFECYCLE_EFAULT);

	return sk_lifecycle_set_at_epoch(lfc, new_state, now, err);
}

static_assert(sizeof(enum sk_state) == sizeof(int),
	"sizeof(enum sk_state) must sizeof(int)");

enum sk_state
sk_lifecycle_get(const sk_lifecycle_t *lfc)
{
	return ck_pr_load_int((int *)&lfc->state);
}

time_t
sk_lifecycle_get_epoch(const sk_lifecycle_t *lfc, enum sk_state state)
{
	const enum sk_state current_state = sk_lifecycle_get(lfc);
	if (state > current_state)
		return 0;

	return ck_pr_load_64((uint64_t *)&lfc->epochs[state]);
}

static inline bool
valid_transition(enum sk_state from, enum sk_state to)
{
	switch (to) {
	case SK_STATE_STARTING:
		return (from == SK_STATE_NEW);
	case SK_STATE_RUNNING:
		return (from == SK_STATE_STARTING);
	case SK_STATE_STOPPING:
		return (from == SK_STATE_RUNNING);
	case SK_STATE_TERMINATED:
		return (from == SK_STATE_STOPPING);
	case SK_STATE_FAILED:
		return from != SK_STATE_FAILED;
	default:
		return false;
	}
}

sk_listener_t *
sk_lifecycle_register_listener(sk_lifecycle_t *lfc, const char *name,
	sk_listener_cb_t callback, void *ctx, sk_error_t *error)
{
	return sk_listeners_register(lfc->listeners, name, callback, ctx, error);
}

void
sk_lifecycle_unregister_listener(sk_lifecycle_t *lfc, sk_listener_t *listener)
{
	sk_listeners_unregister(lfc->listeners, listener);
}
