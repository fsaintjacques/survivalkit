#pragma once

#include <stdint.h>
#include <sys/types.h>

#include <ck_ring.h>

#include <sk_error.h>
#include <sk_flag.h>
#include <sk_log.h>

enum {
	SK_LOG_MSG_MAX = 256,
};

struct sk_log_msg {
	/* Timestamp in nanoseconds */
	uint64_t ts_nsec;
	/* Log level of the message */
	enum sk_log_level level;
	/* Debug information */
	sk_debug_t debug;

	pid_t pid, tid;

	/* Message */
	char payload[SK_LOG_MSG_MAX];
};
typedef struct sk_log_msg sk_log_msg_t;

typedef bool (*sk_logger_open_fn_t)(sk_logger_drv_t *, sk_error_t *);
typedef bool (*sk_logger_log_fn_t)(
    sk_logger_drv_t *, sk_log_msg_t *, sk_error_t *);
typedef void (*sk_logger_close_fn_t)(sk_logger_drv_t *);

struct sk_logger_drv {
	sk_logger_open_fn_t open;
	sk_logger_log_fn_t log;
	sk_logger_close_fn_t close;
	/* used to store driver configuration and state */
	void *ctx;
};

struct sk_logger {
	char *name;

	sk_flag_t flags;

	enum sk_log_level level;

	ck_ring_t ring;
	size_t buf_size;
	sk_log_msg_t *buf;

	sk_logger_drv_t driver;
};

bool
sk_logger_drain(sk_logger_t *logger, size_t *drained, sk_error_t *error)
    sk_nonnull(1, 2);
typedef bool (*sk_logger_drv_builder_fn_t)(
    sk_logger_drv_t *drv, void *ctx, sk_error_t *error);

/* Construct a driver provided by the default builder */
bool
sk_logger_default_drv(sk_logger_drv_t *driver, sk_error_t *error);

/* Set a global driver builder */
void
sk_logger_drv_set_default(sk_logger_drv_builder_fn_t builder, void *ctx);

/* null driver: send message to /dev/null. */
bool
sk_logger_drv_builder_null(
    sk_logger_drv_t *driver, void *ctx, sk_error_t *error);

/* tally driver: count messages of each level. */
struct sk_logger_drv_tally_ctx {
	uint64_t counters[SK_LOG_COUNT];
};
typedef struct sk_logger_drv_tally_ctx sk_logger_drv_tally_ctx_t;
bool
sk_logger_drv_builder_tally(
    sk_logger_drv_t *driver, void *ctx, sk_error_t *error);

/* console driver: log message to stdout/stderr. */
struct sk_logger_drv_console_ctx {
	/*
	 * Levels higher or equal this threshold will be printed to stderr, others
	 * will be routed to stdout.
	 */
	enum sk_log_level threshold;
};
typedef struct sk_logger_drv_console_ctx sk_logger_drv_console_ctx_t;

bool
sk_logger_drv_builder_console(
    sk_logger_drv_t *driver, void *ctx, sk_error_t *error);

/* syslog driver: log message with syslog(3) */
struct sk_logger_drv_syslog_ctx {
	/* Arguments passed to openlog(3) */
	char *ident;
	int option;
	int facility;
};
typedef struct sk_logger_drv_syslog_ctx sk_logger_drv_syslog_ctx_t;

bool
sk_logger_drv_builder_syslog(
    sk_logger_drv_t *driver, void *ctx, sk_error_t *error);
