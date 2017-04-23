#pragma once

#include <stdbool.h>
#include <time.h>

#include <sk_cc.h>
#include <sk_error.h>

/* A state describe the application's lifecycle.
 *
 * The possibles transitions are given by the following state machine:
 *
 *   NEW → STARTING → RUNNING → STOPPING → TERMINATED
 *    └───────┴──────────┴─────────┴─────→ FAILED
 */
enum sk_state {
	SK_STATE_NEW = 0,
	SK_STATE_STARTING,
	SK_STATE_RUNNING,
	SK_STATE_STOPPING,
	SK_STATE_TERMINATED,
	SK_STATE_FAILED,

	/* Do not use, leave at the end */
	SK_STATE_COUNT,
};

/* String representation of a state.
 *
 * @param state, state for which the string representation is requested
 *
 * @return pointer to const string representation, NULL on error
 */
const char *
sk_state_str(enum sk_state state);

struct sk_lifecycle {
	/* The current state */
	enum sk_state state;
	/* Epochs at which state were transitioned to */
	time_t epochs[SK_STATE_COUNT];
} sk_cache_aligned;
typedef struct sk_lifecycle sk_lifecycle_t;

/* Errors returned by the lifecycle APIs */
enum sk_lifecycle_errno {
	SK_LIFECYCLE_OK = 0,
	/* Invalid argument */
	SK_LIFECYCLE_EINVAL = EINVAL,
	/* Call to time(2) failed */
	SK_LIFECYCLE_EFAULT = EFAULT,
};

/* Initialize a `sk_lifecycle_t`.
 *
 * @param lfc, lifecycle to initialize
 * @param error, error to store failure information
 *
 * @return true on success, false otherwise and set error
 *
 * @errors SK_LIFECYCLE_EFAULT, if call to time(2) failed.
 */
bool
sk_lifecycle_init(sk_lifecycle_t *lfc, sk_error_t *error) sk_nonnull(1, 2);

/* Get the current state of a `sk_lifecycle_t`.
 *
 * @param lfc, lifecycle to return the state from
 *
 * @return state
 */
enum sk_state
sk_lifecycle_get(const sk_lifecycle_t *lfc) sk_nonnull(1);

/* Get the epoch at which the lifecycle transition to a given state.
 *
 * @param lfc, lifecycle to query
 * @param state, state to ask epoch for
 *
 * @return -1 on failure,
 *          0 if the state is not yet transitioned to
 *         or the transition time
 *
 * This method is used to view the history of when transitions happened.
 */
time_t
sk_lifecycle_get_epoch(const sk_lifecycle_t *lfc, enum sk_state state)
    sk_nonnull(1);

/* Transition the state of a `sk_lifecycle_t`.
 *
 * @param lfc, lifecycle to affect
 * @param new_state, state to transition to
 * @param error, error to store failure information
 *
 * @return true on success, false otherwise and set error
 *
 * @errors SK_LIFECYCLE_EFAULT, if call to time(2) failed
 *         SK_LIFECYCLE_EINVAL, if state transition is invalid
 */
bool
sk_lifecycle_set(sk_lifecycle_t *lfc, enum sk_state new_state,
    sk_error_t *error) sk_nonnull(1, 3);

/* Transition the state of a `sk_lifecycle_t` at a given epoch.
 *
 * @param lfc, lifecycle to affect
 * @param new_state, state to transition to
 * @param epoch, time at which the transition occurred
 * @param error, error to store failure information
 *
 * @return true on success, false otherwise and set error
 *
 * @errors SK_LIFECYCLE_EFAULT, if call to time(2) failed
 *         SK_LIFECYCLE_EINVAL, if state transition is invalid or if epoch is
 *                              invalid.
 */
bool
sk_lifecycle_set_at_epoch(sk_lifecycle_t *lfc, enum sk_state new_state,
    time_t epoch, sk_error_t *error) sk_nonnull(1, 4);
