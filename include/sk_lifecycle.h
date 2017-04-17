#pragma once

#include <stdbool.h>
#include <time.h>

/* A state describe the application's lifecycle.
 *
 * The possibles transitions are given by the following state machine:
 *
 *   NEW → STARTING → RUNNING → STOPPING → TERMINATED
 *    └───────┴──────────┴─────────┴─────→ FAILED
 */
enum sk_state {
	SK_STATE_NEW = 0,
	SK_STATE_STARTING = 1,
	SK_STATE_RUNNING = 2,
	SK_STATE_STOPPING = 3,
	SK_STATE_TERMINATED = 4,
	SK_STATE_FAILED = 5,
};

#define SK_LIFECYCLE_MAX SK_STATE_FAILED
#define SK_LIFECYCLE_COUNT (SK_LIFECYCLE_MAX + 1)

/* String representation of a state.
 *
 * @param state
 *
 * @return pointer to string representation, NULL if error.
 */
const char *
sk_state_str(enum sk_state state);

struct sk_lifecycle {
	/* The current state */
	enum sk_state state;
	char __padding[12];
	/* Epochs at which state were transitionned to */
	time_t epochs[SK_LIFECYCLE_COUNT];
};
typedef struct sk_lifecycle sk_lifecycle_t;

/* Initialize a `sk_lifecycle_t`.
 *
 * @param lfc, lifecycle to initialize
 *
 * @return true on success, false otherwise
 */
bool
sk_lifecycle_init(sk_lifecycle_t *lfc);

/* Get the current state of a `sk_lifecycle_t`.
 *
 * @param lfc, lifecycle to return the state from
 *
 * @return the state
 */
enum sk_state
sk_lifecycle_get(const sk_lifecycle_t *lfc);

/* Get the epoch at which the lifecycle transition to a given state.
 *
 * @param lfc, lifecycle to query from.
 * @param state,
 *
 * @return -1 on failure, 0 if the given state was never transitionned to or
 *         the transition time.
 *
 * This method is used to view the history of when transitions happened.
 */
time_t
sk_lifecycle_get_epoch(const sk_lifecycle_t *lfc, enum sk_state state);

/* Set the state of a `sk_lifecycle_t`.
 *
 * @param lfc,
 * @param new_state,
 *
 * @return true on success, false otherwise
 */
bool
sk_lifecycle_set(sk_lifecycle_t *lfc, enum sk_state new_state);

/* Set the state of a `sk_lifecycle_t` at a given epoch.
 *
 * @param lfc,
 * @param new_state,
 * @param epoch
 *
 * @return true on success, false otherwise
 *
 * This method exists mostly for testing purposes.
 */
bool
sk_lifecycle_set_at_epoch(
    sk_lifecycle_t *lfc, enum sk_state new_state, time_t epoch);
