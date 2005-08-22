#ifndef __SILC_MESSAGES_H
#define __SILC_MESSAGES_H

#include <silcincludes.h>
#include <silcclient.h>

#include "lib.h"
#include "gateway-connection.h"
#include "event.h"

void i_silc_message_send(struct gateway_connection *gwconn,
		struct event *event);

void *i_silc_privmsg_whois_callback(SilcClient client,
	SilcClientConnection conn, SilcClientEntry *clients, SilcUInt32 count,
	void *target_entry);

#endif /* __SILC_MESSAGES_H */
