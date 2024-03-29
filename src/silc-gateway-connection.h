#ifndef __SILC_GATEWAY_CONNECTION_H
#define __SILC_GATEWAY_CONNECTION_H

#include <silc.h>

#include <server/gateway-connection.h>

#define IS_SILC_GWCONN(gwconn) \
	(strcmp((gwconn->gateway->network->protocol)->name, "SILC") == 0)

struct i_silc_gateway_connection {
	struct gateway_connection gwconn;

	char *passphrase;
	SilcClient client;
	SilcClientOperations ops;
	SilcClientConnection conn;
	SilcSchedule schedule;
	SilcClientConnectionStatus connection_status;
	struct timeout *timeout;
	bool connected;
};

struct gateway_connection *
i_silc_gateway_connection_init(struct gateway *gw, struct local_presence *lp);
void i_silc_gateway_connection_deinit(struct gateway_connection *gwconn);

void i_silc_gateway_connection_events_init(void);
void i_silc_gateway_connection_events_deinit(void);

struct gateway_connection *
i_silc_gateway_connection_lookup_conn(SilcClientConnection conn);

#endif /* __SILC_GATEWAY_CONNECTION_H */
