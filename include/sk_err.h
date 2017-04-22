#pragma once

struct sk_err {
	/* Numeric error code, can be a placeholder for errno(3) */
	int code;
	/* Statically allocated message. Not meant for heap allocated messages */
	const char *message;
};
typedef struct sk_err sk_err_t;

static inline bool
sk_err_msg(sk_err_t *error, const char *message)
{
	error->code = 0;
	error->message = message;

	return false;
}

static inline bool
sk_err_msg_code(sk_err_t *error, const char *message, int code)
{
	error->code = code;
	error->message = message;

	return false;
}
