#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <sk_cc.h>
#include <sk_error.h>
#include <sk_flag.h>

/* The state of a healthcheck. */
enum sk_health {
	/* The check is in an unknown state, probably due to an internal error. */
	SK_HEALTH_UNKNOWN = 0,
	/* The check is healthy. */
	SK_HEALTH_OK,
	/* The check is approaching unhealthy level; action should be taken. */
	SK_HEALTH_WARNING,
	/* The check is unhealthy; action must be taken immediately. */
	SK_HEALTH_CRITICAL,

	/* Do not use, leave at the end */
	SK_HEALTH_COUNT,
};

/* String representation of a health status.
 *
 * @param health, health for which the string representation is requested
 *
 * @return pointer to const string representation, NULL on error
 */
const char *
sk_health_str(enum sk_health health);

/*
 * Healthcheck callback
 *
 * Users implements health checks by mean of a closure. The closure returns
 * the state and optionally provides an error code/message.
 *
 * An fictitious example follows:
 *
 * struct db_ctx;
 *
 * enum sk_health db_health(const void* opaque, sk_error_t *err) {
 *    if (opaque == NULL)
 *      return SK_HEALTH_UNKNOWN;
 *
 *    struct db_ctx *ctx = (struct db_ctx*)opaque;
 *
 *    if (!db_is_connected(ctx)) {
 *        sk_error_msg_code(err, DB_NOT_CONNECTED, "Not connected to db");
 *        return SK_HEALT_CRITICAL;
 *    }
 *
 *    const float usage = db_connection_usage(ctx);
 *    if (usage > 0.85) {
 *        sk_error_msg_code(err, DB_CONNECTION_POOL, "Pool exhausted");
 *        return (usage > 0.95) ? SK_HEALTH_CRITICAL :
 *                                SK_HEALTH_WARN;
 *    }
 *
 *    return SK_HEALTH_OK;
 * }
 */
typedef enum sk_health (*sk_healthcheck_cb_t)(
    const void *opaque, sk_error_t *error);

/* Flags */
enum {
	SK_HEALTHCHECK_ENABLED = 1,
};

struct sk_healthcheck {
	/* The name of the healthcheck. */
	char *name;
	/* A brief description of the healthcheck. */
	char *description;

	sk_flag_t flags;

	/* User provided callback that implements the healthcheck. */
	sk_healthcheck_cb_t callback;
	void *opaque;
};
typedef struct sk_healthcheck sk_healthcheck_t;

/* Errors returned by the healthcheck APIs */
enum sk_healthcheck_errno {
	SK_HEALTHCHECK_OK = 0,
	/* No enough space */
	SK_HEALTHCHECK_ENOMEM = ENOMEM,
	/* Healthcheck disabled */
	SK_HEALTHCHECK_EAGAIN = EAGAIN,
};

/*
 * Initialize a healthcheck.
 *
 * @param healthcheck, healthcheck to initialize
 * @param name, name of the healthcheck
 * @param description, short description of the healthcheck
 * @param flags, flags to initialize the check with
 * @param callback, callback to invoke on polling, see description of type
 *                  sk_healthcheck_cb_t above for more information
 * @param opaque, opaque structure to pass to callback
 * @param error, error to store failure information
 *
 * @return true on success, false otherwise and set error
 *
 * @errors SK_HEALTHCHECK_ENOMEN, if memory allocation failed
 */
bool
sk_healthcheck_init(sk_healthcheck_t *healthcheck, const char *name,
    const char *description, sk_flag_t flags, sk_healthcheck_cb_t callback,
    void *opaque, sk_error_t *error) sk_nonnull(1, 2, 5, 6);

/*
 * Free a healthcheck.
 *
 * @param healthcheck, healthcheck to free
 */
void
sk_healthcheck_destroy(sk_healthcheck_t *healthcheck) sk_nonnull(1);

/*
 * Poll a healthcheck for status.
 *
 * @param healthcheck, healthcheck to poll
 * @param state, health state of the check
 * @param error, error to store failure information
 *
 * @return true on success, false otherwise and set error
 *
 * @errors SK_HEALTHCHECK_EAGAIN, if healthcheck is disabled
 *
 * Note that the callback can also set an error, thus depending on the health
 * state, one might also check the error.
 */
bool
sk_healthcheck_poll(const sk_healthcheck_t *healthcheck, enum sk_health *state,
    sk_error_t *error) sk_nonnull(1, 2, 3);

#define sk_healthcheck_enable(hc)                                              \
	sk_flag((&(hc)->flags), SK_HEALTHCHECK_ENABLED)

#define sk_healthcheck_disable(hc)                                             \
	sk_flag_unset((&(hc)->flags), SK_HEALTHCHECK_ENABLED)
