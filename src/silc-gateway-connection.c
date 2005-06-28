/*
 * Irssi2_silc - a SILC module for Irssi2
 * Copyright (C) 2005 Andrej Kacian
 *
 * - Functions related to struct i_silc_gateway_connection
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

#include <silcincludes.h>
#include <silcclient.h>

#include "lib.h"
#include "event.h"
#include "network.h"
#include "gateway.h"
#include "connection.h"
#include "chat-protocol.h"
#include "hash.h"
#include "ioloop.h"
#include "presence.h"
#include "local-user.h"
#include "local-presence.h"

#include "silc.h"
#include "support.h"
#include "silc-gateway.h"
#include "silc-gateway-connection.h"
#include "silc-client.h"

extern SilcClientOperations ops;
	
struct gateway_connection *
i_silc_gateway_connection_init(struct gateway *gw __attr_unused__,
				struct local_presence *lp)
{
	struct i_silc_gateway_connection *silc_gwconn;

	silc_gwconn = i_new(struct i_silc_gateway_connection, 1);
	silc_gwconn->ops = ops;

	silc_gwconn->client = i_silc_client_init(lp);

	silc_gwconn->client->nickname = lp->name;

	silc_gwconn->gwconn.presences = hash_create(default_pool, default_pool,
					0, (hash_callback_t *)strcase_hash,
					(hash_cmp_callback_t *)strcasecmp);
	silc_gwconn->gwconn.connection = gw->connection;

	return &silc_gwconn->gwconn;
}

void i_silc_gateway_connection_deinit(struct gateway_connection *gwconn)
{
	struct i_silc_gateway_connection *silc_gwconn =
		(struct i_silc_gateway_connection *)gwconn;

	i_silc_client_deinit(silc_gwconn->client, silc_gwconn->conn);
	i_free(gwconn);
	/* FIXME: stub! */
}

static void event_gateway_connected(struct event *event)
{
	struct gateway_connection *gwconn;
	struct i_silc_gateway_connection *silc_gwconn;

	gwconn = event_get_control(event, "gateway_connection");
	if( gwconn == NULL ||
		!IS_SILC_PROTOCOL(gwconn->gateway->network->protocol))
			return;

	silc_gwconn = (struct i_silc_gateway_connection *)gwconn;

	silc_gwconn->conn = silc_client_add_connection(silc_gwconn->client,
				NULL, gwconn->gateway->connection->hostname,
				gwconn->port, NULL);
	if( !silc_gwconn->conn ) printf("connection NOT added!\n");
	silc_client_start_key_exchange(silc_gwconn->client, silc_gwconn->conn,
			silc_gwconn->gwconn.fd);
	silc_gwconn->timeout = timeout_add(200, i_silc_scheduler, silc_gwconn->client);
}

static void event_gateway_disconnected(struct event *event)
{
	struct gateway_connection *gwconn;
	struct i_silc_gateway_connection *silc_gwconn;

	gwconn = event_get_control(event, "gateway_connection");
	if( gwconn == NULL ||
		!IS_SILC_PROTOCOL(gwconn->gateway->network->protocol))
			return;

	silc_gwconn = (struct i_silc_gateway_connection *)gwconn;

	i_silc_client_close_connection(silc_gwconn);
	timeout_remove(silc_gwconn->timeout);
}

static struct event_bind_list events[] = {
	{ NULL, EVENT_GATEWAY_CONNECTED, event_gateway_connected },
	{ NULL, EVENT_GATEWAY_DISCONNECTED, event_gateway_disconnected },
	{ NULL, EVENT_PRESENCE_DEINIT, event_gateway_disconnected },

	{ NULL, NULL, NULL }
};

void i_silc_gateway_connection_events_init(void)
{
	event_bind_list(events, 0);
}

void i_silc_gateway_connection_events_deinit(void)
{
	event_unbind_list(events);
}

struct gateway_connection *
i_silc_gateway_connection_lookup_conn(SilcClientConnection conn)
{
	struct chat_protocol *proto = chat_protocol_lookup("SILC");
	struct local_user *lu = proto->local_user;
	struct gateway_connection *const *g;
	unsigned int i, count;

	i_assert(conn != NULL);
	g = array_get(&lu->gateway_connections, &count);
	for( i = 0; i < count; i++ ) {
		if( ((struct i_silc_gateway_connection *)g[i])->conn == conn ) {
			return g[i];
		}
	}

	return NULL;
}
