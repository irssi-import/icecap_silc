/*
 * Icecap_silc - a SILC module for Icecap
 * Copyright (C) 2005 Andrej Kacian
 *
 * - Module init and deinit, some support functions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "lib.h"
#include "chat-protocol.h"
#include "module-context.h"
#include "network.h"
#include "local-user.h"
#include "module.h"

#include <silc.h>
#include <silcclient.h>

#include "silc.h"
#include "silc-client.h"
#include "silc-gateway.h"
#include "silc-gateway-connection.h"
#include "silc-channel.h"
#include "silc-channel-connection.h"
#include "silc-channel-presence.h"
#include "silc-message.h"
#include "silc-presence.h"
#include "support.h"

#include "icecap-silc.h"

static struct event_bind_list events[];
static struct event_bind_list high_priority_events[];

/* module init */
void icecap_silc_init()
{
	/* bind to the local user init event, and register protocol there */
	i_silc_events_init();
}

/* module deinit */
void icecap_silc_deinit()
{
	i_silc_events_deinit();
}

static struct chat_protocol *i_silc_alloc(void)
{
	return i_new(struct chat_protocol, 1);
}

static void i_silc_init(struct chat_protocol *protocol)
{
	i_silc_gateway_connection_events_init();
	i_silc_channel_connection_events_init();

	i_silc_presence_commands_init();
}

static void i_silc_deinit(struct chat_protocol *protocol)
{
	i_silc_gateway_connection_events_deinit();
	i_silc_channel_connection_events_deinit();

	i_silc_presence_commands_deinit();
}

void i_silc_scheduler(void *client)
{
	silc_client_run_one((SilcClient)client);
}


/* A wrapper that calls silc_client_close_connection() and sets gwconn's fd
 * to -1, so icecap won't try to close it */
void i_silc_client_close_connection(struct i_silc_gateway_connection *
		silc_gwconn)
{
	silc_client_close_connection(silc_gwconn->client, silc_gwconn->conn);
	silc_gwconn->gwconn.fd = -1;
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
	chat_protocol_register(&silc_protocol);
}

static void event_gateway_logged_in(struct event *event)
{
	struct gateway_connection *gwconn =
		event_get_control(event, gwconn);

	i_assert(gwconn != NULL);

	if( !IS_SILC_GWCONN(gwconn) )
		return;

	struct i_silc_gateway_connection *silc_gwconn =
		(struct i_silc_gateway_connection *)gwconn;

	if( silc_gwconn->connection_status == SILC_CLIENT_CONN_SUCCESS ) {
		event_add(event, "resumed", "no");
	} else if( silc_gwconn->connection_status ==
			SILC_CLIENT_CONN_SUCCESS_RESUME ) {
		event_add(event, "resumed", "yes");
	}
}

static struct event_bind_list events[] = {
	{ NULL, EVENT_LOCAL_USER_INIT, event_local_user_init },
	{ NULL, NULL, NULL }
};

static struct event_bind_list high_priority_events[] = {
	{ NULL, EVENT_GATEWAY_LOGGED_IN, event_gateway_logged_in },
	{ NULL, NULL, NULL }
};

struct chat_protocol silc_protocol = {
	0,
	"SILC",

	SILC_DEFAULT_CHARSET, SILC_DEFAULT_CHARSET,

	i_silc_alloc,

	i_silc_init,
	i_silc_deinit,

	i_silc_gateway_init,
	i_silc_gateway_deinit,

	i_silc_gateway_connection_init,
	i_silc_gateway_connection_deinit,

	i_silc_presence_init,
	i_silc_presence_deinit,

	i_silc_channel_init,
	i_silc_channel_deinit,

	i_silc_channel_connection_init,
	i_silc_channel_connection_deinit,

	i_silc_channel_presence_init,
	i_silc_channel_presence_deinit,

	i_silc_message_send,

	i_silc_presence_change_request,
	i_silc_presence_status_request,

	i_silc_channel_change_request
};
