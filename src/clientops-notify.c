/*
 * Icecap_silc - a SILC module for Icecap
 * Copyright (C) 2005 Andrej Kacian
 *
 * - Function handler for SILC notify client operation
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
#include "channel-connection.h"
#include "channel-presence.h"
#include "presence.h"
#include "messages.h"

#include "clientops.h"
#include "support.h"
#include "silc-gateway-connection.h"
#include "silc-channel-connection.h"
#include "silc-client.h"
#include "silc-presence.h"
#include "silc.h"

void i_silc_operation_notify(SilcClient client,
		SilcClientConnection conn,
		SilcNotifyType type, ...)
{
	struct event *event;
	struct gateway_connection *gwconn;
	struct i_silc_gateway_connection *silc_gwconn;
	struct i_silc_channel_connection *silc_chconn;
	struct channel_connection *chconn;
	struct channel_presence *chpres;
	struct presence *presence = NULL;
	struct local_user *lu = client->application;

	char *str = NULL, *str2 = NULL, *set_type = NULL, *set_by = NULL;
	char *userhost = NULL, motd[2049];
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
			while ( (str = strtok(NULL, "\n")) ) {
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

			event = silc_server_event_new(lu, SILC_EVENT_NOTIFY_NONE);
			event_add(event, EVENT_KEY_NETWORK_NAME,
					gwconn->gateway->network->name);
			event_add(event, EVENT_KEY_LOCAL_PRESENCE_NAME,
					gwconn->local_presence->name);
			event_add(event, EVENT_KEY_MSG_TEXT, str);
			event_send(event);

			if( silc_gwconn->connected == FALSE ) {
				silc_client_command_call(silc_gwconn->client,
					silc_gwconn->conn, NULL, "NICK",
					gwconn->local_presence->name, NULL);
			}
			break;

		case SILC_NOTIFY_TYPE_INVITE:
			channel_entry = va_arg(va, SilcChannelEntry);
			str = va_arg(va, char *);
			client_entry = va_arg(va, SilcClientEntry);

			event = silc_server_event_new(lu, SILC_EVENT_NOTIFY_INVITE);
			event_add(event, EVENT_KEY_NETWORK_NAME,
					gwconn->gateway->network->name);
			event_add(event, EVENT_KEY_LOCAL_PRESENCE_NAME,
					gwconn->local_presence->name);
			event_add(event, EVENT_KEY_CHANNEL_NAME, str);
			event_add(event, "nickname", client_entry->nickname);
			event_add(event, "username", client_entry->username);
			event_add(event, "hostname", client_entry->hostname);
			event_send(event);
			break;

		case SILC_NOTIFY_TYPE_JOIN:
			client_entry = va_arg(va, SilcClientEntry);
			channel_entry = va_arg(va, SilcChannelEntry);
			
			event = silc_server_event_new(lu, SILC_EVENT_NOTIFY_JOIN);
			event_add(event, EVENT_KEY_NETWORK_NAME,
					gwconn->gateway->network->name);
			event_add(event, EVENT_KEY_LOCAL_PRESENCE_NAME,
					gwconn->local_presence->name);
			event_add(event, EVENT_KEY_CHANNEL_NAME,
					channel_entry->channel_name);
			event_add(event, "nick", client_entry->nickname);
			event_add(event, "address", userhost);
			event_send(event);

			if( !i_silc_client_id_is_me(silc_gwconn,
						client_entry->id) ) {
				silc_chconn =
					i_silc_channel_connection_lookup(
						silc_gwconn,
						channel_entry->channel_name);
				chconn = &silc_chconn->chconn;

				i_assert(chconn != NULL);

				/* Someone joined, let's add his presence */
				if( client_entry->username != NULL ||
				    client_entry->hostname != NULL) {
					userhost =
						i_silc_userhost(client_entry);
				}

				presence = presence_lookup(gwconn,
						client_entry->nickname);

				if( presence == NULL ) {
					presence = presence_init(gwconn,
							client_entry->nickname);
					presence_set_address(presence,
							userhost);
					presence->uncertain_address = FALSE;

					chpres =
					  channel_connection_presence_init(
							  chconn, presence);

					channel_connection_add_presence(chconn,
							chpres);
					presence_unref(presence);
				} else if( channel_connection_lookup_presence(
						chconn,
						presence->name) == NULL ) {
					if( userhost != NULL ) {
						presence->uncertain_address =
							FALSE;
						presence_set_address(presence,
								userhost);
					}

					chpres =
					  channel_connection_presence_init(
							  chconn, presence);

					channel_connection_add_presence(chconn,
							chpres);
				}
				free(userhost);
			} else {
				/* It's me */
			}
			break;

		case SILC_NOTIFY_TYPE_LEAVE:
			client_entry = va_arg(va, SilcClientEntry);
			channel_entry = va_arg(va, SilcChannelEntry);

			if( client_entry->nickname == NULL ||
			    client_entry->hostname == NULL ||
			    client_entry->username == NULL )
				break;

			userhost = i_silc_userhost(client_entry);

			event = silc_server_event_new(lu, SILC_EVENT_NOTIFY_LEAVE);
			event_add(event, EVENT_KEY_NETWORK_NAME,
					gwconn->gateway->network->name);
			event_add(event, EVENT_KEY_LOCAL_PRESENCE_NAME,
					gwconn->local_presence->name);
			event_add(event, EVENT_KEY_CHANNEL_NAME,
					channel_entry->channel_name);
			event_add(event, "nick", client_entry->nickname);
			event_add(event, "address", userhost);
			event_send(event);

			free(userhost);

			if( !i_silc_client_id_is_me(silc_gwconn,
						client_entry->id) ) {
				/* Someone else left */
				silc_chconn =
					i_silc_channel_connection_lookup(
						silc_gwconn,
						channel_entry->channel_name);
				chconn = &silc_chconn->chconn;
				chpres = channel_connection_lookup_presence(
						chconn,	client_entry->nickname);
				if( chpres != NULL )
					channel_connection_remove_presence(chconn, chpres, NULL, "");
			} else {
				/* It is me (shouldn't happen) */
			}
			break;

		case SILC_NOTIFY_TYPE_SIGNOFF:
			client_entry = va_arg(va, SilcClientEntry);
			str = va_arg(va, char *);

			if( client_entry->username != NULL &&
					client_entry->hostname != NULL )
				userhost = i_silc_userhost(client_entry);

			event = silc_server_event_new(lu, SILC_EVENT_NOTIFY_SIGNOFF);
			event_add(event, EVENT_KEY_NETWORK_NAME,
					gwconn->gateway->network->name);
			event_add(event, EVENT_KEY_LOCAL_PRESENCE_NAME,
					gwconn->local_presence->name);
			if( client_entry->nickname != NULL )
			event_add(event, "nick", client_entry->nickname ?
					client_entry->nickname : "unknown");
			event_add(event, "address", userhost);
			event_add(event, "reason", str ? str : "");
			event_send(event);

			free(userhost);

			if( client_entry && client_entry->nickname ) {
				presence = presence_lookup(gwconn,
						client_entry->nickname);
				if( presence == NULL ) {
					/* unknown, shouldn't happen */
					return;
				}

				channel_connections_remove_presence(gwconn, presence, (str ? str : ""));
			}
			break;

		case SILC_NOTIFY_TYPE_KICKED:
			kicked = va_arg(va, SilcClientEntry);
			str = va_arg(va, char *);
			kicker = va_arg(va, SilcClientEntry);
			channel_entry = va_arg(va, SilcChannelEntry);

			silc_chconn = i_silc_channel_connection_lookup(
					silc_gwconn,
					channel_entry->channel_name);
			if( silc_chconn == NULL ) {
				/* empty, shouldn't happen */
				return;
			}
			chconn = &silc_chconn->chconn;

			event = silc_server_event_new(lu, SILC_EVENT_NOTIFY_KICK);
			event_add(event, EVENT_KEY_NETWORK_NAME,
					gwconn->gateway->network->name);
			event_add(event, EVENT_KEY_LOCAL_PRESENCE_NAME,
					gwconn->local_presence->name);
			event_add(event, EVENT_KEY_CHANNEL_CONN_NAME,
					channel_entry->channel_name);
			event_add(event, "kicker", kicker->nickname);
			event_add(event, "target", kicked->nickname);
			event_add(event, EVENT_KEY_MSG_TEXT, (str ? str : ""));
			event_send(event);

			if( kicked == silc_gwconn->conn->local_entry ) { 
				/* we were kicked */
				channel_connection_deinit(chconn, str, TRUE);
			} else {
				chpres = channel_connection_lookup_presence(
						chconn, kicked->nickname);
				if( chpres != NULL )
					channel_connection_remove_presence(chconn, chpres, "kick", str);
			}
			break;

		case SILC_NOTIFY_TYPE_NICK_CHANGE:
			old = va_arg(va, SilcClientEntry);
			new = va_arg(va, SilcClientEntry);
			fprintf(stderr, "|%s| -> |%s|\n", old->nickname,
					new->nickname);

			if( !strcmp(old->nickname, new->nickname) )
				break;

			event = silc_server_event_new(lu, SILC_EVENT_NOTIFY_NICK_CHANGE);
			event_add(event, EVENT_KEY_NETWORK_NAME,
					gwconn->gateway->network->name);
			event_add(event, EVENT_KEY_LOCAL_PRESENCE_NAME,
					gwconn->local_presence->name);
			event_add(event, "name", old->nickname);
			event_add(event, "new_name", new->nickname);
			event_send(event);

			presence = presence_lookup(gwconn, old->nickname);
			if( presence == NULL ) {
				fprintf(stderr, "foo1\n");
				/* don't know, don't care */
				break;
			}
			if( presence_lookup(gwconn, new->nickname) != NULL ) {
				fprintf(stderr, "foo2\n");
				break; /* shouldn't happen, but ... */
			}
				
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

			event = silc_server_event_new(lu, SILC_EVENT_NOTIFY_TOPIC_SET);
			event_add(event, EVENT_KEY_NETWORK_NAME,
					gwconn->gateway->network->name);
			event_add(event, EVENT_KEY_LOCAL_PRESENCE_NAME,
					gwconn->local_presence->name);
			event_add(event, "set_type", set_type);
			event_add(event, "set_by", set_by);
			event_add(event, "channel",
					channel_entry2->channel_name);
			event_add(event, "topic", str);
			event_send(event);

			if( set_type != NULL )
				free(set_type);

			silc_chconn = i_silc_channel_connection_lookup_entry(
					silc_gwconn, channel_entry2);
			i_assert(silc_chconn != NULL);
			channel_connection_set_topic(&silc_chconn->chconn, str,
					presence, ioloop_time);
			break;		

		default:
			event = silc_server_event_new(lu, "unhandled");
			event_send(event);
			break;
	}
	va_end(va);
}
