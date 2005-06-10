#ifndef __SILC_CHANNEL_H
#define __SILC_CHANNEL_H

#include <silcincludes.h>
#include <silcclient.h>

#include "channel.h"

#include "silc-gateway-connection.h"

struct i_silc_channel {
	struct channel channel;
	char *name;

	SilcChannelEntry channel_entry;
};

typedef struct {
	SilcChannelEntry channel;
	bool retry;
} SilcJoinResolve;

void i_silc_channel_events_init(void);
void i_silc_channel_events_deinit(void);

struct channel *i_silc_channel_init(struct gateway_connection *gwconn,
				 const char *name);
void i_silc_channel_deinit(struct channel *channel);

void i_silc_join_send(struct channel *channel, struct event *event);
void i_silc_part_send(struct channel *channel, struct event *event);
/*
static void refresh_nicklist_resolved(SilcClient client,
		SilcClientConnection conn, SilcClientEntry *clients,
		SilcUInt32 clients_count, void *context);
*/
struct i_silc_channel *
i_silc_channel_lookup_entry(struct i_silc_gateway_connection *silc_gwconn,
		SilcChannelEntry entry);

struct i_silc_channel *
i_silc_channel_lookup(struct i_silc_gateway_connection *silc_gwconn,
		const char *name);

#endif /* __SILC_CHANNEL_H */
