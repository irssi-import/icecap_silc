#ifndef __SILC_H
#define __SILC_H

#include <silcincludes.h>
#include <silcclient.h>

#include "lib.h"
#include "chat-protocol.h"

#include "silc-gateway-connection.h"

#define SILC_DEFAULT_CHARSET	"UTF-8"

struct silc_protocol {
	struct chat_protocol protocol;

	SilcClient client;	/* THE client */
	SilcClientConnection conn;
	SilcRng rng;
	SilcClientOperations ops;
};	

extern struct chat_protocol silc_protocol;

void i_silc_scheduler(void *client);
void i_silc_client_close_connection(struct i_silc_gateway_connection *
		silc_gwconn);

struct event *silc_event_new(struct local_user *local_user, const char *name);

unsigned int verify_message_signature(SilcClientEntry sender,
		SilcMessageSignedPayload sig, SilcMessagePayload payload);

void i_silc_events_init(void);
void i_silc_events_deinit(void);

#define IS_SILC_PROTOCOL(protocol) \
	(strcmp((protocol)->name, "SILC") == 0)

#define SILC_EVENT			"silc_event"

#define SILC_EVENT_CONNECTED		"connected"

#define SILC_EVENT_COMMAND_SUCCESS	"cmd_success"
#define SILC_EVENT_COMMAND_FAIL		"cmd_fail"
#define SILC_EVENT_ERROR		"error"
#define SILC_EVENT_SERVER_SAY		"server_say"

#define SILC_EVENT_NOTIFY_NONE		"notify"
#define SILC_EVENT_NOTIFY_MOTD		"motd"
#define SILC_EVENT_NOTIFY_INVITE	"invite"
#define SILC_EVENT_NOTIFY_JOIN		"join"
#define SILC_EVENT_NOTIFY_LEAVE		"leave"
#define SILC_EVENT_NOTIFY_SIGNOFF	"signoff"

#define SILC_EVENT_SIGNATURE		"signature"
#define SILC_SIGSTATUS_VALID		"valid"
#define SILC_SIGSTATUS_INVALID		"invalid"
#define SILC_SIGSTATUS_DUNNO		"dunno"

#endif /* __SILC_H */
