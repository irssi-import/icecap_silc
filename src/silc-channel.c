/* Copyright 2005 Andrej Kacian */

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
#include "silc-channel.h"
#include "silc-gateway-connection.h"
#include "silc-presence.h"

static struct event_bind_list events[];

void i_silc_channel_events_init(void)
{
	event_bind_list(events, 0);
}

void i_silc_channel_events_deinit(void)
{
	event_unbind_list(events);
}

struct channel *
i_silc_channel_init(struct gateway_connection *gwconn __attr_unused__,
		 const char *name)
{
	struct i_silc_channel *silc_channel = i_new(struct i_silc_channel, 1);

	silc_channel->channel.presences =
		tree_create(default_pool, (tree_cmp_callback_t *)strcasecmp);
	silc_channel->name = i_strdup(name);

	return &silc_channel->channel;
}

void i_silc_channel_deinit(struct channel *channel)
{
	struct i_silc_channel *silc_channel =
		(struct i_silc_channel *)channel;
	i_free(silc_channel->name);
	i_free(silc_channel);
}

void i_silc_join_send(struct channel *channel, struct event *event
		__attr_unused__ )
{
	SilcBuffer idp;
	struct i_silc_gateway_connection *silc_gwconn =
		(struct i_silc_gateway_connection *)channel->gwconn;
	const char *channel_str = channel->name;

	idp = silc_id_payload_encode(silc_gwconn->conn->local_id,
			SILC_ID_CLIENT);

	i_assert(idp != NULL);

	silc_client_command_send(silc_gwconn->client, silc_gwconn->conn,
			SILC_COMMAND_JOIN, 0, 2, 1, channel_str,
			strlen(channel_str), 2, idp->data, idp->len);
}

void i_silc_part_send(struct channel *channel, struct event *event
		__attr_unused__ )
{
	struct i_silc_gateway_connection *silc_gwconn =
		(struct i_silc_gateway_connection *)channel->gwconn;
	const char *channel_str = channel->name;

	silc_client_command_call(silc_gwconn->client, silc_gwconn->conn, NULL,
						"LEAVE", channel_str, NULL);
}

struct i_silc_channel *
i_silc_channel_lookup_entry(struct i_silc_gateway_connection *silc_gwconn,
		SilcChannelEntry entry)
{
	struct channel *const *c;
	unsigned int i, count;
	struct i_silc_channel *silc_channel;
	struct gateway_connection *gwconn = &silc_gwconn->gwconn;

	c = array_get(&gwconn->channels, &count);
	for( i = 0; i < count; i++ ) {
		silc_channel = (struct i_silc_channel *)c[i];
		i_assert(silc_channel != NULL);
		i_assert(silc_channel->channel_entry != NULL);
		if( silc_channel->channel_entry == entry )
			return silc_channel;
	}

	return NULL;
}

struct i_silc_channel *
i_silc_channel_lookup(struct i_silc_gateway_connection *silc_gwconn,
		const char *name) {
	struct channel *const *c;
	unsigned int i, count;
	struct gateway_connection *gwconn = &silc_gwconn->gwconn;

	i_assert(gwconn != NULL);
	i_assert(name != NULL);

	c = array_get(&gwconn->channels, &count);
	for( i = 0; i < count; i++ ) {
		if( strcasecmp(c[i]->name, name) == 0 )
			return (struct i_silc_channel *)c[i];
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
	struct i_silc_channel *silc_channel;
	int usercount = 0;
	char *userhost;

	silc_gwconn = (struct i_silc_gateway_connection *)
		i_silc_gateway_connection_lookup_conn(conn);
	i_assert(silc_gwconn != NULL);
	silc_channel = i_silc_channel_lookup_entry(silc_gwconn, channel_entry);
	i_assert(silc_channel != NULL);

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
			channel_add_presence(&silc_channel->channel, presence);
			presence_unref(presence);
			silc_presence = (struct i_silc_presence *)presence;
			silc_presence->client_entry = chu->client;
		} else channel_add_presence(&silc_channel->channel, presence);
		free(userhost);
	}
	silc_hash_table_list_reset(&htl);
	channel_initial_presences_added(&silc_channel->channel);
}

static void event_joined(struct event *event)
{
	struct channel *channel = event_get_control(event, "channel");
	struct i_silc_channel *silc_channel;
	struct i_silc_gateway_connection *silc_gwconn;
	SilcJoinResolve *r;

	i_assert(channel != NULL);

	if( !IS_SILC_PROTOCOL(channel->gwconn->gateway->network->protocol) )
		return;
	
	silc_channel = (struct i_silc_channel *)channel;
	silc_gwconn = (struct i_silc_gateway_connection *)channel->gwconn;

	r = silc_calloc(1, sizeof(*r));
	r->channel = silc_channel->channel_entry;
	r->retry = 0;

	if( event_isset(event, "init") )
		silc_client_get_clients_by_channel(silc_gwconn->client,
				silc_gwconn->conn, silc_channel->channel_entry,
				refresh_nicklist_resolved, r);
}

static struct event_bind_list events[] = {
	{ NULL, EVENT_CHANNEL_JOIN, event_joined },
	{ NULL, NULL, NULL }
};
