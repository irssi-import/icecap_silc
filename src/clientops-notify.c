/* Copyright 2005 Andrej Kacian */

#include <silcincludes.h>
#include <silcclient.h>
#include <stdarg.h>

#include "lib.h"
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

	char *str;
	char userhost[256];
	SilcChannelEntry channel_entry;
	SilcClientEntry client_entry;

	va_list va;

	silc_gwconn = (struct i_silc_gateway_connection *)
		i_silc_gateway_connection_lookup_conn(conn);
	i_assert(silc_gwconn != NULL);
	gwconn = &silc_gwconn->gwconn;
	va_start(va, type);

	switch(type) {
		case SILC_NOTIFY_TYPE_MOTD:
			str = va_arg(va, char *);
	
			event = gwconn_get_event(gwconn, EVENT_GATEWAY_MOTD);
			event_add(event, "data", str);
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
			

			if( !SILC_ID_COMPARE(client_entry->id,
					silc_gwconn->conn->local_entry->id,
					sizeof(SilcClientID)) ) {
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
		default:
			event = silc_event_new(lu, "unhandled");
			event_send(event);
			break;
	}
	va_end(va);
}
