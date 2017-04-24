#pragma once

#include <ck_pr.h>

#include <sk_flag.h>
#include <sk_healthcheck.h>

static inline bool
sk_healthcheck_enabled(const sk_healthcheck_t *hc)
{
	if (hc == NULL)
		return false;

	return sk_flag_get(&hc->flags, SK_HEALTHCHECK_ENABLED);
}
