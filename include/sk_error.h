#pragma once

#include <errno.h>
#include <stddef.h>

struct sk_error {
	/* Numeric error code, can be a placeholder for errno(3) */
	int code;
	/* Statically allocated message. Not meant for heap string */
	const char *message;
};
typedef struct sk_error sk_error_t;

static inline bool
sk_error_msg(sk_error_t *error, const char *message)
{
	error->code = -1;
	error->message = message;

	return false;
}

static inline bool
sk_error_msg_code(sk_error_t *error, const char *message, int code)
{
	error->code = code;
	error->message = message;

	return false;
}

static inline bool
sk_error_code(sk_error_t *error, int code)
{
	error->code = code;
	error->message = NULL;

	return false;
}

static inline bool
sk_error_errno(sk_error_t *error)
{
	error->code = errno;
	/*
	 * Thanks glibc: https://github.com/fish-shell/fish-shell/issues/1830
	 * error->message = sys_errlist[code];
	 */
	error->message = NULL;

	return false;
}
