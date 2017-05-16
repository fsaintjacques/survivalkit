#pragma once

#include <ck_pr.h>

#include <stdint.h>

/* A flag is a packed representation of 64 bits */
typedef uint64_t sk_flag_t;

#define sk_flag_get(flag, value) (ck_pr_load_64((flag)) & (value))

#define sk_flag_set(flag, value) ck_pr_or_64(flag, value)

#define sk_flag_unset(flag, value) ck_pr_and_64(flag, ~value)
