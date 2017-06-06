#include <sk_label.h>

#include "test.h"

static inline sk_label_t
label_default(const sk_label_descriptor_t *desc)
{
	switch (desc->value_type) {
	case SK_VALUE_STRING:
		return sk_label_s(desc->key, "test");
	case SK_VALUE_BOOL:
		return sk_label_b(desc->key, true);
	case SK_VALUE_INT64:
		return sk_label_i(desc->key, 1);
	}

	return (sk_label_t){NULL, {NULL}};
}

static void
label_basic()
{
	sk_error_t error;

	const sk_label_descriptor_t descriptors[] = {
		{"test_string", SK_VALUE_STRING, "test string"},
		{"test_bool", SK_VALUE_BOOL, "test bool"},
		{"test_int", SK_VALUE_INT64, "test int"},
	};

	assert_true(sk_registry_init(&error));

	for (size_t i = 0; i < sk_array_size(descriptors); i++) {
		const sk_label_descriptor_t desc = descriptors[i];

		assert_null(label_default(&desc).descriptor);
		assert_true(sk_label_register(&desc, &error));
		assert_non_null(label_default(&desc).descriptor);
	}
};

int
main()
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(label_basic),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
