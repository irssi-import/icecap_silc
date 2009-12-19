#ifndef __CLIENTOPERATIONS_H
#define __CLIENTOPERATIONS_H

#include <silc.h>
#include <silcclient.h>

void i_silc_operation_say(SilcClient client, SilcClientConnection conn,
		SilcClientMessageType type, char *msg, ...);

void i_silc_operation_channel_message(SilcClient client,
		SilcClientConnection conn,
		SilcClientEntry sender,
		SilcChannelEntry channel,
		SilcMessagePayload payload,
		SilcChannelPrivateKey key,
		SilcMessageFlags flags,
		const unsigned char *message,
		SilcUInt32 message_len);

void i_silc_operation_private_message(SilcClient client,
		SilcClientConnection conn, SilcClientEntry sender,
		SilcMessagePayload payload, SilcMessageFlags flags,
		const unsigned char *message, SilcUInt32 message_len);

void i_silc_operation_notify(SilcClient client, SilcClientConnection conn,
		SilcNotifyType type, ...);

void i_silc_operation_command(SilcClient client, SilcClientConnection conn,
		SilcBool success, SilcCommand command, SilcStatus status,
		SilcUInt32 argc, unsigned char **argv);

void i_silc_operation_command_reply(SilcClient client,
		SilcClientConnection conn, SilcCommand command,
		SilcStatus status, SilcStatus error, va_list va);

void i_silc_operation_get_auth_method(SilcClient client,
		SilcClientConnection conn, char *hostname, SilcUInt16 port,
		SilcAuthMethod auth_method, SilcGetAuthMeth completion, void *context);

void i_silc_operation_verify_public_key(SilcClient client,
		SilcClientConnection conn, SilcConnectionType conn_type,
		SilcPublicKey public_key, SilcVerifyPublicKey completion,
		void *context);

void i_silc_operation_ask_passphrase(SilcClient client,
		SilcClientConnection conn, SilcAskPassphrase completion,
		void *context);

void i_silc_operation_key_agreement(SilcClient client,
		SilcClientConnection conn, SilcClientEntry client_entry,
		const char *hostname, SilcUInt16 protocol, SilcUInt16 port);

void i_silc_operation_ftp(SilcClient client, SilcClientConnection conn,
		SilcClientEntry client_entry, SilcUInt32 session_id,
		const char *hostname, SilcUInt16 port);

void i_silc_get_auth_method_callback(SilcClient client,
		SilcClientConnection conn, SilcAuthMethod auth_meth,
		void *context);

#endif /* __CLIENTOPERATIONS_H */
