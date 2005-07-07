#ifndef __SILC_CHANNEL_CONNECTION_H
#define __SILC_CHANNEL_CONNECTION_H

#include <silcincludes.h>
#include <silcclient.h>

#include "channel-connection.h"

#include "silc-gateway-connection.h"

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
