#pragma once

#include <ck_queue.h>

#define SK_LISTENER(prefix, callback_type)                                     \
	struct prefix##_listener {                                                 \
		char *name;                                                            \
		callback_type callback;                                                \
		void *ctx;                                                             \
                                                                               \
		CK_SLIST_ENTRY(prefix##_listener) next;                                \
	};                                                                         \
	typedef struct prefix##_listener prefix##_listener_t
