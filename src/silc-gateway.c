/*
 * Icecap_silc - a SILC module for Icecap
 * Copyright (C) 2005 Andrej Kacian
 *
 * - Functions related to struct i_silc_gateway
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

#include <stdlib.h>

#include "lib.h"
#include "event.h"
#include "connection.h"
#include "array.h"

#include "silc-gateway.h"

#define SILC_DEFAULT_MAX_LINE_LENGTH	2048	/* FIXME: find out a real
						   limit */

static unsigned int silc_default_port = 706;

struct gateway*
i_silc_gateway_init(const char *hostname, struct event *event)
{
	const char *port_str = event_get(event, "port");
	const char *password = event_get(event, "password");
	struct i_silc_gateway *silc_gw;
	ARRAY_TYPE(port_range) ports;
	struct port_range port;

	silc_gw = i_new(struct i_silc_gateway, 1);
	silc_gw->max_line_length = SILC_DEFAULT_MAX_LINE_LENGTH;

	t_array_init(&ports, 4);
	if( *port_str != '\0' ) {
		const char *const *list = t_strsplit(port_str, ",");

		for( ; *list != NULL; list++) {
			const char *p = strchr(*list, '-');

			port.first = atoi(t_strcut(*list, '-'));
			port.last = p == NULL ? port.first :
					(unsigned int)atoi(p+1);
			array_append(&ports, &port, 1);
		}
	}

	if( array_count(&ports) == 0) {
		port.first = port.last = silc_default_port;
		array_append(&ports, &port, 1);
	}

	if( *password != '\0' ) {
		i_free(silc_gw->server_password);
		silc_gw->server_password = i_strdup(password);
	}

	silc_gw->gateway.connection = connection_init(hostname, &ports, NULL);

	return &silc_gw->gateway;
}

void i_silc_gateway_deinit(struct gateway *gw)
{
	struct i_silc_gateway *silc_gw = (struct i_silc_gateway *)gw;

	i_free(silc_gw->server_password);
	i_free(silc_gw);
}
