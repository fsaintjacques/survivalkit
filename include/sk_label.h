#pragma once

#include <stdint.h>
#include <string.h>

#include <sk_cc.h>
#include <sk_error.h>

struct sk_label_descriptor {
	/* Name of the label */
	const char *key;

	/* Type of value associated with the label */
	enum {
		SK_VALUE_STRING = 0,
		SK_VALUE_BOOL,
		SK_VALUE_INT64,
	} value_type;

	/* Short description of the label */
	const char *description;
};
typedef struct sk_label_descriptor sk_label_descriptor_t;

bool
sk_registry_init(sk_error_t *error) sk_nonnull(1);

bool
sk_label_register(const sk_label_descriptor_t *descriptor, sk_error_t *error)
	sk_nonnull(1, 2);

sk_label_descriptor_t *
sk_label_get(const char *key);

union sk_label_value {
	char *string_value;
	bool bool_value;
	int64_t int64_value;
};
typedef union sk_label_value sk_label_value_t;

struct sk_label {
	const sk_label_descriptor_t *descriptor;
	sk_label_value_t value;
};
typedef struct sk_label sk_label_t;

sk_label_t
sk_label_s(const char *key, const char *value) sk_nonnull(1, 2);

sk_label_t
sk_label_b(const char *key, bool value) sk_nonnull(1);

sk_label_t
sk_label_i(const char *key, int64_t value) sk_nonnull(1);
