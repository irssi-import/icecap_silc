/*
 * Irssi2_silc - a SILC module for Irssi2
 * Copyright (C) 2005 Andrej Kacian
 *
 * - Functions related to struct i_silc_channel_connection
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
#include "tree.h"
#include "presence.h"
#include "network.h"
#include "gateway.h"
#include "tree.h"

#include "silc.h"
#include "support.h"
#include "silc-channel-connection.h"
#include "silc-gateway-connection.h"
#include "silc-presence.h"

static struct event_bind_list events[];

void i_silc_channel_connection_events_init(void)
{
	event_bind_list(events, 0);
}

void i_silc_channel_connection_events_deinit(void)
{
	event_unbind_list(events);
}

struct channel_connection *
i_silc_channel_connection_init(
		struct gateway_connection *gwconn __attr_unused__,
		struct channel *channel,
		struct event *event __attr_unused__)
{
	struct i_silc_channel_connection *silc_chconn =
				i_new(struct i_silc_channel_connection, 1);

	silc_chconn->name = i_strdup(channel->name);
	silc_chconn->chconn.presences =
		tree_create(default_pool, (tree_cmp_callback_t *)strcasecmp);
	return &silc_chconn->chconn;
}

void i_silc_channel_connection_deinit(struct channel_connection *chconn)
{
	struct i_silc_channel_connection *silc_chconn =
		(struct i_silc_channel_connection *)chconn;
	i_free(silc_chconn->name);
	i_free(silc_chconn);
}

void event_channel_connection_init(struct event *event)
{
	struct channel_connection *chconn = event_get_channel_conn(event);

	if( !IS_SILC_PROTOCOL(chconn->gwconn->gateway->network->protocol) )
		return;

	SilcBuffer idp;
	struct i_silc_gateway_connection *silc_gwconn =
		(struct i_silc_gateway_connection *)chconn->gwconn;
	const char *channel_str = chconn->channel->name;

	idp = silc_id_payload_encode(silc_gwconn->conn->local_id,
			SILC_ID_CLIENT);

	i_assert(idp != NULL);

	silc_client_command_send(silc_gwconn->client, silc_gwconn->conn,
			SILC_COMMAND_JOIN, 0, 2, 1, channel_str,
			strlen(channel_str), 2, idp->data, idp->len);
}

void event_channel_connection_deinit(struct event *event)
{
	struct channel_connection *chconn = event_get_channel_conn(event);

	if( !IS_SILC_PROTOCOL(chconn->gwconn->gateway->network->protocol) )
		return;

	struct i_silc_gateway_connection *silc_gwconn =
		(struct i_silc_gateway_connection *)chconn->gwconn;
	const char *channel_str = chconn->channel->name;

	silc_client_command_call(silc_gwconn->client, silc_gwconn->conn, NULL,
						"LEAVE", channel_str, NULL);
}

struct i_silc_channel_connection *
i_silc_channel_connection_lookup_entry(
		struct i_silc_gateway_connection *silc_gwconn,
		SilcChannelEntry entry)
{
	struct channel_connection *const *c;
	unsigned int i, count;
	struct i_silc_channel_connection *silc_chconn;
	struct gateway_connection *gwconn = &silc_gwconn->gwconn;

	c = array_get(&gwconn->channel_connections, &count);
	for( i = 0; i < count; i++ ) {
		silc_chconn = (struct i_silc_channel_connection *)c[i];
		i_assert(silc_chconn != NULL);
		i_assert(silc_chconn->channel_entry != NULL);
		if( silc_chconn->channel_entry == entry )
			return silc_chconn;
	}

	return NULL;
}

struct i_silc_channel_connection *
i_silc_channel_connection_lookup(struct i_silc_gateway_connection *silc_gwconn,
		const char *name) {
	struct channel_connection *const *c;
	unsigned int i, count;
	struct gateway_connection *gwconn = &silc_gwconn->gwconn;

	i_assert(gwconn != NULL);
	i_assert(name != NULL);

	c = array_get(&gwconn->channel_connections, &count);
	for( i = 0; i < count; i++ ) {
		if( strcasecmp(c[i]->channel->name, name) == 0 )
			return (struct i_silc_channel_connection *)c[i];
	}
	return NULL;
}

static void refresh_nicklist_resolved(SilcClient client,
		SilcClientConnection conn, SilcClientEntry *clients,
		SilcUInt32 clients_count __attr_unused__, void *context)
{
	SilcJoinResolve *r = context;
	SilcChannelEntry channel_entry = r->channel;
	SilcHashTableList htl;
	SilcChannelUser chu;
	struct presence *presence;
	struct i_silc_presence *silc_presence;
	struct i_silc_gateway_connection *silc_gwconn;
	struct i_silc_channel_connection *silc_chconn;
	int usercount = 0;
	char *userhost;

	silc_gwconn = (struct i_silc_gateway_connection *)
		i_silc_gateway_connection_lookup_conn(conn);
	i_assert(silc_gwconn != NULL);
	silc_chconn = i_silc_channel_connection_lookup_entry(silc_gwconn,
								channel_entry);
	i_assert(silc_chconn != NULL);

	if( !clients && r->retry < 1 ) {
		r->retry++;
		silc_client_get_clients_by_channel(client, conn, channel_entry,
				refresh_nicklist_resolved, context);
		return;
	}

	silc_hash_table_list(channel_entry->user_list, &htl);
	while( silc_hash_table_get(&htl, NULL, (void *)&chu) ) {
		if( !chu->client->nickname ) continue;
		usercount++;

		/* Do not init myself, as I'm already inited */
		if( chu->client == silc_gwconn->conn->local_entry ) continue;

		userhost = i_silc_userhost(chu->client);

		presence = presence_lookup(&silc_gwconn->gwconn,
				chu->client->nickname);
		if( presence == NULL ) {
			presence = presence_init(&silc_gwconn->gwconn,
					chu->client->nickname);
			presence_set_address(presence, userhost);
			presence_set_real_name(presence,
					chu->client->realname);
			channel_connection_add_presence(&silc_chconn->chconn,
								presence);
			presence_unref(presence);
			silc_presence = (struct i_silc_presence *)presence;
			silc_presence->client_entry = chu->client;
		} else channel_connection_add_presence(&silc_chconn->chconn,
								presence);
		free(userhost);
	}
	silc_hash_table_list_reset(&htl);
	channel_connection_initial_presences_added(&silc_chconn->chconn);
}

static void event_joined(struct event *event)
{
	struct channel *channel = event_get_control(event, "channel");
	struct channel_connection *chconn = channel_get_connection(channel);
	struct i_silc_channel_connection *silc_chconn;
	struct i_silc_gateway_connection *silc_gwconn;
	SilcJoinResolve *r;

	i_assert(chconn != NULL);

	if( !IS_SILC_PROTOCOL(chconn->gwconn->gateway->network->protocol) )
		return;
	
	silc_chconn = (struct i_silc_channel_connection *)chconn;
	silc_gwconn = (struct i_silc_gateway_connection *)chconn->gwconn;

	r = silc_calloc(1, sizeof(*r));
	r->channel = silc_chconn->channel_entry;
	r->retry = 0;

	if( event_isset(event, "init") )
		silc_client_get_clients_by_channel(silc_gwconn->client,
				silc_gwconn->conn, silc_chconn->channel_entry,
				refresh_nicklist_resolved, r);
}

static struct event_bind_list events[] = {
	{ NULL, EVENT_CHANNEL_CONN_JOIN, event_joined },
	{ NULL, EVENT_CHANNEL_CONN_INIT, event_channel_connection_init },
	{ NULL, EVENT_CHANNEL_CONN_DEINIT, event_channel_connection_deinit },
	{ NULL, NULL, NULL }
};
