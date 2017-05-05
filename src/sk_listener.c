#include <stdlib.h>
#include <string.h>

#include <ck_queue.h>
#include <ck_rwlock.h>

#include <sk_listener.h>

bool
sk_listener_init(sk_listeners_t *lts)
{
	CK_SLIST_INIT(&lts->listeners);

	return true;
}

sk_listener_t *
sk_listener_register(sk_listeners_t *lts, const char *name,
	sk_listener_cb_t callback, void *ctx, sk_error_t *error)
{

	sk_listener_t *listener;
	if ((listener = calloc(1, sizeof(*listener))) == NULL) {
		sk_error_msg_code(error, "listener calloc failed", 1);
		return NULL;
	}

	if ((listener->name = strdup(name)) == NULL) {
		sk_error_msg_code(error, "name strdup failed", 1);
		goto name_alloc_failed;
	}

	listener->callback = callback;
	listener->ctx = ctx;

	ck_rwlock_write_lock(&lts->lock);
	CK_SLIST_INSERT_HEAD(&lts->listeners, listener, next);
	ck_rwlock_write_unlock(&lts->lock);

	return listener;

name_alloc_failed:
	free(listener);
	return NULL;
}

void
sk_listener_unregister(sk_listeners_t *lts, sk_listener_t *listener)
{
	ck_rwlock_write_lock(&lts->lock);
	CK_SLIST_REMOVE(&lts->listeners, listener, sk_listener, next);
	ck_rwlock_write_unlock(&lts->lock);
}

bool
sk_listener_call(sk_listeners_t *lts, void *ctx, sk_error_t *error)
{
	sk_listener_t *listener;
	CK_SLIST_FOREACH(listener, &lts->listeners, next)
	{
		if (!listener->callback(listener->ctx, ctx, error))
			return false;
	}

	return true;
}
