#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <ck_pr.h>

#include <sk_flag.h>
#include <sk_log.h>
#include <sk_logger_drv.h>

// clang-format off
static const char *level_labels[] = {
	[SK_LOG_EMERGENCY] = "emergency",
	[SK_LOG_ALERT] = "alert",
	[SK_LOG_CRITICAL] = "critical",
	[SK_LOG_ERROR] = "error",
	[SK_LOG_WARNING] = "warning",
	[SK_LOG_NOTICE] = "notice",
	[SK_LOG_INFO] = "info",
	[SK_LOG_DEBUG] = "debug",
};
// clang-format on

const char *
sk_log_level_str(enum sk_log_level level)
{
	return (level < SK_LOG_COUNT) ? level_labels[level] : NULL;
}

enum {
	SK_LOGGER_ENABLED = 1,
};

CK_RING_PROTOTYPE(msg, sk_log_msg)

sk_logger_t *
sk_logger_create(const char *name, uint8_t log_size, sk_logger_drv_t *driver,
    sk_error_t *error)
{
	size_t ring_size = 1 << log_size;

	if (log_size > SK_LOGGER_RING_MAX) {
		sk_error_msg_code(
		    error, "log_size > SK_LOGGER_RING_MAX", SK_LOGGER_EINVAL);
		goto failed;
	}

	sk_logger_t *logger = calloc(1, sizeof(*logger));
	if (logger == NULL) {
		sk_error_msg_code(error, "logger calloc failed", SK_LOGGER_ENOMEM);
		goto failed;
	}

	if ((logger->name = strdup(name)) == NULL) {
		sk_error_msg_code(error, "name strdup failed", SK_LOGGER_ENOMEM);
		goto failed_name_alloc;
	}

	if ((logger->buf = calloc(sizeof(sk_log_msg_t), ring_size)) == NULL) {
		sk_error_msg_code(error, "buffer calloc failed", SK_LOGGER_ENOMEM);
		goto failed_buf_alloc;
	}

	logger->buf_size = ring_size;
	ck_ring_init(&logger->ring, ring_size);
	sk_logger_set_level(logger, SK_LOG_DEFAULT_LEVEL);
	sk_flag_set(&logger->flags, SK_LOGGER_ENABLED);

	/* Find driver */
	if (driver != NULL) {
		logger->driver = *driver;
	} else if (!sk_logger_default_drv(&logger->driver, error)) {
		goto failed_default_driver;
	}

	/* Initialize driver */
	if (logger->driver.open != NULL &&
	    !logger->driver.open(&logger->driver, error))
		goto failed_open_driver;

	return logger;

failed_open_driver:
	if (logger->driver.close != NULL)
		logger->driver.close(&logger->driver);
failed_default_driver:
	free(logger->buf);
failed_buf_alloc:
	free(logger->name);
failed_name_alloc:
	free(logger);
failed:
	return NULL;
}

void
sk_logger_destroy(sk_logger_t *logger)
{
	sk_flag_unset(&logger->flags, SK_LOGGER_ENABLED);

	/* TODO: wait until drained */

	if (logger->driver.close != NULL)
		logger->driver.close(&logger->driver);

	free(logger->buf);
	free(logger->name);
	free(logger);
}

enum sk_log_level
sk_logger_get_level(sk_logger_t *logger)
{
	return (enum sk_log_level)ck_pr_load_int((int *)&logger->level);
}

bool
sk_logger_set_level(sk_logger_t *logger, enum sk_log_level level)
{
	ck_pr_store_int((int *)&logger->level, level);

	return true;
}

bool
sk_log(sk_logger_t *logger, enum sk_log_level level, sk_debug_t debug,
    const char *fmt, ...)
{
	struct timespec time;
	clock_gettime(CLOCK_REALTIME, &time);

	if (sk_logger_get_level(logger) < level)
		return true;

	sk_log_msg_t msg;
	msg.ts_nsec = (time.tv_sec * 1000000) + time.tv_nsec;
	msg.level = level;
	msg.debug = debug;
	msg.pid = getpid();
	msg.tid = syscall(SYS_gettid);

	va_list args;
	va_start(args, fmt);
	if (vsnprintf(msg.payload, SK_LOG_MSG_MAX, fmt, args) == -1)
		return false;
	va_end(args);

	/* TODO(fsaintjacques): blocks on queue full. */
	return ck_ring_enqueue_mpmc_msg(&logger->ring, logger->buf, &msg);
}

bool
sk_logger_drain(sk_logger_t *logger, size_t *drained)
{
	sk_log_msg_t msg;
	sk_error_t error;
	sk_logger_drv_t *driver = &logger->driver;
	bool ok = true;
	size_t count = 0;

	while (ck_ring_trydequeue_mpmc_msg(&logger->ring, logger->buf, &msg)) {
		if (driver->log != NULL) {
			ok &= driver->log(driver, &msg, &error);
			count++;
		}
	}

	*drained = count;
	return ok;
}
