# Surival Kit

Facilities to write production ready services.

## lifecycle

A small state machine that indicates the state your application is.

```
NEW → STARTING → RUNNING → STOPPING → TERMINATED
 └───────┴──────────┴─────────┴─────→ FAILED
```

It provides a epoch history at which time a state was transitioned to.

## health check

Registers callbacks that exports health status of components. This allows the
application to easily expose a health endpoint.

## events (logs)

Log are stored into a ring buffer. Each logger instance has his own
buffer & log level. Loggers are layered in a hierarchy and one can dynamically
manipulate loggers' level with a regex, e.g.

`sk_logger_set_level_match("myapp.db.*", SK_LOG_WARNING);`

## metrics

Registers metrics that represent statistics of components. Supported counter
types:

### counters

### gauge

### histogram

## managed components

Add support for managing components that requires to be properly
started/stopped, e.g. disk flush, database connections.

## watchdog

Add support for automatic process restart, e.g. parent process monitoring.

## exporter

Export the various utilities mentioned earlier to external sources, e.g.

  * health check => nagios
  * events => syslog or udp syslog
  * metrics => prometheus or graphite
