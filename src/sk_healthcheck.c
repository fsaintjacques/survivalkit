#include <stdlib.h>
#include <string.h>

#include "sk_healthcheck.h"
#include "sk_healthcheck_priv.h"

bool
sk_healthcheck_init(sk_healthcheck_t *hc, const char *name,
    const char *description, int flags, sk_healthcheck_cb_t callback,
    void *opaque)
{
	if (hc == NULL || callback == NULL)
		return false;

	memset(hc, 0, sizeof(*hc));

	hc->name = strdup(name);
	hc->description = strdup(description);
	hc->callback = callback;
	hc->opaque = opaque;

	return sk_healthcheck_set(hc, flags | SK_HEALTHCHECK_ENABLED);
}

bool
sk_healthcheck_destroy(sk_healthcheck_t *hc)
{
	if (hc == NULL)
		return false;

	sk_healthcheck_disable(hc);

	free(hc->name);
	free(hc->description);

	return true;
}

bool
sk_healthcheck_set(sk_healthcheck_t *hc, int flags)
{
	if (!hc)
		return false;

	for (;;) {
		int old_flags = ck_pr_load_64(&hc->flags);
		if (ck_pr_cas_64(&hc->flags, old_flags, old_flags | flags))
			break;
	}

	return true;
}

bool
sk_healthcheck_unset(sk_healthcheck_t *hc, int flags)
{
	if (!hc)
		return false;

	for (;;) {
		int old_flags = ck_pr_load_64(&hc->flags);
		if (ck_pr_cas_64(&hc->flags, old_flags, old_flags & ~flags))
			break;
	}

	return true;
}

bool
sk_healthcheck_poll(
    const sk_healthcheck_t *hc, enum sk_health_state *result, sk_err_t *err)
{
	if (!sk_healthcheck_valid(hc) || !sk_healthcheck_enabled(hc))
		return false;

	*result = hc->callback(hc->opaque, err);

	return true;
}
