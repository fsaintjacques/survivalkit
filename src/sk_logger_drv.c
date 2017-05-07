#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#include <sk_logger_drv.h>

static sk_logger_drv_console_ctx_t __default_console_ctx = {SK_LOG_WARNING};

/* protect with lock */
sk_logger_drv_builder_fn_t drv_builder = sk_logger_drv_builder_console;
void *drv_builder_ctx = &__default_console_ctx;

void
sk_logger_drv_set_default(sk_logger_drv_builder_fn_t builder, void *ctx)
{
	drv_builder = builder;
	drv_builder_ctx = ctx;
}

bool
sk_logger_default_drv(sk_logger_drv_t *driver, sk_error_t *error)
{
	if (drv_builder == NULL)
		return sk_error_msg_code(
			error, "no default driver builder", SK_ERROR_EINVAL);

	return drv_builder(driver, drv_builder_ctx, error);
}

/*
 * null driver
 */
bool
sk_logger_drv_open_null(sk_logger_drv_t *driver, sk_error_t *error)
{
	(void)driver;
	(void)error;

	return true;
}

bool
sk_logger_drv_log_null(
	sk_logger_drv_t *driver, sk_log_msg_t *msg, sk_error_t *error)
{
	(void)driver;
	(void)msg;
	(void)error;

	return true;
}

void
sk_logger_drv_close_null(sk_logger_drv_t *driver)
{
	(void)driver;
}

bool
sk_logger_drv_builder_null(
	sk_logger_drv_t *driver, void *ctx, sk_error_t *error)
{
	(void)error;

	driver->open = sk_logger_drv_open_null;
	driver->log = sk_logger_drv_log_null;
	driver->close = sk_logger_drv_close_null;

	driver->ctx = ctx;

	return true;
}

/*
 * tally driver
 */
bool
sk_logger_drv_open_tally(sk_logger_drv_t *driver, sk_error_t *error)
{
	if ((driver->ctx = calloc(1, sizeof(sk_logger_drv_tally_ctx_t))) == NULL)
		return sk_error_msg_code(
			error, "tally ctx calloc failed", SK_ERROR_ENOMEM);

	return true;
}

bool
sk_logger_drv_log_tally(
	sk_logger_drv_t *driver, sk_log_msg_t *msg, sk_error_t *error)
{
	(void)error;
	sk_logger_drv_tally_ctx_t *ctx = driver->ctx;

	ck_pr_inc_64(&ctx->counters[msg->level]);

	return true;
}

void
sk_logger_drv_close_tally(sk_logger_drv_t *driver)
{
	free(driver->ctx);
}

bool
sk_logger_drv_builder_tally(
	sk_logger_drv_t *driver, void *ctx, sk_error_t *error)
{
	(void)ctx;
	(void)error;

	driver->open = sk_logger_drv_open_tally;
	driver->log = sk_logger_drv_log_tally;
	driver->close = sk_logger_drv_close_tally;

	driver->ctx = NULL;

	return true;
}

/*
 * console driver
 */
bool
sk_logger_drv_log_console(
	sk_logger_drv_t *driver, sk_log_msg_t *msg, sk_error_t *error)
{
	const enum sk_log_level threshold =
		((sk_logger_drv_console_ctx_t *)driver->ctx)->threshold;

	FILE *fd = (msg->level <= threshold) ? stderr : stdout;

	if (fprintf(fd, "%f %s {file: %s, func: %s, line: %d} [%s]: %s\n",
			msg->ts_nsec / 1000000.0, "logger", msg->debug.file,
			msg->debug.function, msg->debug.line, sk_log_level_str(msg->level),
			msg->payload) < 0) {
		return sk_error_msg(error, "failed to print to console");
	}

	return true;
}

void
sk_logger_drv_close_console(sk_logger_drv_t *driver)
{
	free(driver->ctx);
}

bool
sk_logger_drv_builder_console(
	sk_logger_drv_t *driver, void *ctx, sk_error_t *error)
{
	(void)error;

	sk_logger_drv_console_ctx_t *console_ctx = calloc(1, sizeof(*console_ctx));
	if (console_ctx == NULL)
		return sk_error_msg_code(
			error, "console_ctx calloc failed", SK_ERROR_ENOMEM);
	memcpy(console_ctx, ctx, sizeof(*console_ctx));

	/* No need a custom opener since we initialize the context in the builder */
	driver->open = sk_logger_drv_open_null;
	driver->log = sk_logger_drv_log_console;
	driver->close = sk_logger_drv_close_console;

	driver->ctx = console_ctx;

	return true;
}

/*
 * syslog driver
 */
bool
sk_logger_drv_open_syslog(sk_logger_drv_t *driver, sk_error_t *error)
{
	(void)error;

	sk_logger_drv_syslog_ctx_t *ctx = driver->ctx;
	openlog(ctx->ident, ctx->option, ctx->facility);

	return true;
}

bool
sk_logger_drv_log_syslog(
	sk_logger_drv_t *driver, sk_log_msg_t *msg, sk_error_t *error)
{
	(void)driver;
	(void)error;

	syslog(msg->level, "%s {file: %s, func: %s, line: %d} %s\n", "logger",
		msg->debug.file, msg->debug.function, msg->debug.line, msg->payload);

	return true;
}

void
sk_logger_drv_close_syslog(sk_logger_drv_t *driver)
{
	/* Other drivers might use syslog */
	// closelog();
	free(driver->ctx);
}

bool
sk_logger_drv_builder_syslog(
	sk_logger_drv_t *driver, void *ctx, sk_error_t *error)
{
	(void)error;

	sk_logger_drv_syslog_ctx_t *syslog_ctx = calloc(1, sizeof(*syslog_ctx));
	if (syslog_ctx == NULL)
		return sk_error_msg_code(
			error, "syslog_ctx calloc failed", SK_ERROR_ENOMEM);
	memcpy(syslog_ctx, ctx, sizeof(*syslog_ctx));

	driver->open = sk_logger_drv_open_syslog;
	driver->log = sk_logger_drv_log_syslog;
	driver->close = sk_logger_drv_close_syslog;

	driver->ctx = syslog_ctx;

	return true;
}
