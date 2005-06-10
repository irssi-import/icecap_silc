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
#include "messages.h"

#include "clientops.h"
#include "silc-gateway-connection.h"
#include "silc-channel.h"
#include "silc.h"

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
    i_silc_operation_connected,
    i_silc_operation_disconnected,
    i_silc_operation_get_auth_method,
    i_silc_operation_verify_public_key,
    i_silc_operation_ask_passphrase,
    i_silc_operation_failure,
    i_silc_operation_key_agreement,
    i_silc_operation_ftp,
    i_silc_operation_detach
};

void i_silc_operation_say(SilcClient client, SilcClientConnection conn,
		SilcClientMessageType type, char *msg, ...)
{
	struct event *event;
	char str[256];
	va_list va;
	struct chat_protocol *proto = chat_protocol_lookup("SILC");
	struct local_user *lu = proto->local_user;

	event = silc_event_new(lu, SILC_EVENT_SERVER_SAY);

	va_start(va, msg);

	vsnprintf(str, sizeof(str) - 1, msg, va);
	event_add(event, "msg", str);

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
	char userhost[256];
	unsigned char *mime_data_buffer;
	SilcUInt32 mime_data_len;
	bool valid_mime;
	unsigned int header_length, sgn;
	char header_length_str[16];
/*	bool error; */
	struct event *event;
	struct chat_protocol *proto = chat_protocol_lookup("SILC");
	struct local_user *lu = proto->local_user;
	struct gateway_connection *gwconn =
		i_silc_gateway_connection_lookup_conn(conn);
	struct i_silc_gateway_connection *silc_gwconn =
		(struct i_silc_gateway_connection *)gwconn;
	struct i_silc_channel *silc_channel =
		i_silc_channel_lookup_entry(silc_gwconn, channel);
	struct channel *ichannel = &silc_channel->channel;

	event = event_new(lu, EVENT_MSG);

	memset(content_type, 0, sizeof(content_type));
	memset(transfer_encoding, 0, sizeof(transfer_encoding));

	valid_mime = silc_mime_parse(message, message_len,
			NULL, 0, content_type, sizeof(content_type) - 1,
			transfer_encoding, sizeof(transfer_encoding) - 1,
			&mime_data_buffer, &mime_data_len);

	
	event_add(event, "network",
			ichannel->gwconn->gateway->network->name);
	event_add(event, "presence",
			ichannel->gwconn->local_presence->name);
	event_add(event, "channel",
			ichannel->name);
	event_add(event, "nick", sender->nickname);
	snprintf(userhost, 255, "%s@%s", sender->username, sender->hostname);
	event_add(event, "address", userhost);
	event_add_control(event, "gateway_connection", ichannel->gwconn);

	if( valid_mime == TRUE ) {
		header_length = message_len - mime_data_len;
		sprintf(header_length_str, "%d", header_length);
		event_add(event, "content_type", content_type);
		event_add(event, "transfer_encoding", transfer_encoding);
		event_add(event, "header_length", header_length_str);
	} else {
		event_add(event, "msg", message);
	}

	if( flags & SILC_MESSAGE_FLAG_SIGNED ) {
		SilcMessageSignedPayload sig =
			silc_message_get_signature(payload);
		sgn = verify_message_signature(sender, sig, payload);
		switch(sgn) {
			case 1:
				event_add(event, SILC_EVENT_SIGNATURE,
						SILC_SIGSTATUS_VALID);
				break;
			case 0:
				event_add(event, SILC_EVENT_SIGNATURE,
						SILC_SIGSTATUS_INVALID);
				break;
			case -1:
				event_add(event, SILC_EVENT_SIGNATURE,
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
}

void i_silc_operation_command(SilcClient client, SilcClientConnection conn,
		SilcClientCommandContext cmd_context, bool success,
		SilcCommand command, SilcStatus status)
{
}

void i_silc_operation_command_reply(SilcClient client,
		SilcClientConnection conn, SilcCommandPayload cmd_payload,
		bool success, SilcCommand command, SilcStatus status, ...)
{
	struct chat_protocol *proto = chat_protocol_lookup("SILC");
	struct local_user *lu = proto->local_user;
	struct channel *channel;
	struct i_silc_channel *silc_channel;
	struct event *event;
	struct i_silc_gateway_connection *silc_gwconn =
		(struct i_silc_gateway_connection *)
		i_silc_gateway_connection_lookup_conn(conn);
	struct gateway_connection *gwconn = &silc_gwconn->gwconn;
	SilcChannelEntry channel_entry;
	struct tree_iterate_context *iter;
	struct presence *presence;
	char *channel_name;
	void *key, *value;
	va_list va;

	va_start(va, status);

	switch(command) {
		case SILC_COMMAND_JOIN:
			channel_name = va_arg(va, char *);
			channel_entry = va_arg(va, SilcChannelEntry);
			silc_channel =
				i_silc_channel_lookup(silc_gwconn,
						channel_entry->channel_name);
			channel = &silc_channel->channel;

			i_assert(channel != NULL);

			silc_channel =
				(struct i_silc_channel *)channel;
			silc_channel->channel_entry = channel_entry;
			channel_set_joined(channel);
			break;
		case SILC_COMMAND_LEAVE:
			channel_entry = va_arg(va, SilcChannelEntry);
			if( success == TRUE ) {
				silc_channel =
					i_silc_channel_lookup(silc_gwconn,
						channel_entry->channel_name);
				channel = &silc_channel->channel;
				i_assert(channel);
				channel_deinit(channel, "part");
			}
			break;
	}
}

void i_silc_operation_connected(SilcClient client, SilcClientConnection conn,
		SilcClientConnectionStatus status)
{
	struct event *event;
	struct chat_protocol *proto = chat_protocol_lookup("SILC");
	struct local_user *lu = proto->local_user;
	struct gateway_connection *gwconn = NULL;
	struct i_silc_gateway_connection *silc_gwconn = NULL;

	silc_gwconn =
		(struct i_silc_gateway_connection *)
		i_silc_gateway_connection_lookup_conn(conn);
	gwconn = &silc_gwconn->gwconn;

	switch(status) {
		case SILC_CLIENT_CONN_SUCCESS:
		case SILC_CLIENT_CONN_SUCCESS_RESUME:
			silc_gwconn->connection_status = status;
			gateway_connection_set_logged_in(gwconn);
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

void i_silc_operation_disconnected(SilcClient client, SilcClientConnection conn,
		SilcStatus status, const char *message)
{
	printf(">>>>>>>>>>>>>>>>>>>>>> disconnected\n");
}

void i_silc_operation_get_auth_method(SilcClient client,
		SilcClientConnection conn, char *hostname, SilcUInt16 port,
		SilcGetAuthMeth completion, void *context)
{
	InternalGetAuthMethod internal;

	internal = silc_calloc(1, sizeof(*internal));
	internal->completion = completion;
	internal->context = context;

	silc_client_request_authentication_method(client, conn,
			i_silc_get_auth_method_callback, internal);
}

void i_silc_operation_verify_public_key(SilcClient client,
		SilcClientConnection conn, SilcSocketType conn_type,
		unsigned char *pk, SilcUInt32 pk_len, SilcSKEPKType pk_type,
		SilcVerifyPublicKey completion, void *context)
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
		const char *hostname, SilcUInt16 port,
		SilcKeyAgreementCallback *completion, void **context)
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

	switch(auth_meth) {
		case SILC_AUTH_NONE:
			(*internal->completion)(TRUE, auth_meth, NULL, 0,
						internal->context);
			break;
		case SILC_AUTH_PUBLIC_KEY:
			(*internal->completion)(TRUE, auth_meth, NULL, 0,
						internal->context);
			break;
	}

	silc_free(internal);
}
