# Surival Kit

A survival kit is a package of basic tools and supplies prepared in advance as
an aid to survival in an emergency.

This library aims to offer facilities to ease the implementation of production
ready services.

## Goals

* Linux first, readability of code is much more important than portability
* Thread safe, lightweight
* Container friendly, does not depends on local external processes
* Integrate easily with multiple backend

## Facilities


### Lifecycle

Lifecycle is a thread safe state machine representing the operational state
of a component. A lifecycle might serve many purposes:

* Improve auditing via event logs
* Automatically toggles healthcheck in STARTING and STOPPING transitions
* Centralize the exit condition of a main loop

The possibles transitions are given by the following state machine:

```
  NEW → STARTING → RUNNING → STOPPING → TERMINATED
   └───────┴──────────┴─────────┴─────→ FAILED
```

### Health checks

Registers callbacks that exports health status of components. This allows the
application to easily expose a health endpoint.

### Logs

Log are stored into a ring buffer. Each logger instance has his own
buffer & log level. Loggers are layered in a hierarchy and one can dynamically
manipulate loggers' level with a regex, e.g.

`sk_logger_set_level_match("myapp.db.*", SK_LOG_WARNING);`

### Metrics

Registers metrics that represent statistics of components. Supported counter
types:

* counter
* gauge
* histogram

### Managed components

Add support for managing components that requires to be properly
started/stopped, e.g. disk flush, database connections.

### Watchdog

Add support for automatic process restart, e.g. parent process monitoring.

### Exporter

Export the various utilities mentioned earlier to external sources, e.g.

  * health check => nagios
  * events => syslog or udp syslog
  * metrics => prometheus or graphite
