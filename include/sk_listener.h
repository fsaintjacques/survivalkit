#pragma once

#include <ck_queue.h>
#include <ck_rwlock.h>

#include <sk_cc.h>
#include <sk_error.h>

/*
 * A listener is a callback notified on important events.
 *
 * User implements a listener by mean of a closure. The closure is called with
 * the following parameters:
 *
 * @param user_ctx, user defined context, this context exists such that listener
 *                  may hold state across events call.
 * @param event_ctx, event defined context, the structure of this context will
 *                   be found in the facility providing the listener interface
 * @param error, error to store failure information
 *
 * @return true on success, false on failure and set error.
 *
 * The callback _must_ be thread-safe as there's no guarantee where and when the
 * closure is run. The callback will likely run in a different thread than
 * where the context was initialized (or modified). A good rule is to ensure
 * that the callback only read atomically in the context.
 *
 * An fictitious example follows:
 *
 * struct http_ctx;
 *
 * // Callback that notifies an http endpoint on lifecycle transition
 * bool sk_lifecycle_http_ping(void* ctx, void *event_ctx, sk_error_t *err) {
*    struct http_ctx *http_ctx = ctx;
 *    sk_lifecycle_listener_ctx_t *listener_ctx = event_ctx;
 *
 *    if (!http_client_post(http_ctx->url, listener_ctx->state))
 *        return sk_error_msg(error, "failed to ping url");
 *
 *    return true;
 * }
 *
 */
typedef bool (*sk_listener_cb_t)(
	void *user_ctx, void *event_ctx, sk_error_t *error);

struct sk_listener {
	/* Name of the listener */
	char *name;

	/* User defined callback */
	sk_listener_cb_t callback;
	/* User defined context */
	void *ctx;

	/* Next listener in to call */
	CK_SLIST_ENTRY(sk_listener) next;
};
typedef struct sk_listener sk_listener_t;

/* A set of listeners */
struct sk_listeners {
	/* Lock to protect write access to list */
	ck_rwlock_t lock;

	/* A list of listeners to notify */
	CK_SLIST_HEAD(, sk_listener) listeners;
};
typedef struct sk_listeners sk_listeners_t;

/* Errors returned by the lifecycle APIs */
enum sk_listener_errno {
	SK_LISTENER_OK = 0,
	/* No enough space */
	SK_LISTENER_ENOMEM = ENOMEM,
};

/*
 * Initialize a set of listeners.
 *
 * @param listeners, listeners to initilize
 * @param error, error to store failure information
 *
 * @return true on success, false otherwise and set error
 */
bool
sk_listeners_init(sk_listeners_t *listeners, sk_error_t *error)
	sk_nonnull(1, 2);

/*
 * Free a set of listeners
 *
 * @param listeners, listeners to free
 */
void
sk_listeners_destroy(sk_listeners_t *listeners) sk_nonnull(1);

/*
 * Register a listener.
 *
 * @param listeners, listeners to register a listener to
 * @param name, name of the listener to register
 * @param callback, callback that will be called upon trigger
 * @param user_ctx, user registered context, ownership is transferred
 * @param error, error to store failure information
 *
 * @return a pointer to the newly added listener, or NULL otherwise and set
 *         error.
 *
 * Note that the listener returned by this function shall be freed by calling
 * `sk_listeners_unregister`. Since ownership of the context is transferred, it
 * will be freed upon failure to register a listener.
 */
sk_listener_t *
sk_listeners_register(sk_listeners_t *listeners, const char *name,
	sk_listener_cb_t callback, void *user_ctx, sk_error_t *error)
	sk_nonnull(1, 2, 3, 5);

/*
 * Unregister and free a listener.
 *
 * @param listeners, listeners to remove the listener from
 * @param listener, listener to unregister (and free)
 */
void
sk_listeners_unregister(sk_listeners_t *listeners, sk_listener_t *listener)
	sk_nonnull(1, 2);

/*
 * Observe an event and call registered listeners.
 *
 * @param listeners, listeners to observe event on
 * @param event_ctx, event to observe
 * @param error, error to store the first failure information
 *
 * @return true if all listeners succeed without error, or false if at least
 *         one listener fail.
 */
bool
sk_listeners_observe(sk_listeners_t *listeners, void *event_ctx,
	sk_error_t *error) sk_nonnull(1, 3);
