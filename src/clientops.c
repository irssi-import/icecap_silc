/*
 * Icecap_silc - a SILC module for Icecap
 * Copyright (C) 2005 Andrej Kacian
 *
 * - Function handlers for SILC client operations (except for notify clientop)
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

#include <stdarg.h>

#include <lib/lib.h>
#include <lib/ioloop.h>
#include <server/chat-protocol.h>
#include <server/server-event.h>
#include <server/local-user.h>
#include <server/local-presence.h>
#include <server/gateway-connection.h>
#include <server/network.h>
#include <server/gateway.h>
#include <server/channel-connection.h>
#include <server/messages.h>
#include <server/presence.h>

#include <silc.h>
#include <silcclient.h>

#include "clientops.h"
#include "support.h"
#include "icecap-silc.h"
#include "silc-gateway.h"
#include "silc-gateway-connection.h"
#include "silc-channel-connection.h"
#include "silc-client.h"
#include "silc-presence.h"

typedef struct {
	SilcGetAuthMeth completion;
	void *context;
} *InternalGetAuthMethod;

SilcClientOperations ops = {
    i_silc_operation_say,
    i_silc_operation_channel_message,
    i_silc_operation_private_message,
    i_silc_operation_notify,
    i_silc_operation_command,
    i_silc_operation_command_reply,
    i_silc_operation_get_auth_method,
    i_silc_operation_verify_public_key,
    i_silc_operation_ask_passphrase,
    i_silc_operation_key_agreement,
    i_silc_operation_ftp
};

void i_silc_operation_say(SilcClient client, SilcClientConnection conn,
		SilcClientMessageType type, char *msg, ...)
{
	struct event *event;
	char str[256];
	va_list va;
	struct local_user *lu = client->application;

	event = silc_server_event_new(lu, SILC_EVENT_SERVER_SAY);

	va_start(va, msg);

	vsnprintf(str, sizeof(str) - 1, msg, va);
	event_add(event, EVENT_KEY_MSG_TEXT, str);

	va_end(va);

	event_send(event);
}

void i_silc_operation_channel_message(SilcClient client,
		SilcClientConnection conn, SilcClientEntry sender,
		SilcChannelEntry channel, SilcMessagePayload payload,
		SilcChannelPrivateKey key, SilcMessageFlags flags,
		const unsigned char *message, SilcUInt32 message_len)
{
	char content_type[128];
	char transfer_encoding[128];
	unsigned char *mime_data_buffer;
	SilcUInt32 mime_data_len;
	bool valid_mime;
	unsigned int header_length, sgn;
	char header_length_str[16];
	char *userhost = i_silc_userhost(sender);
/*	bool error; */
	struct event *event;
	struct gateway_connection *gwconn =
		i_silc_gateway_connection_lookup_conn(conn);
	struct i_silc_gateway_connection *silc_gwconn =
		(struct i_silc_gateway_connection *)gwconn;
	struct i_silc_channel_connection *silc_chconn =
		i_silc_channel_connection_lookup_entry(silc_gwconn, channel);
	struct channel_connection *ichconn = &silc_chconn->chconn;
	struct local_user *lu = client->application;

	event = server_event_new(lu, EVENT_MSG);

	event_add_control(event, EVENT_CONTROL_GWCONN, ichconn->gwconn);
	event_add(event, EVENT_KEY_PRESENCE_NAME, sender->nickname);
	event_add(event, "address", userhost);
	i_free(userhost);
	event_add(event, EVENT_KEY_NETWORK_NAME,
			ichconn->gwconn->gateway->network->name);
	event_add(event, EVENT_KEY_LOCAL_PRESENCE_NAME,
			ichconn->gwconn->local_presence->name);
	event_add(event, EVENT_KEY_CHANNEL_CONN_NAME,
			ichconn->channel->name);

	memset(content_type, 0, sizeof(content_type));
	memset(transfer_encoding, 0, sizeof(transfer_encoding));

	valid_mime = silc_mime_parse(message, message_len,
			NULL, 0, content_type, sizeof(content_type) - 1,
			transfer_encoding, sizeof(transfer_encoding) - 1,
			&mime_data_buffer, &mime_data_len);

	if( valid_mime == TRUE ) {
		header_length = message_len - mime_data_len;
		sprintf(header_length_str, "%d", header_length);
		event_add(event, SILC_EVENT_KEY_CONTENT_TYPE, content_type);
		event_add(event, SILC_EVENT_KEY_TRANSFER_ENCODING,
				transfer_encoding);
		event_add(event, SILC_EVENT_KEY_HEADER_LENGTH,
				header_length_str);
	} else {
		event_add(event, EVENT_KEY_MSG_TEXT, message);
	}

	if( flags & SILC_MESSAGE_FLAG_ACTION )
		event_add(event, "type", "action");

	if( flags & SILC_MESSAGE_FLAG_SIGNED ) {
		SilcMessageSignedPayload sig =
			silc_message_get_signature(payload);
		sgn = verify_message_signature(sender, sig, payload);
		switch(sgn) {
			case 1:
				event_add(event, SILC_EVENT_KEY_SIGNATURE,
						SILC_SIGSTATUS_VALID);
				break;
			case 0:
				event_add(event, SILC_EVENT_KEY_SIGNATURE,
						SILC_SIGSTATUS_INVALID);
				break;
			case -1:
				event_add(event, SILC_EVENT_KEY_SIGNATURE,
						SILC_SIGSTATUS_DUNNO);
				break;
		}
	}

	event_send(event);
}

void i_silc_operation_private_message(SilcClient client,
		SilcClientConnection conn, SilcClientEntry sender,
		SilcMessagePayload payload, SilcMessageFlags flags,
		const unsigned char *message, SilcUInt32 message_len)
{
	struct local_user *lu = client->application;
	struct event *event;
	struct gateway_connection *gwconn =
					i_silc_gateway_connection_lookup_conn(conn);
	char *userhost = i_silc_userhost(sender);
	char content_type[128];
	char transfer_encoding[128];
	unsigned char *mime_data_buffer;
	SilcUInt32 mime_data_len;
	bool valid_mime;
	unsigned int header_length, sgn;
	char header_length_str[16];

	event = server_event_new(lu, EVENT_MSG);

	event_add_control(event, EVENT_CONTROL_GWCONN, gwconn);
	event_add(event, EVENT_KEY_PRESENCE_NAME, sender->nickname);
	event_add(event, "address", userhost);
	i_free(userhost);
	event_add(event, EVENT_KEY_NETWORK_NAME, gwconn->gateway->network->name);
	event_add(event, EVENT_KEY_LOCAL_PRESENCE_NAME,
									gwconn->local_presence->name);

	memset(content_type, 0, sizeof(content_type));
	memset(transfer_encoding, 0, sizeof(transfer_encoding));

	valid_mime = silc_mime_parse(message, message_len, NULL, 0,
									content_type, sizeof(content_type) - 1,
									transfer_encoding, sizeof(transfer_encoding) - 1,
									&mime_data_buffer, &mime_data_len);

	if( valid_mime == TRUE ) {
		header_length = message_len - mime_data_len;
		sprintf(header_length_str, "%d", header_length);
		event_add(event, SILC_EVENT_KEY_CONTENT_TYPE, content_type);
		event_add(event, SILC_EVENT_KEY_TRANSFER_ENCODING, transfer_encoding);
		event_add(event, SILC_EVENT_KEY_HEADER_LENGTH, header_length_str);
	} else {
		event_add(event, EVENT_KEY_MSG_TEXT, message);
	}

	if( flags & SILC_MESSAGE_FLAG_ACTION )
		event_add(event, "type", "action");

	if( flags & SILC_MESSAGE_FLAG_SIGNED ) {
		SilcMessageSignedPayload sig = silc_message_get_signature(payload);
		sgn = verify_message_signature(sender, sig, payload);
		switch(sgn) {
			case 1:
				event_add(event, SILC_EVENT_KEY_SIGNATURE, SILC_SIGSTATUS_VALID);
				break;
			case 0:
				event_add(event, SILC_EVENT_KEY_SIGNATURE, SILC_SIGSTATUS_INVALID);
				break;
			case -1:
				event_add(event, SILC_EVENT_KEY_SIGNATURE, SILC_SIGSTATUS_DUNNO);
				break;
		}
	}

	event_send(event);
}

void i_silc_operation_command(SilcClient client, SilcClientConnection conn,
		SilcBool success, SilcCommand command, SilcStatus status,
		SilcUInt32 argc unsigned char **argv)
{
}

void i_silc_operation_command_reply(SilcClient client,
		SilcClientConnection conn, SilcCommand command,
		SilcStatus status, SilcStatus error, va_list va)
{
	struct channel_connection *chconn;
	struct i_silc_channel_connection *silc_chconn;
	struct event *event;
	struct i_silc_gateway_connection *silc_gwconn =
		(struct i_silc_gateway_connection *)
		i_silc_gateway_connection_lookup_conn(conn);
	struct gateway_connection *gwconn = &silc_gwconn->gwconn;
	SilcChannelEntry channel_entry;
	SilcClientEntry client_entry;
	SilcClientID *client_id;
	struct presence *presence;
	char *channel_name, *new_nick;
	struct local_user *lu = client->application;

	switch(command) {
		case SILC_COMMAND_JOIN:
			channel_name = va_arg(va, char *);
			channel_entry = va_arg(va, SilcChannelEntry);
			silc_chconn =
				i_silc_channel_connection_lookup(silc_gwconn,
						channel_entry->channel_name);
			chconn = &silc_chconn->chconn;

			i_assert(chconn != NULL);

			silc_chconn =
				(struct i_silc_channel_connection *)chconn;
			silc_chconn->channel_entry = channel_entry;
			if( success ) {
				channel_connection_set_joined(chconn);
				channel_connection_set_topic(chconn,
					channel_entry->topic, NULL,
					ioloop_time);
			} else {
				if( !chconn->joined )
					channel_connection_deinit(chconn, NULL,
							TRUE);
			}
			break;
		case SILC_COMMAND_LEAVE:
			break;
		case SILC_COMMAND_NICK:
			client_entry = va_arg(va, SilcClientEntry);
			new_nick = va_arg(va, char *);
			client_id = va_arg(va, SilcClientID *);

			if( !i_silc_client_id_is_me(silc_gwconn, client_id) ) {
				if( silc_gwconn->connected == FALSE ) {
					gateway_connection_set_logged_in(gwconn);
					silc_gwconn->connected = TRUE;
				}
				return;
			}

			presence = local_presence_get_presence(gwconn->local_presence);
			if( strcmp(presence->name, new_nick) != 0 )
				presence_set_name(presence, new_nick);
			break;
		case SILC_COMMAND_WHOIS:
			event = silc_server_event_new(lu, "whoisreply");
			event_send(event);
			break;
	}
}

void i_silc_operation_connected(SilcClient client, SilcClientConnection conn,
		SilcClientConnectionStatus status)
{
	struct gateway_connection *gwconn = NULL;
	struct i_silc_gateway_connection *silc_gwconn = NULL;

	silc_gwconn =
		(struct i_silc_gateway_connection *)
		i_silc_gateway_connection_lookup_conn(conn);
	gwconn = &silc_gwconn->gwconn;

	silc_gwconn->connection_status = status;
	switch(status) {
		case SILC_CLIENT_CONN_SUCCESS:
		case SILC_CLIENT_CONN_SUCCESS_RESUME:
			break;
		case SILC_CLIENT_CONN_ERROR:
			i_silc_client_close_connection(silc_gwconn);
			break;
		case SILC_CLIENT_CONN_ERROR_KE:
			i_silc_client_close_connection(silc_gwconn);
			break;
		case SILC_CLIENT_CONN_ERROR_AUTH:
			i_silc_client_close_connection(silc_gwconn);
			break;
		case SILC_CLIENT_CONN_ERROR_RESUME:
			i_silc_client_close_connection(silc_gwconn);
			break;
		case SILC_CLIENT_CONN_ERROR_TIMEOUT:
			i_silc_client_close_connection(silc_gwconn);
			break;
		default:
			i_silc_client_close_connection(silc_gwconn);
			break;
	}
}

void i_silc_operation_get_auth_method(SilcClient client,
		SilcClientConnection conn, char *hostname, SilcUInt16 port,
		SilcAuthMethod auth_method, SilcGetAuthMeth completion, void *context)
{
	InternalGetAuthMethod internal;

	internal = silc_calloc(1, sizeof(*internal));
	internal->completion = completion;
	internal->context = context;

	silc_client_request_authentication_method(client, conn,
			i_silc_get_auth_method_callback, internal);
}

void i_silc_operation_verify_public_key(SilcClient client,
		SilcClientConnection conn, SilcConnectionType conn_type,
		SilcPublicKey public_key, SilcVerifyPublicKey completion, void *context)
{
	SilcPublicKey pkey;
	bool ret;

	ret = silc_pkcs_public_key_decode(pk, pk_len, &pkey);
	if( ret != TRUE ) { /* not a SILC key */
		completion(FALSE, context);
		return;
	}

	completion(TRUE, context);
	return;
}

void i_silc_operation_ask_passphrase(SilcClient client,
		SilcClientConnection conn, SilcAskPassphrase completion,
		void *context)
{
	printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> ask_passphrase\n");
}

void i_silc_operation_failure(SilcClient client, SilcClientConnection conn,
		SilcProtocol protocol, void *failure)
{
	printf(">>>>>>>>>>>>>>>>>>>>>>>>> operation_failure\n");
}

bool i_silc_operation_key_agreement(SilcClient client,
		SilcClientConnection conn, SilcClientEntry client_entry,
		const char *hostname, SilcUInt16 protocol, SilcUInt16 port)
{
	printf(">>>>>>>>>>>>>>>>>>>>>>>>> key_agreement\n");
	return FALSE;
}

void i_silc_operation_ftp(SilcClient client, SilcClientConnection conn,
		SilcClientEntry client_entry, SilcUInt32 session_id,
		const char *hostname, SilcUInt16 port)
{
}

void i_silc_operation_detach(SilcClient client, SilcClientConnection conn,
		const unsigned char *detach_data,
		SilcUInt32 detach_data_len)
{
}

void i_silc_get_auth_method_callback(SilcClient client,
		SilcClientConnection conn, SilcAuthMethod auth_meth,
		void *context)
{
	InternalGetAuthMethod internal = (InternalGetAuthMethod)context;
	struct gateway_connection *gwconn =
					i_silc_gateway_connection_lookup_conn(conn);
	struct i_silc_gateway *silc_gw =
					(struct i_silc_gateway *)gwconn->gateway;
	char *password = NULL;

	switch(auth_meth) {
		case SILC_AUTH_NONE:
			(*internal->completion)(TRUE, auth_meth, NULL, 0,
						internal->context);
			break;
		case SILC_AUTH_PUBLIC_KEY:
			(*internal->completion)(TRUE, auth_meth, NULL, 0,
						internal->context);
			break;
		case SILC_AUTH_PASSWORD:
			password = silc_gw->server_password;
			if( password )
				(*internal->completion)(TRUE, auth_meth,
													password,
													strlen(password),
													internal->context);
			else
				(*internal->completion)(TRUE, auth_meth, NULL, 0,
						internal->context);

			break;
	}

	silc_free(internal);
}
