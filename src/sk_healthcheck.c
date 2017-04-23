#include <stdlib.h>
#include <string.h>

#include <sk_healthcheck.h>

#include "sk_healthcheck_priv.h"

bool
sk_healthcheck_init(sk_healthcheck_t *hc, const char *name,
    const char *description, sk_flag_t flags, sk_healthcheck_cb_t callback,
    void *opaque)
{
	if (hc == NULL || callback == NULL)
		return false;

	memset(hc, 0, sizeof(*hc));

	if ((hc->name = strdup(name)) == NULL)
		goto fail_name_alloc;
	if ((hc->description = strdup(description)) == NULL)
		goto fail_desc_alloc;
	hc->callback = callback;
	hc->opaque = opaque;

	sk_flag_set(&hc->flags, flags | SK_HEALTHCHECK_ENABLED);

	return true;

	free(hc->description);
fail_desc_alloc:
	free(hc->name);
fail_name_alloc:
	return false;
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
sk_healthcheck_poll(
    const sk_healthcheck_t *hc, enum sk_health_state *result, sk_error_t *err)
{
	if (!sk_healthcheck_valid(hc) || !sk_healthcheck_enabled(hc))
		return false;

	*result = hc->callback(hc->opaque, err);

	return true;
}
