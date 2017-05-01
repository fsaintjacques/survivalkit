#include <stdlib.h>
#include <string.h>

#include <sk_healthcheck.h>

#include "sk_healthcheck_priv.h"

// clang-format off
static const char *health_labels[] = {
	[SK_HEALTH_UNKNOWN] = "unknown",
	[SK_HEALTH_OK] = "ok",
	[SK_HEALTH_WARNING] = "warning",
	[SK_HEALTH_CRITICAL] = "critical",
};
// clang-format on

const char *
sk_health_str(enum sk_health health)
{
	return (health < SK_HEALTH_COUNT) ? health_labels[health] : NULL;
}

bool
sk_healthcheck_init(sk_healthcheck_t *hc, const char *name,
    const char *description, sk_flag_t flags, sk_healthcheck_cb_t callback,
    void *opaque, sk_error_t *error)
{
	memset(hc, 0, sizeof(*hc));

	if ((hc->name = strdup(name)) == NULL) {
		sk_error_msg_code(error, "name strdup failed", SK_HEALTHCHECK_ENOMEM);
		goto fail_name_alloc;
	}
	if ((hc->description = strdup(description)) == NULL) {
		sk_error_msg_code(error, "desc strdup failed", SK_HEALTHCHECK_ENOMEM);
		goto fail_desc_alloc;
	}
	hc->callback = callback;
	hc->opaque = opaque;
	hc->flags = flags;

	ck_rwlock_init(&hc->lock);

	sk_healthcheck_enable(hc);

	return true;

	free(hc->description);
fail_desc_alloc:
	free(hc->name);
fail_name_alloc:
	return false;
}

void
sk_healthcheck_destroy(sk_healthcheck_t *hc)
{
	ck_rwlock_write_lock(&hc->lock);

	sk_healthcheck_disable(hc);

	free(hc->name);
	free(hc->description);
	free(hc->opaque);

	ck_rwlock_write_unlock(&hc->lock);

	free(hc);
}

bool
sk_healthcheck_poll(
    sk_healthcheck_t *hc, enum sk_health *result, sk_error_t *err)
{
	if (ck_rwlock_read_trylock(&hc->lock)) {
		if (!sk_healthcheck_enabled(hc)) {
			ck_rwlock_read_unlock(&hc->lock);
			return sk_error_msg_code(
			    err, "healthcheck disabled", SK_HEALTHCHECK_EAGAIN);
		}

		*result = hc->callback(hc->opaque, err);

		ck_rwlock_read_unlock(&hc->lock);

		return true;
	}

	return sk_error_msg_code(
	    err, "healthcheck lock failed", SK_HEALTHCHECK_EAGAIN);
}
