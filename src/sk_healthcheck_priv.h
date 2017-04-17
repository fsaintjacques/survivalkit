#pragma once

#include <ck_pr.h>

#include "sk_healthcheck.h"

static inline bool
sk_healthcheck_enabled(const sk_healthcheck_t *hc)
{
	if (hc == NULL)
		return false;

	return ck_pr_load_64(&hc->flags) & SK_HEALTHCHECK_ENABLED;
}

static inline bool
sk_healthcheck_valid(const sk_healthcheck_t *hc)
{
	if (hc == NULL || hc->callback == NULL)
		return false;

	return true;
}
