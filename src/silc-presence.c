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

#include <silcincludes.h>
#include <silcclient.h>

#include "lib.h"
#include "presence.h"
#include "local-presence.h"
#include "gateway-connection.h"
#include "chat-protocol.h"
#include "event.h"
#include "client-commands.h"

#include "silc-presence.h"
#include "silc-gateway-connection.h"

struct presence *i_silc_presence_init(struct gateway_connection *gwconn,
		const char *name)
{
	struct i_silc_presence *silc_presence;

	silc_presence = i_new(struct i_silc_presence, 1);
	silc_presence->client_entry = NULL;

	return &silc_presence->presence;
}

void i_silc_presence_deinit(struct presence *presence)
{
	struct i_silc_presence *silc_presence =
		(struct i_silc_presence *)presence;
	i_free(silc_presence);
}

void i_silc_presence_change_request(struct local_presence *lpresence,
		struct event *event, presence_change_request_callback_t *cb,
		void *context)
{
	struct i_silc_gateway_connection *silc_gwconn =
		(struct i_silc_gateway_connection *)lpresence->_gwconn;
	const char *new_name = event_get(event, "new_name");

	if( *new_name == '\0' )
		cb(CLIENT_CMDERR_ARGS, lpresence, context);
	else
		silc_client_command_call(silc_gwconn->client, silc_gwconn->conn,
				NULL, "NICK", new_name, NULL);
}

void i_silc_presence_status_request(struct presence *presence,
		const char *const *status_fields,
		presence_status_request_callback_t *cb, void *context)
{
	struct i_silc_presence *silc_presence =
		(struct i_silc_presence *)presence;
	struct i_silc_gateway_connection *silc_gwconn;
	SilcClientID *id;
	SilcBuffer idp;

	i_assert(silc_presence != NULL);

	silc_gwconn = (struct i_silc_gateway_connection *)presence->gwconn;

	if( silc_presence->client_entry == NULL ) {
		/* the presence is unknown to us, WHOIS by nickname */
		silc_client_command_send(silc_gwconn->client, silc_gwconn->conn,
			SILC_COMMAND_WHOIS, 0, 1, 1, presence->name,
			strlen(presence->name) );
	} else {
		/* we know this presence, WHOIS by clientid */
		id = silc_presence->client_entry->id;
		idp = silc_id_payload_encode(id, SILC_ID_CLIENT);
		silc_client_command_send(silc_gwconn->client, silc_gwconn->conn,
			SILC_COMMAND_WHOIS, 0, 4, 1, NULL, NULL, 2, NULL, NULL,
			3, NULL, NULL, 4, idp->data, idp->len);
	}
}

/*
 * Return a newly allocated "user@host" string made from client_entry info.
 * Returned string must be freed!
 */
char *i_silc_userhost(SilcClientEntry client_entry)
{
	char *userhost = malloc(256);

	if(client_entry != NULL)
		snprintf(userhost, 255, "%s@%s", client_entry->username,
			client_entry->hostname);
	else sprintf(userhost, "@");

	return userhost;
}
