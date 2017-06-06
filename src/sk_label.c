#include <stdlib.h>
#include <string.h>

#include <ck_hs.h>

#include <sk_label.h>
#include <sk_label_default.h>

#include "sk_alloc_priv.h"

static struct {
	ck_hs_t set;
} registry;

unsigned long
sk_label_desc_hash(const void *key, unsigned long seed)
{
	const sk_label_descriptor_t *desc = key;

	/* FIXME: use a real hash. */
	return ((unsigned long)desc->key) ^ seed;
}

bool
sk_label_desc_cmp(const void *fst, const void *snd)
{
	const sk_label_descriptor_t *fst_d = fst;
	const sk_label_descriptor_t *snd_d = snd;

	return strcmp(fst_d->key, snd_d->key) == 0;
}

#define HASH(desc) CK_HS_HASH(&registry.set, sk_label_desc_hash, (desc))

static struct ck_malloc alloc = {.malloc = sk_malloc, .free = sk_free};

bool
sk_registry_init(sk_error_t *error)
{
	if (!ck_hs_init(&registry.set, CK_HS_MODE_OBJECT | CK_HS_MODE_SPMC,
			sk_label_desc_hash, sk_label_desc_cmp, &alloc, 1 << 8, 0x385f093)) {
		return sk_error_msg_code(
			error, "label registry initialization failed", SK_ERROR_EINVAL);
	}

	for (size_t i = 0; i < sk_array_size(sk_default_labels); i++)
		if (!sk_label_register(&sk_default_labels[i], error))
			return false;

	return true;
}

bool
sk_label_register(const sk_label_descriptor_t *descriptor, sk_error_t *error)
{
	const unsigned long hashed_key = HASH(descriptor);
	sk_label_descriptor_t *desc = calloc(1, sizeof(*desc));

	if (desc == NULL)
		return sk_error_msg_code(
			error, "descriptor allocation failed", SK_ERROR_ENOMEM);

	memcpy(desc, descriptor, sizeof(*desc));

	if (ck_hs_put(&registry.set, hashed_key, desc) == false) {
		free(desc);
		return sk_error_msg_code(
			error, "descriptor registration failed", SK_ERROR_EINVAL);
	}

	return true;
}

sk_label_descriptor_t *
sk_label_get(const char *key)
{
	sk_label_descriptor_t dummy = { .key = key };
	const unsigned long hashed_key = HASH(&dummy);

	return ck_hs_get(&registry.set, hashed_key, &dummy);
}

#define sk_label_null                                                          \
	(sk_label_t)                                                               \
	{                                                                          \
		.descriptor = NULL, .value.string_value = NULL                         \
	}

#define SK_LABEL_IMPL_GEN(SUF, TYPE, VAL, VAL_MOD)                             \
	sk_label_t sk_label_##SUF(const char *key, TYPE value)                     \
	{                                                                          \
		sk_label_descriptor_t *desc;                                           \
		if ((desc = sk_label_get(key)) == NULL)                                \
			return sk_label_null;                                              \
                                                                               \
		return (sk_label_t){desc, {.VAL##_value = VAL_MOD(value)}};            \
	}

SK_LABEL_IMPL_GEN(s, const char *, string, strdup)
SK_LABEL_IMPL_GEN(b, bool, bool, )
SK_LABEL_IMPL_GEN(i, int64_t, int64, )

#undef SK_LABEL_IMPL_GEN
