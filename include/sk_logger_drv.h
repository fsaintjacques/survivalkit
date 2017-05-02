#pragma once

#include <stdint.h>
#include <sys/types.h>

#include <ck_ring.h>

#include <sk_error.h>
#include <sk_flag.h>
#include <sk_log.h>

enum {
	/* Message maximum size */
	SK_LOG_MSG_MAX = 256,
};

struct sk_log_msg {
	/* Timestamp in nanoseconds */
	uint64_t ts_nsec;
	/* Log level of the message */
	enum sk_log_level level;
	/* Debug information */
	sk_debug_t debug;

	/* Process and thread identifiers captured while logging the message  */
	pid_t pid, tid;

	/* Message */
	char payload[SK_LOG_MSG_MAX];
};
typedef struct sk_log_msg sk_log_msg_t;

typedef bool (*sk_logger_open_fn_t)(sk_logger_drv_t *, sk_error_t *);
typedef bool (*sk_logger_log_fn_t)(
	sk_logger_drv_t *, sk_log_msg_t *, sk_error_t *);
typedef void (*sk_logger_close_fn_t)(sk_logger_drv_t *);

/*
 * A logger driver process messages.
 *
 * The driver decides how messages are handled; drivers will usually forward the
 * messages to a specific backed, e.g. syslog, console, files, ...
 *
 */
struct sk_logger_drv {
	/*
	 * A pointer to an opaque structure. The driver should store all required
	 * information by open & log callbacks in this structure. It is advised that
	 * the close callback free all allocated memory by the open callback (and
	 * possibly the builder).
	 */
	void *ctx;
	/*
	 * Callback that initialize the driver. It should return true on success or
	 * false on failure and set the error message.
	 */
	sk_logger_open_fn_t open;
	/*
	 * Callback that process a message. It should return true on success or
	 * false on failure and set the error message.
	 */
	sk_logger_log_fn_t log;
	/* Callback that close the driver. */
	sk_logger_close_fn_t close;
};

/*
 * A logger is a ring buffer of messages to be processed by the driver.
 */
struct sk_logger {
	/* Name of the logger */
	char *name;

	/* Various flags */
	sk_flag_t flags;

	/* Current minimum log level threshold */
	enum sk_log_level level;

	/* Ring buffer storing messages */
	ck_ring_t ring;
	size_t buf_size;
	sk_log_msg_t *buf;

	/* Driver */
	sk_logger_drv_t driver;
};

/*
 * Logger's ring buffer drain method.
 *
 * Each message will be processed by the driver log callback.
 *
 * @param logger, logger to drain message from
 * @param drained, counter to store the number of drained messages
 * @param maximum_drain, stop here if not fully drained
 * @param error, error to store failure information
 *
 * @return true on success, false otherwise and set error
 */
bool
sk_logger_drain(sk_logger_t *logger, size_t *drained, size_t maximum_drain, sk_error_t *error)
    sk_nonnull(1, 2, 4);

/*
 * A driver builder is a function and a context that instantiate drivers.
 *
 * The builder is useful in situation where the user doesn't have to pass an
 * explicit driver to loggers it initialize. It also enables external library,
 * e.g. LD_PRELOAD=my-driver-impl.so, to set drivers without recompiling.
 *
 * The builder should allocate a driver context (in `drv->ctx`) with values
 * required by open callback (`drv->open`).
 *
 * @param drv, driver struct to instantiate into
 * @param ctx, context passed to builder
 * @param error, error to store failure information
 *
 * @return true on success, false otherwise and set error
 */
typedef bool (*sk_logger_drv_builder_fn_t)(
	sk_logger_drv_t *drv, void *ctx, sk_error_t *error);

/*
 * Instantiate a driver provided by the default builder.
 *
 * @param driver, drive to instantiate to
 * @param error, error to store failure information
 *
 * @return true on success, false otherwise and set error
 */
bool
sk_logger_default_drv(sk_logger_drv_t *driver, sk_error_t *error)
	sk_nonnull(1, 2);

/*
 * Set a global driver builder.
 *
 * `sk_logger_create` invoked without a driver will fallback to instantiate a
 * driver with the builder set by this method.
 *
 * @param builder, global builder to set to
 * @param ctx, global context to set to
 */
void
sk_logger_drv_set_default(sk_logger_drv_builder_fn_t builder, void *ctx)
	sk_nonnull(1);

/*
 * Null driver
 *
 * Send messages to /dev/null.
 */
bool
sk_logger_drv_builder_null(
	sk_logger_drv_t *driver, void *ctx, sk_error_t *error) sk_nonnull(1, 3);

/*
 * Tally driver
 *
 * Count messages processed by severity level.
 */
struct sk_logger_drv_tally_ctx {
	uint64_t counters[SK_LOG_COUNT];
};
typedef struct sk_logger_drv_tally_ctx sk_logger_drv_tally_ctx_t;
bool

sk_logger_drv_builder_tally(
	sk_logger_drv_t *driver, void *ctx, sk_error_t *error) sk_nonnull(1, 2, 3);

/*
 * Console driver
 *
 * Log message to stdout/stderr.
 */
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
	sk_logger_drv_t *driver, void *ctx, sk_error_t *error) sk_nonnull(1, 2, 3);

/*
 * Syslog driver
 *
 * log message with syslog(3)
 */
struct sk_logger_drv_syslog_ctx {
	/* Arguments passed to openlog(3) */
	char *ident;
	int option;
	int facility;
};
typedef struct sk_logger_drv_syslog_ctx sk_logger_drv_syslog_ctx_t;

bool
sk_logger_drv_builder_syslog(
	sk_logger_drv_t *driver, void *ctx, sk_error_t *error) sk_nonnull(1, 2, 3);
