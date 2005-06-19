/* Copyright 2005 Andrej Kacian */

#include <silcincludes.h>
#include <silcclient.h>

#include "lib.h"
#include "chat-protocol.h"
#include "local-user.h"
#include "module.h"

#include "silc.h"
#include "silc-client.h"
#include "silc-gateway.h"
#include "silc-gateway-connection.h"
#include "silc-channel.h"
#include "silc-message.h"
#include "silc-presence.h"

unsigned int silc_module_id;

static struct event_bind_list events[];
static struct event_bind_list high_priority_events[];

/* module init */
void irssi2_silc_init()
{
	/* bind to the local user init event, and register protocol there */
	i_silc_events_init();
	silc_module_id = irssi2_module_id++;
}

/* module deinit */
void irssi2_silc_deinit()
{
	i_silc_events_deinit();
}

static struct chat_protocol *i_silc_alloc(void)
{
	return i_new(struct chat_protocol, 1);
}

static void i_silc_init(struct chat_protocol *protocol __attr_unused__)
{
	i_silc_gateway_connection_events_init();
	i_silc_channel_events_init();
}

static void i_silc_deinit(struct chat_protocol *protocol __attr_unused__)
{
	i_silc_gateway_connection_events_deinit();
	i_silc_channel_events_deinit();
}

unsigned int verify_message_signature(SilcClientEntry sender __attr_unused__,
		SilcMessageSignedPayload sig __attr_unused__,
		SilcMessagePayload payload __attr_unused__)
{
	/* FIXME: do signature check - return "dunno" for now */
	return -1;
}

void i_silc_scheduler(void *client)
{
	silc_client_run_one((SilcClient)client);
}


/* A wrapper that calls silc_client_close_connection() and sets gwconn's fd
 * to -1, so irssi2 won't try to close it */
void i_silc_client_close_connection(struct i_silc_gateway_connection *
		silc_gwconn)
{
	silc_client_close_connection(silc_gwconn->client, silc_gwconn->conn);
	silc_gwconn->gwconn.fd = -1;
}

/* A convenience function for creating "silc_event" events. */
struct event *silc_event_new(struct local_user *local_user, const char *name)
{
	struct event *event = event_new(local_user, SILC_EVENT);
	event_add(event, "event", name);
	return event;
}

void i_silc_events_init(void)
{
	event_bind_list(events, 0);
	event_bind_list(high_priority_events, PRIORITY_HIGH);
}

void i_silc_events_deinit(void)
{
	event_unbind_list(high_priority_events);
	event_unbind_list(events);
}

static void event_local_user_init(struct event *event)
{
	struct local_user *lu = NULL;

	/* grab the newly init'd local_user... */
	lu = event_get_control(event, "local_user");
	i_assert(lu != NULL);

	/* ...and register our protocol with it */
	chat_protocol_register(lu, &silc_protocol);
}

static void event_logged_in(struct event *event)
{
	struct gateway_connection *gwconn =
		event_get_control(event, "gateway_connection");

	i_assert(gwconn != NULL);

	struct i_silc_gateway_connection *silc_gwconn =
		(struct i_silc_gateway_connection *)gwconn;

	if( silc_gwconn->connection_status == SILC_CLIENT_CONN_SUCCESS )
		event_add(event, "resumed", "no");
	else if( silc_gwconn->connection_status ==
			SILC_CLIENT_CONN_SUCCESS_RESUME )
		event_add(event, "resumed", "yes");
}

static struct event_bind_list events[] = {
	{ NULL, EVENT_LOCAL_USER_INIT, event_local_user_init },
	{ NULL, NULL, NULL }
};

static struct event_bind_list high_priority_events[] = {
	{ NULL, EVENT_GATEWAY_LOGGED_IN, event_logged_in },
	{ NULL, NULL, NULL }
};

struct chat_protocol silc_protocol = {
	0,
	"SILC",
	NULL,

	SILC_DEFAULT_CHARSET,

	i_silc_alloc,

	i_silc_init,
	i_silc_deinit,

	i_silc_gateway_init,
	i_silc_gateway_deinit,

	i_silc_gateway_connection_init,
	i_silc_gateway_connection_deinit,

	i_silc_channel_init,
	i_silc_channel_deinit,

	i_silc_presence_init,
	i_silc_presence_deinit,
	
	i_silc_message_send,

	i_silc_join_send,
	i_silc_part_send,

	i_silc_presence_change_request,
	i_silc_presence_status_request
};
