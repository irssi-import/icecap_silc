/* Copyright 2005 Andrej Kacian */

#include <stdlib.h>

#include "lib.h"
#include "event.h"
#include "connection.h"

#include "silc-gateway.h"

#define SILC_DEFAULT_MAX_LINE_LENGTH	2048	/* FIXME: find out a real
						   limit */

static unsigned int silc_default_port = 706;

struct gateway*
i_silc_gateway_init(const char *hostname, const struct event_arg *args)
{
	struct i_silc_gateway *silc_gw;
	array_t ARRAY_DEFINE(ports, struct port_range);
	struct port_range port;

	ARRAY_CREATE(&ports, pool_datastack_create(), struct port_range, 4);
	for( ; args->key != NULL; args++ ) {
		if( strcmp(args->key, "port") == 0 && args->value != NULL ) {
			const char *p = strchr(args->value, '-');

			port.first = atoi(t_strcut(args->value, '-'));
			port.last = p == NULL ? port.first :
					(unsigned int)atoi(p+1);
			array_append(&ports, &port, 1);
		}
	}

	if( array_count(&ports) == 0) {
		port.first = port.last = silc_default_port;
		array_append(&ports, &port, 1);
	}

	silc_gw = i_new(struct i_silc_gateway, 1);
	silc_gw->max_line_length = SILC_DEFAULT_MAX_LINE_LENGTH;
	silc_gw->gateway.connection = connection_init(hostname, &ports, NULL);

	return &silc_gw->gateway;
}

void i_silc_gateway_deinit(struct gateway *gw)
{
	struct i_silc_gateway *silc_gw = (struct i_silc_gateway *)gw;

	i_free(silc_gw);
}

