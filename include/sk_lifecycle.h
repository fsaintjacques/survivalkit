#pragma once

#include <stdbool.h>
#include <time.h>

#include <ck_rwlock.h>

#include <sk_cc.h>
#include <sk_error.h>
#include <sk_listener.h>

/*
 * Lifecycle is a thread safe state machine representing the operational state
 * of a component. A lifecycle might serve many purposes:
 *
 * - Improve auditing via event logs
 * - Automatically toggles healthcheck in STARTING and STOPPING transitions
 * - Centralize the exit condition of a main loop
 *
 * The possibles transitions are given by the following state machine:
 *
 *   NEW → STARTING → RUNNING → STOPPING → TERMINATED
 *    └───────┴──────────┴─────────┴─────→ FAILED
 */

enum sk_state {
	/*
	 * A component in this state is inactive. It does minimal work and consumes
	 * minimal resources.
	 */
	SK_STATE_NEW = 0,
	/* A component in this state is transitioning to SK_STATE_RUNNING. */
	SK_STATE_STARTING,
	/* A service in this state is operational. */
	SK_STATE_RUNNING,
	/* A service in this state is transitioning to SK_STATE_TERMINATED. */
	SK_STATE_STOPPING,
	/*
	 * A service in this state has completed execution normally. It does
	 * minimal work and consumes minimal resources.
	 */
	SK_STATE_TERMINATED,
	/*
	 * A service in this state has encountered a problem and may not be
	 * operational. It cannot be started nor stopped.
	 */
	SK_STATE_FAILED,

	/* Do not use, leave at the end */
	SK_STATE_COUNT,
};

/*
 * String representation of a state.
 *
 * @param state, state for which the string representation is requested
 *
 * @return pointer to const string representation, NULL on error
 */
const char *
sk_state_str(enum sk_state state);

/* State transition callback */
typedef void (*sk_lifecycle_listener_cb_t)(
	void *ctx, enum sk_state state, time_t epoch);

SK_LISTENER(sk_lifecycle, sk_lifecycle_listener_cb_t);

struct sk_lifecycle {
	/* The current state */
	enum sk_state state;
	ck_rwlock_t lock;
	CK_SLIST_HEAD(, sk_lifecycle_listener) listeners;
	/* Epochs at which state were transitioned to */
	time_t epochs[SK_STATE_COUNT];
} sk_cache_aligned;
typedef struct sk_lifecycle sk_lifecycle_t;

/* Errors returned by the lifecycle APIs */
enum sk_lifecycle_errno {
	SK_LIFECYCLE_OK = 0,
	/* No enough space */
	SK_LIFECYCLE_ENOMEM = ENOMEM,
	/* Invalid argument */
	SK_LIFECYCLE_EINVAL = EINVAL,
	/* Call to time(2) failed */
	SK_LIFECYCLE_EFAULT = EFAULT,
};

/*
 * Initialize a `sk_lifecycle_t`.
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

/*
 * Get the current state of a `sk_lifecycle_t`.
 *
 * @param lfc, lifecycle to return the state from
 *
 * @return state
 */
enum sk_state
sk_lifecycle_get(const sk_lifecycle_t *lfc) sk_nonnull(1);

/*
 * Get the epoch at which the lifecycle transition to a given state.
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

/*
 * Transition the state of a `sk_lifecycle_t`.
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

/*
 * Transition the state of a `sk_lifecycle_t` at a given epoch.
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
 *                              invalid
 */
bool
sk_lifecycle_set_at_epoch(sk_lifecycle_t *lfc, enum sk_state new_state,
	time_t epoch, sk_error_t *error) sk_nonnull(1, 4);

/*
 * Register a listener.
 *
 * @param lfc, lifecycle to register a listener to
 * @param name, name of the listener
 * @param callback, callback to invoke on transitions
 * @param ctx, context to pass to callback when invoked
 * @param error, error to store failure information
 *
 * @return pointer to listener on success , NULL on failure and set error
 *
 * @errors SK_HEALTHCHECK_ENOMEN, if memory allocation failed
 */
sk_lifecycle_listener_t *
sk_lifecycle_register_listener(sk_lifecycle_t *lfc, const char *name,
	sk_lifecycle_listener_cb_t callback, void *ctx, sk_error_t *error)
	sk_nonnull(1, 2, 4);

/*
 * Unregister a listener.
 *
 * @param lfc, lifecycle to unregister the listener from
 * @param listener, listener to unregister
 */
void
sk_lifecycle_unregister_listener(
	sk_lifecycle_t *lfc, sk_lifecycle_listener_t *listener) sk_nonnull(1, 2);
