/* Copyright 2005 Andrej Kacian */

#include "lib.h"
#include "event.h"
#include "chat-protocol.h"
#include "local-user.h"
#include "client-commands.h"
#include "messages.h"

#include "silc-message.h"
#include "silc-gateway-connection.h"
#include "silc-channel.h"

void i_silc_message_send(struct gateway_connection *gwconn, struct event *event)
{
	/* FIXME: stub! */
	struct i_silc_gateway_connection *silc_gwconn =
		(struct i_silc_gateway_connection *)gwconn;
	struct network *network;
	struct presence *presence;
	const char *msg = event_get(event, EVENT_MSG);
	const char *channel = event_get(event, "channel");
	const char *target = event_get(event, "target");
	struct i_silc_channel *silc_channel =
		i_silc_channel_lookup(silc_gwconn, channel);
	SilcMessageFlags sendflags = SILC_MESSAGE_FLAG_UTF8;

	if( *target == '\0' && *channel == '\0' ) {
		client_command_error(event, CLIENT_CMDERR_ARGS);
		return;
	}

	if( silc_channel == NULL ) {
		client_command_error(event, CLIENT_CMDERR_NOT_FOUND);
		return;
	}	

	sendflags |= SILC_MESSAGE_FLAG_SIGNED;

	if( !client_command_get_network_presence(event, &network, &presence) )
		return;

	silc_client_send_channel_message(silc_gwconn->client,
				silc_gwconn->conn, silc_channel->channel_entry,
				NULL, sendflags, (unsigned char *)msg,
				strlen(msg), FALSE);
}
