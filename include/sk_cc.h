#pragma once

#ifndef SK_CACHE_SIZE
#define SK_CACHE_SIZE 64
#endif

/* Function attributes */
#define sk_printf(a, b) __attribute__((format(printf, a, b)))
#define sk_nonnull(...) __attribute__((nonnull(__VA_ARGS__)))
#define sk_constructor __attribute__((constructor))
#define sk_destructor __attribute__((destructor))

/* Type attributes */
#define sk_align(n) __attribute__((aligned(n)))
#define sk_packed __attribute__((packed))
#define sk_cache_aligned sk_align(SK_CACHE_SIZE)

/* sk_log requires 2 attributes */
#define sk_log_attr __attribute__((format(printf, 4, 5), nonnull(1, 4)))

#define sk_likely(x) __builtin_expect((x), 1)
#define sk_unlikely(x) __builtin_expect((x), 0)
#define sk_array_size(x) (sizeof(x) / sizeof(x[0]))

/* Structure that encapsulate file, function and line reference */
struct sk_debug {
	const char *file;
	const char *function;
	int line;
};
typedef struct sk_debug sk_debug_t;

#define sk_debug                                                               \
	(sk_debug_t)                                                               \
	{                                                                          \
		__FILE__, __func__, __LINE__                                           \
	}
