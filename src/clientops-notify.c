/* Copyright 2005 Andrej Kacian */

#include <silcincludes.h>
#include <silcclient.h>
#include <stdarg.h>

#include "lib.h"
#include "ioloop.h"
#include "chat-protocol.h"
#include "event.h"
#include "local-user.h"
#include "local-presence.h"
#include "gateway-connection.h"
#include "network.h"
#include "gateway.h"
#include "channel.h"
#include "presence.h"

#include "clientops.h"
#include "silc-gateway-connection.h"
#include "silc-channel.h"
#include "silc-client.h"
#include "silc.h"

void i_silc_operation_notify(SilcClient client __attr_unused__,
		SilcClientConnection conn,
		SilcNotifyType type, ...)
{
	struct chat_protocol *proto = chat_protocol_lookup("SILC");
	struct local_user *lu = proto->local_user;
	struct event *event;
	struct gateway_connection *gwconn;
	struct i_silc_gateway_connection *silc_gwconn;
	struct i_silc_channel *silc_channel;
	struct channel *channel;
	struct presence *presence = NULL;

	char *str = NULL, *str2 = NULL, *set_type = NULL, *set_by = NULL;
	char userhost[256], motd[2049];
	SilcChannelEntry channel_entry, channel_entry2;
	SilcClientEntry client_entry, kicked, kicker, old, new;
	SilcServerEntry server_entry;
	SilcIdType id_type;

	va_list va;

	silc_gwconn = (struct i_silc_gateway_connection *)
		i_silc_gateway_connection_lookup_conn(conn);
	i_assert(silc_gwconn != NULL);
	gwconn = &silc_gwconn->gwconn;
	va_start(va, type);

	switch(type) {
		case SILC_NOTIFY_TYPE_MOTD:
			str2 = va_arg(va, char *);
			
			memcpy(motd, "0", 2048);
			strncat(motd, str2, 2048);

			/* send each line of motd separately */
			str = strtok(motd, "\n");
			event = gwconn_get_event(gwconn, EVENT_GATEWAY_MOTD);
			event_add(event, "data", str);
			event_send(event);
			while ( str = strtok(NULL, "\n") ) {
				event = gwconn_get_event(gwconn,
						EVENT_GATEWAY_MOTD);
				event_add(event, "data", str);
				event_send(event);
			}

			/* send gateway_motd_end event */
			event = gwconn_get_event(gwconn,
					EVENT_GATEWAY_MOTD_END);
			event_send(event);

			break;

		case SILC_NOTIFY_TYPE_NONE:
			str = va_arg(va, char *);

			event = silc_event_new(lu, SILC_EVENT_NOTIFY_NONE);
			event_add(event, "network",
					gwconn->gateway->network->name);
			event_add(event, "presence",
					gwconn->local_presence->name);
			event_add(event, "msg", str);
			event_send(event);
			break;

		case SILC_NOTIFY_TYPE_INVITE:
			channel_entry = va_arg(va, SilcChannelEntry);
			str = va_arg(va, char *);
			client_entry = va_arg(va, SilcClientEntry);

			event = silc_event_new(lu, SILC_EVENT_NOTIFY_INVITE);
			event_add(event, "network",
					gwconn->gateway->network->name);
			event_add(event, "presence",
					gwconn->local_presence->name);
			event_add(event, "channel", str);
			event_add(event, "nickname", client_entry->nickname);
			event_add(event, "username", client_entry->username);
			event_add(event, "hostname", client_entry->hostname);
			event_send(event);
			break;

		case SILC_NOTIFY_TYPE_JOIN:
			client_entry = va_arg(va, SilcClientEntry);
			channel_entry = va_arg(va, SilcChannelEntry);
			
			if( !i_silc_client_id_is_me(silc_gwconn,
						client_entry->id) ) {
				silc_channel =
					i_silc_channel_lookup(silc_gwconn,
						channel_entry->channel_name);
				channel = &silc_channel->channel;

				i_assert(channel != NULL);

				/* Someone joined, let's add his presence */
				if( client_entry->username != NULL ||
				    client_entry->hostname != NULL) {
					snprintf(userhost, 255, "%s@%s",
						client_entry->username,
						client_entry->hostname);
				}

				event = silc_event_new(lu,
						SILC_EVENT_NOTIFY_JOIN);
				event_add(event, "nick",
						client_entry->nickname);
				event_add(event, "address", userhost);
				event_send(event);

				presence = presence_lookup(gwconn,
						client_entry->nickname);

				if( presence == NULL ) {
					presence = presence_init(gwconn,
							client_entry->nickname);
					presence->uncertain_address = FALSE;
					presence_set_address(presence,
							userhost);

					channel_add_presence(channel, presence);
					presence_unref(presence);
				} else if( channel_lookup_presence(channel,
						presence->name) == NULL ) {
					if( userhost != NULL ) {
						presence->uncertain_address =
							FALSE;
						presence_set_address(presence,
								userhost);
					}
					channel_add_presence(channel, presence);
				}
			} else {
				/* It's me */
			}
			break;

		case SILC_NOTIFY_TYPE_LEAVE:
			client_entry = va_arg(va, SilcClientEntry);
			channel_entry = va_arg(va, SilcChannelEntry);
			snprintf(userhost, 255, "%s@%s", client_entry->username,
					client_entry->hostname);

			event = silc_event_new(lu, SILC_EVENT_NOTIFY_LEAVE);
			event_add(event, "channel",
					channel_entry->channel_name);
			event_add(event, "nick", client_entry->nickname);
			event_add(event, "address", userhost);
			event_send(event);

			if( !SILC_ID_COMPARE(client_entry->id,
					silc_gwconn->conn->local_entry->id,
					sizeof(SilcClientID)) ) {
				/* Someone else left */
				silc_channel =
					i_silc_channel_lookup(silc_gwconn,
						channel_entry->channel_name);
				channel = &silc_channel->channel;
				presence = channel_lookup_presence(channel,
						client_entry->nickname);
				if( presence != NULL )
					channel_remove_presence(channel,
							presence, "");
			} else {
				/* It is me (shouldn't happen) */
			}
			break;

		case SILC_NOTIFY_TYPE_SIGNOFF:
			client_entry = va_arg(va, SilcClientEntry);
			str = va_arg(va, char *);

			if( client_entry->username != NULL &&
					client_entry->hostname != NULL )
				snprintf(userhost, 255, "%s@%s",
						client_entry->username,
						client_entry->hostname);
			else strncat(userhost, "unknown@unknown", 255);

			event = silc_event_new(lu, SILC_EVENT_NOTIFY_SIGNOFF);
			if( client_entry->nickname != NULL )
			event_add(event, "nick", client_entry->nickname ?
					client_entry->nickname : "unknown");
			event_add(event, "address", userhost);
			event_add(event, "reason", str ? str : "");
			event_send(event);

			if( client_entry && client_entry->nickname ) {
				presence = presence_lookup(gwconn,
						client_entry->nickname);
				if( presence == NULL ) {
					/* unknown, shouldn't happen */
					return;
				}

				channels_remove_presence(gwconn, presence,
						str ? str : "");
			}
			break;

		case SILC_NOTIFY_TYPE_KICKED:
			kicked = va_arg(va, SilcClientEntry);
			str = va_arg(va, char *);
			kicker = va_arg(va, SilcClientEntry);
			channel_entry = va_arg(va, SilcChannelEntry);

			silc_channel = i_silc_channel_lookup(silc_gwconn,
					channel_entry->channel_name);
			if( silc_channel == NULL ) {
				/* empty, shouldn't happen */
				return;
			}
			channel = &silc_channel->channel;

			event = silc_event_new(lu,
					SILC_EVENT_NOTIFY_KICK);
			event_add(event, "channel",
					channel_entry->channel_name);
			event_add(event, "kicker", kicker->nickname);
			event_add(event, "target", kicked->nickname);
			event_add(event, "msg", (str ? str : ""));
			event_send(event);

			if( kicked == silc_gwconn->conn->local_entry ) { 
				/* we were kicked */
				channel_deinit(channel, str);
			} else {
				presence = channel_lookup_presence(channel,
						kicked->nickname);
				if( presence != NULL )
					channel_remove_presence(channel,
							presence, str);
			}
			break;

		case SILC_NOTIFY_TYPE_NICK_CHANGE:
			old = va_arg(va, SilcClientEntry);
			new = va_arg(va, SilcClientEntry);

			event = silc_event_new(lu,
					SILC_EVENT_NOTIFY_NICK_CHANGE);
			event_add(event, "name", old->nickname);
			event_add(event, "new_name", new->nickname);
			event_send(event);

			presence = presence_lookup(gwconn, old->nickname);
			if( presence == NULL ) {
				/* don't know, don't care */
				return;
			}
			if( presence_lookup(gwconn, new->nickname) != NULL )
				return; /* shouldn't happen, but ... */
				
			presence_set_name(presence, new->nickname);
			break;

		case SILC_NOTIFY_TYPE_TOPIC_SET:
			presence = NULL;
			id_type = va_arg(va, int);
			switch( id_type ) {
				case SILC_ID_SERVER:
					server_entry = va_arg(va,
							SilcServerEntry);
					set_type = strdup("server");
					set_by = server_entry->server_name;
					break;
				case SILC_ID_CHANNEL:
					channel_entry = va_arg(va,
							SilcChannelEntry);
					set_type = strdup("channel");
					set_by = channel_entry->channel_name;
					break;
				case SILC_ID_CLIENT:
					client_entry = va_arg(va,
							SilcClientEntry);
					set_type = strdup("client");
					set_by = client_entry->nickname;
					presence = presence_lookup(gwconn,
							client_entry->nickname);
					break;
			}

			str = va_arg(va, char *);
			channel_entry2 = va_arg(va, SilcChannelEntry);

			event = silc_event_new(lu, SILC_EVENT_NOTIFY_TOPIC_SET);
			event_add(event, "set_type", set_type);
			event_add(event, "set_by", set_by);
			event_add(event, "channel",
					channel_entry2->channel_name);
			event_add(event, "topic", str);
			event_send(event);

			if( set_type != NULL )
				free(set_type);

			silc_channel = i_silc_channel_lookup_entry(silc_gwconn,
					channel_entry2);
			i_assert(silc_channel != NULL);
			channel_set_topic(&silc_channel->channel, str,
					presence, ioloop_time);
			break;		

		default:
			event = silc_event_new(lu, "unhandled");
			event_send(event);
			break;
	}
	va_end(va);
}
