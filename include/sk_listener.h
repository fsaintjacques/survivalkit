#pragma once

#include <ck_queue.h>

#include <sk_error.h>

#define SK_LISTENER(prefix, callback_type)                                     \
	struct prefix##_listener {                                                 \
		char *name;                                                            \
		callback_type callback;                                                \
		void *ctx;                                                             \
                                                                               \
		CK_SLIST_ENTRY(prefix##_listener) next;                                \
	};                                                                         \
	typedef struct prefix##_listener prefix##_listener_t

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

bool
sk_listener_init(sk_listeners_t *list);

sk_listener_t *
sk_listener_register(sk_listeners_t *lts, const char *name,
	sk_listener_cb_t callback, void *ctx, sk_error_t *error);

void
sk_listener_unregister(sk_listeners_t *lts, sk_listener_t *listener);

bool
sk_listener_call(sk_listeners_t *lts, void *ctx, sk_error_t *error);
