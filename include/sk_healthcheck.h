#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <sk_err.h>

/* The state of a healthcheck. */
enum sk_health_state {
	/* The check is in an unknown state, probably due to an internal error. */
	SK_HEALTH_STATE_UNKNOWN = 0,
	/* The check is healthy. */
	SK_HEALTH_STATE_OK = 1,
	/* The check is approaching unhealthy level; action should be taken. */
	SK_HEALTH_STATE_WARN = 2,
	/* The check is unhealthy; action must be taken immediately. */
	SK_HEALTH_STATE_CRITICAL = 3,
};

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
 * enum sk_health_state db_health(const void* opaque, sk_err_t *err) {
 *    if (ctx == NULL)
 *      return SK_HEALTH_STATE_UNKNOWN;
 *
 *    struct db_ctx *ctx = (struct db_ctx*)opaque;
 *
 *    if (!db_is_connected(ctx)) {
 *        err->code = DB_NOT_CONNECTED;
 *        sk_buf_sprintf(err->message, "Not connected to database %s",
 *                       ctx->host);
 *        return SK_HEALT_STATE_CRITICAL;
 *    }
 *
 *    const float usage = db_connection_usage(ctx);
 *    if (usage > 0.85) {
 *        err->code = DB_CONNECTION_POOL;
 *        sk_buf_sprintf(err->message, "Connection usage at %f", usage);
 *        return (usage > 0.95) ? SK_HEALTH_STATE_CRITICAL :
 *                                SK_HEALTH_STATE_WARN;
 *    }
 *
 *    return SK_HEALTH_STATE_OK;
 * }
 */
typedef enum sk_health_state (*sk_healthcheck_cb_t)(
    const void *opaque, sk_err_t *err);

/* Flags */
enum {
	SK_HEALTHCHECK_ENABLED = 1,
};

struct sk_healthcheck {
	/* The name of the healthcheck. */
	char *name;
	/* A brief description of the healthcheck. */
	char *description;

	uint64_t flags;

	/* User provided callback that implements the healthcheck. */
	sk_healthcheck_cb_t callback;
	void *opaque;
};
typedef struct sk_healthcheck sk_healthcheck_t;

/*
 * Initialize a healthcheck.
 */
bool
sk_healthcheck_init(sk_healthcheck_t *hc, const char *name,
    const char *description, int flags, sk_healthcheck_cb_t callback,
    void *opaque);

bool
sk_healthcheck_destroy(sk_healthcheck_t *hc);

bool
sk_healthcheck_poll(
    const sk_healthcheck_t *hc, enum sk_health_state *result, sk_err_t *err);

bool
sk_healthcheck_set(sk_healthcheck_t *hc, int flags);

bool
sk_healthcheck_unset(sk_healthcheck_t *hc, int flags);

#define sk_healthcheck_enable(hc)                                              \
	sk_healthcheck_set((hc), SK_HEALTHCHECK_ENABLED)

#define sk_healthcheck_disable(hc)                                             \
	sk_healthcheck_unset((hc), SK_HEALTHCHECK_ENABLED)
