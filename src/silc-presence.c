/* Copyright 2005 Andrej Kacian */

#include <silcincludes.h>
#include <silcclient.h>

#include "lib.h"
#include "presence.h"
#include "gateway-connection.h"
#include "chat-protocol.h"
#include "event.h"

#include "silc-presence.h"
#include "silc-gateway-connection.h"

struct presence *i_silc_presence_init(struct gateway_connection *gwconn,
		const char *name)
{
	struct i_silc_presence *silc_presence;

	silc_presence = i_new(struct i_silc_presence, 1);
	silc_presence->client_entry = NULL;
	return &silc_presence->presence;
}

void i_silc_presence_deinit(struct presence *presence)
{
	struct i_silc_presence *silc_presence =
		(struct i_silc_presence *)presence;
	i_free(silc_presence);
}

void i_silc_presence_change_request(struct presence *presence,
		struct event *event)
{
	struct i_silc_gateway_connection *silc_gwconn =
		(struct i_silc_gateway_connection *)presence->gwconn;
	const char *new_name = event_get(event, "new_name");

	printf("foo\n");
	if( *new_name != '\0' )
		silc_client_command_call(silc_gwconn->client, silc_gwconn->conn,
				NULL, "NICK", new_name, NULL);
}

void i_silc_presence_status_request(struct presence *presence,
		const char *const *status_fields,
		presence_status_request_callback_t *cb, void *context)
{
	/* FIXME: stub! */
}
