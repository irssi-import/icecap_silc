#ifndef __SILC_CHANNEL_H
#define __SILC_CHANNEL_H

struct channel *i_silc_channel_init(struct local_presence *local_presence,
				 const char *name);
void i_silc_channel_deinit(struct channel *channel);

#endif /* __SILC_CHANNEL_H */
