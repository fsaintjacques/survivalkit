#include <stdlib.h>
#include <string.h>

#include <ck_queue.h>
#include <ck_rwlock.h>

#include <sk_listener.h>

bool
sk_listeners_init(sk_listeners_t *lts, sk_error_t *error)
{
	(void) error;

	ck_rwlock_init(&lts->lock);
	CK_SLIST_INIT(&lts->listeners);

	return true;
}

static void
__sk_list_free(sk_listener_t *listener)
{
	free(listener->name);
	free(listener->ctx);
	free(listener);
}

void
sk_listeners_destroy(sk_listeners_t *lts)
{
	ck_rwlock_write_lock(&lts->lock);
	while (!CK_SLIST_EMPTY(&lts->listeners)) {
		sk_listener_t *listener = CK_SLIST_FIRST(&lts->listeners);
		CK_SLIST_REMOVE_HEAD(&lts->listeners, next);
		__sk_list_free(listener);
	}
	ck_rwlock_write_unlock(&lts->lock);

	free(lts);
}

sk_listener_t *
sk_listeners_register(sk_listeners_t *lts, const char *name,
	sk_listener_cb_t callback, void *ctx, sk_error_t *error)
{

	sk_listener_t *listener;
	if ((listener = calloc(1, sizeof(*listener))) == NULL) {
		sk_error_msg_code(error, "listener calloc failed", SK_LISTENER_ENOMEM);
		free(ctx);
		return NULL;
	}

	if ((listener->name = strdup(name)) == NULL) {
		sk_error_msg_code(error, "name strdup failed", SK_LISTENER_ENOMEM);
		goto name_alloc_failed;
	}

	listener->callback = callback;
	listener->ctx = ctx;

	ck_rwlock_write_lock(&lts->lock);
	CK_SLIST_INSERT_HEAD(&lts->listeners, listener, next);
	ck_rwlock_write_unlock(&lts->lock);

	return listener;

name_alloc_failed:
	free(ctx);
	free(listener);
	return NULL;
}

void
sk_listeners_unregister(sk_listeners_t *lts, sk_listener_t *listener)
{
	ck_rwlock_write_lock(&lts->lock);
	CK_SLIST_REMOVE(&lts->listeners, listener, sk_listener, next);
	__sk_list_free(listener);
	ck_rwlock_write_unlock(&lts->lock);
}

bool
sk_listeners_observe(sk_listeners_t *lts, void *ctx, sk_error_t *error)
{
	sk_listener_t *listener;

	ck_rwlock_read_lock(&lts->lock);
	CK_SLIST_FOREACH(listener, &lts->listeners, next)
	{
		if (!listener->callback(listener->ctx, ctx, error)) {
			ck_rwlock_read_unlock(&lts->lock);
			return false;
		}
	}
	ck_rwlock_read_unlock(&lts->lock);

	return true;
}
