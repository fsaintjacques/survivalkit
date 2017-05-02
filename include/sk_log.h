#pragma once

#include <stddef.h>
#include <stdint.h>

#include <sk_cc.h>
#include <sk_error.h>

/* The level determines the importance of the message, see syslog(3). */
enum sk_log_level {
	/* System is unusable */
	SK_LOG_EMERGENCY = 0,
	/* Action must be taken immediately */
	SK_LOG_ALERT,
	/* Critical conditions */
	SK_LOG_CRITICAL,
	/* Error conditions */
	SK_LOG_ERROR,
	/* Warning conditions */
	SK_LOG_WARNING,
	/* Normal but significant condition */
	SK_LOG_NOTICE,
	/* Informational messages */
	SK_LOG_INFO,
	/* Debug-level messages */
	SK_LOG_DEBUG,

	/* Do not use, leave at the end */
	SK_LOG_COUNT,
};

/*
 * String representation of a log level.
 *
 * @param level, level for which the string representation is requested
 *
 * @return pointer to const string representation, NULL on error
 */
const char *
sk_log_level_str(enum sk_log_level level);

/* Default log level of newly created logger */
#define SK_LOG_DEFAULT_LEVEL SK_LOG_NOTICE

/*
 * A logger is a FIFO where messages are enqueued with various information such
 * as the log level, the time the message occurred, etc.
 *
 * Messages are dequeued in a separate thread and forwarded to a runtime
 * defined backend.
 */
struct sk_logger;
typedef struct sk_logger sk_logger_t;

/*
 * A logger driver is an interface implementing the backend storage layer. See
 * `sk_logger_drv.h` for information on drivers.
 */
struct sk_logger_drv;
typedef struct sk_logger_drv sk_logger_drv_t;

/* Errors returned by the logger APIs */
enum sk_healthcheck_errno {
	SK_LOGGER_OK = 0,
	/* No enough space */
	SK_LOGGER_ENOMEM = ENOMEM,
	/* Driver not supported */
	SK_LOGGER_EINVAL = EINVAL,
};

enum {
	/* Maximum size of a logger's ring buffer */
	SK_LOGGER_RING_MAX = 16,
};

/*
 * Initialize a logger.
 *
 * Loggers are named via the `name` parameter; this can be useful when applying
 * log level manipulation by filtering loggers on their name with regular
 * expression.
 *
 * The driver parameter controls how the logger will store messages. If NULL,
 * the logger will fallback to the default driver, see
 * `sk_logger_drv_set_default`. The default driver is the console driver which
 * logs message to stdout and stderr.
 *
 * @param name, name of the logger
 * @param log_size, logger capacity 2^log_size
 * @param driver, backend storage for the logger
 * @param error, error to store failure information
 *
 * @return newly allocated logger on success, or NULL on failure and set error
 *
 * @errors SK_LOGGER_ENOMEM, if memory allocations failed
 *         SK_LOGGER_EINVAL, if log_size > SK_LOGER_RING_MAX
 *         The driver open function may also return a custom error_code
 */
sk_logger_t *
sk_logger_create(const char *name, uint8_t log_size, sk_logger_drv_t *driver,
	sk_error_t *error) sk_nonnull(1, 4);

/*
 * Free a logger.
 *
 * @param logger, logger to free
 */
void
sk_logger_destroy(sk_logger_t *logger) sk_nonnull(1);

/*
 * Get the effective log level of a logger.
 *
 * @param logger, logger to get the level from
 *
 * @return the log level
 */
enum sk_log_level
sk_logger_get_level(sk_logger_t *logger) sk_nonnull(1);

/*
 * Set the effective log level of a logger.
 *
 * All future message of priority lower then the provided level will be
 * discarded.
 *
 * @param logger, logger to get the level from
 * @param level, level to set to
 *
 * @return true on success, false on failure
 */
bool
sk_logger_set_level(sk_logger_t *logger, enum sk_log_level level) sk_nonnull(1);

/*
 * Log a message.
 *
 * @param logger, logger to log the message to
 * @param level, level of the message
 * @param debug, captured information of the caller, see sk_debug_t
 * @param fmt, printf format definition to construct the message
 * @param ..., arguments of provided for the formatting
 *
 * @return true on success, false on failure
 */
bool
sk_log(sk_logger_t *logger, enum sk_log_level level, sk_debug_t debug,
	const char *fmt, ...) sk_log_attr;

/* Convenience macros that captures sk_debug_t and currify the level */

#define sk_log_debug(logger, fmt, ...)                                         \
	sk_log((logger), SK_LOG_DEBUG, sk_debug, (fmt), ##__VA_ARGS__)

#define sk_log_info(logger, fmt, ...)                                          \
	sk_log((logger), SK_LOG_INFO, sk_debug, (fmt), ##__VA_ARGS__)

#define sk_log_notice(logger, fmt, ...)                                        \
	sk_log((logger), SK_LOG_NOTICE, sk_debug, (fmt), ##__VA_ARGS__)

#define sk_log_warning(logger, fmt, ...)                                       \
	sk_log((logger), SK_LOG_WARNING, sk_debug, (fmt), ##__VA_ARGS__)

#define sk_log_error(logger, fmt, ...)                                         \
	sk_log((logger), SK_LOG_ERROR, sk_debug, (fmt), ##__VA_ARGS__)

#define sk_log_critical(logger, fmt, ...)                                      \
	sk_log((logger), SK_LOG_CRITICAL, sk_debug, (fmt), ##__VA_ARGS__)

#define sk_log_alert(logger, fmt, ...)                                         \
	sk_log((logger), SK_LOG_ALERT, sk_debug, (fmt), ##__VA_ARGS__)

#define sk_log_emergency(logger, fmt, ...)                                     \
	sk_log((logger), SK_LOG_EMERGENCY, sk_debug, (fmt), ##__VA_ARGS__)
