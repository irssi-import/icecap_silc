/*
 * Irssi2_silc - a SILC module for Irssi2
 * Copyright (C) 2005 Andrej Kacian
 *
 * - Functions for handling SILC messages
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
#include "event.h"
#include "chat-protocol.h"
#include "local-user.h"
#include "client-commands.h"
#include "messages.h"

#include "silc-message.h"
#include "silc-gateway-connection.h"
#include "silc-channel-connection.h"

void i_silc_message_send(struct gateway_connection *gwconn, struct event *event)
{
	struct i_silc_gateway_connection *silc_gwconn =
		(struct i_silc_gateway_connection *)gwconn;
	struct network *network;
	struct presence *presence;
	const char *msg = event_get(event, EVENT_MSG);
	const char *channel = event_get(event, EVENT_KEY_CHANNEL_NAME);
	const char *target = event_get(event, "presence");
	const char *type = event_get(event, "type");
	struct i_silc_channel_connection *silc_chconn;
	SilcMessageFlags sendflags = SILC_MESSAGE_FLAG_UTF8;
	SilcClientEntry target_entry = NULL;
	bool success;

	i_assert(channel != NULL);

	if( *target == '\0' && *channel == '\0' ) {
		client_command_error(event, CLIENT_CMDERR_ARGS);
		return;
	}

	silc_chconn = i_silc_channel_connection_lookup(silc_gwconn, channel);

	if( silc_chconn == NULL ) {
		client_command_error(event, CLIENT_CMDERR_NOT_FOUND);
		return;
	}	

	sendflags |= SILC_MESSAGE_FLAG_SIGNED;

	if( !strcmp(type, "action") )
		sendflags |= SILC_MESSAGE_FLAG_ACTION;

	if( !client_command_get_network_presence(event, &network, &presence) )
		return;

	/* At this point we know that either channel or target are given to us,
	 * so we can do this. Channel takes priority if both are specified. */
	if( *channel != '\0' ) {
		success = silc_client_send_channel_message(silc_gwconn->client,
										silc_gwconn->conn, silc_chconn->channel_entry,
										silc_chconn->channel_entry->curr_key, sendflags,
										(unsigned char *)msg,	strlen(msg), FALSE);
	} else {
		silc_client_get_clients_whois(silc_gwconn->client, silc_gwconn->conn,
										target, NULL, NULL, (SilcGetClientCallback)i_silc_privmsg_whois_callback,
										(void *)target_entry);
		if( target_entry == NULL ) {
			/* Couldn't find such target, error to client and return */
			client_command_error(event, CLIENT_CMDERR_NOT_FOUND);
			return;
		}

		success = silc_client_send_private_message(silc_gwconn->client,
										silc_gwconn->conn, target_entry, sendflags,
										(unsigned char *)msg, strlen(msg), FALSE);
	}

	if( !success )
		client_command_error(event, CLIENT_CMDERR_UNKNOWN);
}

/* Return first found entry */
void *i_silc_privmsg_whois_callback(SilcClient client,
				SilcClientConnection conn, SilcClientEntry *clients, SilcUInt32 count,
				void *target_entry)
{
	if( clients == NULL ) {
		target_entry = NULL;
		return NULL;
	}

	target_entry = clients[0];
	return NULL;
}
	
