#ifndef __SILC_LOCAL_PRESENCE_H
#define __SILC_LOCAL_PRESENCE_H

#include "buffer.h"

void i_silc_presence_commands_init(void);
void i_silc_presence_commands_deinit(void);

struct i_silc_local_presence_auth {
	char *public_key;
	struct buffer *private_key;
	char *passphrase;
};

#endif /* __SILC_LOCAL_PRESENCE_H */
