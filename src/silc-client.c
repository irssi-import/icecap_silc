/*
 * Icecap_silc - a SILC module for Icecap
 * Copyright (C) 2005 Andrej Kacian
 *
 * - Functions related to SILC client itself
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
#include "base64.h"
#include "chat-protocol.h"
#include "event.h"
#include "local-presence.h"

#include <silc.h>
#include <silcclient.h>

#include "icecap-silc.h"
#include "support.h"
#include "silc-client.h"
#include "silc-presence.h"
#include "silc-local-presence.h"

SilcPublicKey public_key = NULL;
SilcPrivateKey private_key = NULL;

extern SilcClientOperations ops;
extern unsigned int silc_module_id;

SilcClient i_silc_client_init(struct local_presence *lp)
{
	SilcClient client;
	SilcClientParams params;
	struct event *event;
	struct local_user *lu = lp->local_user;
//	struct i_silc_local_presence_auth *const *_auth;
//	struct i_silc_local_presence_auth *auth;
	char *fingerprint;
	unsigned char *pk;
	SilcUInt32 pk_len;

	strncpy(params.nickname_format, "%n@%h%a",
			sizeof(params.nickname_format) - 1);
	params.nickname_force_format = FALSE;

	client = silc_client_alloc(&ops, &params, lu, NULL);

	client->username = silc_get_username();
	if( !client->username )
		client->username = "icecap";

	client->hostname = silc_net_localhost();
	if( !client->hostname )
		client->hostname = "localhost"; /* shouldn't happen */

	client->realname = "icecap is here!";
	
	silc_pkcs_register_default();
	silc_hash_register_default();
	silc_cipher_register_default();
	silc_hmac_register_default();

//	_auth = array_idx(&lp->module_contexts, silc_module_id);
//	auth = *_auth;
//	i_assert(auth != NULL);

	/* Try loading keys provided with the presence */
//	if( auth->public_key && auth->private_key ) {
//		i_silc_load_keys(auth, &client->pkcs, &client->public_key,
//						&client->private_key);
//		memset(auth, 0, sizeof(auth));
//	}

	/* Use some pre-generated keys for now */
	if( !private_key->pkcs ) {
		silc_load_key_pair(i_silc_gen_key_path(lp, FALSE),
			i_silc_gen_key_path(lp, TRUE), "", &public_key, &private_key);
	}

	/* Generate a keypair for use */
	if( !private_key->pkcs ) {
		silc_create_key_pair(NULL, 0, i_silc_gen_key_path(lp, FALSE),
			i_silc_gen_key_path(lp, TRUE),
			"UN=icecap,HN=localhost", "", &public_key, &private_key, FALSE);
		event = silc_server_event_new(lu, SILC_EVENT_KEY_KEYS_GENERATED);
		event_send(&event);
	}

	if( !private_key->pkcs ) {
		event = silc_server_event_new(lu, SILC_EVENT_KEY_KEYS_LOADED);

		pk = silc_pkcs_public_key_encode(public_key, &pk_len);
		fingerprint = silc_hash_fingerprint(NULL, pk, pk_len);

		event_add(event, SILC_EVENT_KEY_FINGERPRINT, fingerprint);
		event_send(&event);

		free(pk);
		free(fingerprint);
	}

	silc_client_init(client, client->username, client->hostname,
			client->realname, NULL, NULL);
	return client;
}

void i_silc_client_deinit(SilcClient client, SilcClientConnection conn)
{
	silc_client_close_connection(client, conn);
	silc_client_free(client);
}

bool i_silc_client_id_is_me(struct i_silc_gateway_connection *silc_gwconn,
				SilcClientID *id)
{
	if( SILC_ID_COMPARE(silc_gwconn->conn->local_id, id,
				sizeof(SilcClientID)) )
		return TRUE;
	return FALSE;
}
