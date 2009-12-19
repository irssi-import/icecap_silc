#ifndef __SILC_CHANNEL_CONNECTION_H
#define __SILC_CHANNEL_CONNECTION_H

#include <silc.h>
#include <silcclient.h>

#include "channel-connection.h"

#include "silc-gateway-connection.h"

#define IS_SILC_CHCONN(chconn) \
	(strcmp((chconn->gwconn->gateway->network->protocol)->name, \
		"SILC") == 0)

struct i_silc_channel_connection {
	struct channel_connection chconn;
	char *name;

	SilcChannelEntry channel_entry;
};

typedef struct {
	SilcChannelEntry channel;
	bool retry;
} SilcJoinResolve;

void i_silc_channel_connection_events_init(void);
void i_silc_channel_connection_events_deinit(void);

struct channel_connection *i_silc_channel_connection_init(
				struct gateway_connection *gwconn,
				struct channel *channel,
				struct event *event);
void i_silc_channel_connection_deinit(struct channel_connection *channel);

void i_silc_channel_change_request(struct channel_connection *chconn,
				struct event *event,
				async_change_request_callback_t *cb,
				struct client_async_cmd_context *ctx);

void event_channel_connection_init(struct event *event);
void event_channel_connection_deinit(struct event *event);

/*
static void refresh_nicklist_resolved(SilcClient client,
		SilcClientConnection conn, SilcClientEntry *clients,
		SilcUInt32 clients_count, void *context);
*/
struct i_silc_channel_connection *
i_silc_channel_connection_lookup_entry(
		struct i_silc_gateway_connection *silc_gwconn,
		SilcChannelEntry entry);

struct i_silc_channel_connection *
i_silc_channel_connection_lookup(struct i_silc_gateway_connection *silc_gwconn,
		const char *name);

#endif /* __SILC_CHANNEL_CONNECTION_H */
