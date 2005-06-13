#ifndef __SILC_PRESENCE_H
#define __SILC_PRESENCE_H

#include "lib.h"
#include "presence.h"
#include "gateway-connection.h"
#include "chat-protocol.h"

struct i_silc_presence {
	struct presence presence;

	SilcClientEntry client_entry;
};

struct presence *i_silc_presence_init(struct gateway_connection *gwconn,
		const char *name);
void i_silc_presence_deinit(struct presence *presence);

void i_silc_presence_change_request(struct presence *presence,
		struct event *event);
void i_silc_presence_status_request(struct presence *presence,
		const char *const *status_fields,
		presence_status_request_callback_t *cb, void *context);

#endif /* __SILC_PRESENCE_H */