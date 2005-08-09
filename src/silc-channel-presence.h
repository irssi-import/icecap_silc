#ifndef __SILC_CHANNEL_PRESENCE_H
#define __SILC_CHANNEL_PRESENCE_H

#include "channel-presence.h"

struct i_silc_channel_presence {
	struct channel_presence chpres;

	SilcChannelUser chuser;
};

struct channel_presence *i_silc_channel_presence_init(
		struct channel_connection *chconn,
		struct presence *presence);
void i_silc_channel_presence_deinit(struct channel_presence *chpres);

#endif /* __SILC_CHANNEL_PRESENCE_H */
