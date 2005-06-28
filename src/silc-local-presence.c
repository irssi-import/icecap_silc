/*
 * Irssi2_silc - a SILC module for Irssi2
 * Copyright (C) 2005 Andrej Kacian
 *
 * - SILC-specific unctions related to irssi2's struct presence
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

#include <string.h>

#include <silcincludes.h>
#include <silcclient.h>

#include "lib.h"
#include "buffer.h"
#include "event.h"
#include "client-commands.h"
#include "client.h"
#include "local-presence.h"
#include "network.h"
#include "base64.h"

#include "silc.h"
#include "support.h"
#include "silc-presence.h"
#include "silc-gateway-connection.h"
#include "silc-local-presence.h"

extern unsigned int silc_module_id;

static struct client_command_bind_list silc_cmd_presence_low[];

void i_silc_presence_commands_init(void)
{
	client_command_bind_list(silc_cmd_presence_low, PRIORITY_LOW);
}

void i_silc_presence_commands_deinit(void){
	client_command_unbind_list(silc_cmd_presence_low);
}

static void silc_cmd_presence_add_low(struct event *event)
{
	const char *pub_key = event_get(event, SILC_EVENT_KEY_PUBKEY);
	const char *prv_key = event_get(event, SILC_EVENT_KEY_PRVKEY);
	const char *passphrase = event_get(event, SILC_EVENT_KEY_PASSPHRASE);
	const char *name = event_get(event, "name");
	struct client *client = event_get_client(event);
	struct local_user *lu;
	const char *network = event_get(event, "network");
//	struct buffer *debuf = buffer_create_dynamic(default_pool, 1);
	struct network *net;
	struct local_presence *lpr;
	struct i_silc_local_presence_auth *auth;

	auth = i_new(struct i_silc_local_presence_auth, 1);

	i_assert( client != NULL );
	i_assert( network != NULL );

	lu = client->local_user;
	net = network_lookup(network);
	i_assert( net != NULL ); 

	/* There should already be a local presence initialized */
	lpr = local_presence_lookup(lu, net, name);
	i_assert(lpr != NULL);

	/* Don't do anything special if the keys weren't passed */
	if( pub_key == NULL || prv_key == NULL ||
			!strlen(pub_key) || !strlen(prv_key) ) {
		auth->public_key = NULL;
		auth->private_key = NULL;
		auth->passphrase = NULL;
	} else {
		auth->public_key = strdup(pub_key);

		auth->private_key = buffer_create_dynamic(default_pool, 1);
		base64_decode(prv_key, strlen(prv_key), NULL,
				auth->private_key);

		auth->passphrase =
			(passphrase ? strdup(passphrase) : strdup(""));
	}

	array_idx_set(&lpr->module_contexts, silc_module_id, &auth);
}

static struct client_command_bind_list silc_cmd_presence_low[] = {
	{ NULL, "presence add", silc_cmd_presence_add_low },
	{ NULL, NULL, NULL }
};