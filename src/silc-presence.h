#ifndef __SILC_PRESENCE_H
#define __SILC_PRESENCE_H

#include "lib.h"
#include "presence.h"
#include "gateway-connection.h"
#include "chat-protocol.h"

struct i_silc_presence {
	struct presence presence;

	SilcClientEntry client_entry;

	/* This should actually be stored only in struct local_presence, but
	 * since irssi2 API doesn't allow custom local_presence-derived
	 * structs, it has to go here.
	 * These two members are only used when this presence belongs to
	 * local presence, and store pointers to keypair to be loaded by
	 * i_silc_client_init()
	 * */
	char *public_key;
	char *private_key;
};

void i_silc_presence_commands_init(void);
void i_silc_presence_commands_deinit(void);

struct presence *i_silc_presence_init(struct gateway_connection *gwconn,
		const char *name);
void i_silc_presence_deinit(struct presence *presence);

void i_silc_presence_change_request(struct presence *presence,
		struct event *event);
void i_silc_presence_status_request(struct presence *presence,
		const char *const *status_fields,
		presence_status_request_callback_t *cb, void *context);

char *i_silc_userhost(SilcClientEntry client_entry);

#endif /* __SILC_PRESENCE_H */
