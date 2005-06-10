#ifndef __SILC_MESSAGES_H
#define __SILC_MESSAGES_H

#include "lib.h"
#include "gateway-connection.h"
#include "event.h"

void i_silc_message_send(struct gateway_connection *gwconn,
		struct event *event);

#endif /* __SILC_MESSAGES_H */
