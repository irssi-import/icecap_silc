#ifndef __SILC_MESSAGES_H
#define __SILC_MESSAGES_H

#include <silc.h>
#include <silcclient.h>

#include "lib.h"
#include "gateway-connection.h"
#include "event.h"

struct i_privmsg_cb_t {
	char *msg;
	SilcMessageFlags sendflags;
};

void i_silc_message_send(struct gateway_connection *gwconn,
		struct event *event, struct event *reply);

void i_silc_privmsg_whois_callback(SilcClient client,
	SilcClientConnection conn, SilcClientEntry *clients, SilcUInt32 count,
	void *i_privmsg_cb);

#endif /* __SILC_MESSAGES_H */
