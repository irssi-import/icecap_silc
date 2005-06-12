/* Copyright 2005 Andrej Kacian */

#include <silcincludes.h>
#include <silcclient.h>

#include "lib.h"
#include "chat-protocol.h"
#include "event.h"
#include "local-presence.h"

#include "silc.h"
#include "silc-client.h"

extern SilcClientOperations ops;

SilcClient i_silc_client_init(void)
{
	SilcClient client;
	SilcClientParams params;
	struct event *event;
	struct chat_protocol *proto = chat_protocol_lookup("SILC");
	struct local_user *lu = proto->local_user;

	params.task_max = 200;
	params.rekey_secs = 0;			/* use default */
	params.connauth_request_secs = 0; 	/* use default */
	strncpy(params.nickname_format, "%n@%h%a",
			sizeof(params.nickname_format) - 1);
	params.nickname_force_format = FALSE;
	params.nickname_parse = &i_silc_client_nickname_parse;
	params.ignore_requested_attributes = TRUE;

	client = silc_client_alloc(&ops, &params, NULL, NULL);

	client->username = silc_get_username();
	if( !client->username )
		client->username = "irssi2";

	client->hostname = silc_net_localhost();
	if( !client->hostname )
		client->hostname = "localhost"; /* shouldn't happen */

	client->realname = "irssi2 is here!";
	
	silc_pkcs_register_default();
	silc_hash_register_default();
	silc_cipher_register_default();
	silc_hmac_register_default();

	silc_load_key_pair(SILC_PUBKEY, SILC_PRVKEY, "",
			&client->pkcs, &client->public_key,
			&client->private_key);


	if( !client->pkcs ) {
		silc_create_key_pair(NULL, 0, SILC_PUBKEY, SILC_PRVKEY,
			"UN=irssi2,HN=localhost", "", &client->pkcs,
			&client->public_key, &client->private_key, FALSE);
		event = event_new(lu, "silc_keys_generated");
		event_add(event, "pubkey", SILC_PUBKEY);
		event_add(event, "prvkey", SILC_PRVKEY);
		event_send(event);
	} else {
		event = event_new(lu, "silc_keys_loaded");
		event_send(event);
	}
		

	silc_client_init(client);
	return client;
}

void i_silc_client_deinit(SilcClient client, SilcClientConnection conn)
{
	silc_client_del_connection(client, conn);
	silc_client_free(client);
}

void i_silc_client_nickname_parse(const char *nickname, char **return_nickname)
{
	*return_nickname = strdup(nickname);
}

bool i_silc_client_id_is_me(struct i_silc_gateway_connection *silc_gwconn,
				SilcClientID *id)
{
	if( !SILC_ID_COMPARE(silc_gwconn->conn->local_entry->id, id,
				sizeof(SilcClientID)) )
		return TRUE;
	return FALSE;
}
