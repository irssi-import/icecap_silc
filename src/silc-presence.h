#ifndef __SILC_PRESENCE_H
#define __SILC_PRESENCE_H

#include "lib.h"
#include "presence.h"
#include "gateway-connection.h"
#include "chat-protocol.h"
#include "server-event.h"

struct i_silc_presence {
	struct presence presence;

	SilcClientEntry client_entry;
};

void i_silc_presence_commands_init(void);
void i_silc_presence_commands_deinit(void);

struct presence *i_silc_presence_init(struct gateway_connection *gwconn,
		const char *name);
void i_silc_presence_deinit(struct presence *presence);

void i_silc_presence_change_request(struct local_presence *lp,
		struct event *event, async_change_request_callback_t *cb,
		struct client_async_cmd_context *context);
void i_silc_presence_status_request(struct presence *presence,
		const char *const *status_fields,
		presence_status_request_callback_t *cb, void *context);

char *i_silc_userhost(SilcClientEntry client_entry);

#endif /* __SILC_PRESENCE_H */
