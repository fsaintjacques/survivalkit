#pragma once

#include <ck_queue.h>
#include <ck_rwlock.h>

#include <sk_error.h>

typedef bool (*sk_listener_cb_t)(
	void *ctx, void *facility_ctx, sk_error_t *error);

struct sk_listener {
	char *name;

	sk_listener_cb_t callback;
	void *ctx;

	CK_SLIST_ENTRY(sk_listener) next;
};
typedef struct sk_listener sk_listener_t;

struct sk_listeners {
	ck_rwlock_t lock;

	CK_SLIST_HEAD(, sk_listener) listeners;
};
typedef struct sk_listeners sk_listeners_t;

/* Errors returned by the lifecycle APIs */
enum sk_listener_errno {
	SK_LISTENER_OK = 0,
	/* No enough space */
	SK_LISTENER_ENOMEM = ENOMEM,
};

bool
sk_listener_init(sk_listeners_t *list);

void
sk_listener_destroy(sk_listeners_t *list);

sk_listener_t *
sk_listener_register(sk_listeners_t *lts, const char *name,
	sk_listener_cb_t callback, void *ctx, sk_error_t *error);

void
sk_listener_unregister(sk_listeners_t *lts, sk_listener_t *listener);

bool
sk_listener_observe(sk_listeners_t *lts, void *ctx, sk_error_t *error);
