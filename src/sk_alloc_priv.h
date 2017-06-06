#pragma once

static inline void *
sk_malloc(size_t r)
{

	return malloc(r);
}

static inline void
sk_free(void *p, size_t b, bool r)
{

	(void)b;
	(void)r;
	free(p);
	return;
}
