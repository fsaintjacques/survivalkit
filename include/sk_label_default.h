#pragma once

#include <sk_label.h>

static const sk_label_descriptor_t sk_default_labels[] = {
	{"hostname", SK_VALUE_STRING, "hostname of the host running the service"},

	{"datacenter", SK_VALUE_STRING,
		"datacenter where the machine is physically located"},

	{"service", SK_VALUE_STRING, "name of the service"},

	{"environment", SK_VALUE_STRING, "environment, e.g. production, staging"},

	{"version", SK_VALUE_STRING, "version of the monitored resource"},

	{"revision", SK_VALUE_STRING, "SCM revision of the service"},

	{"pid", SK_VALUE_INT64, "process identifier of the service"},
};
