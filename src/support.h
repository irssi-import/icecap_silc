#ifndef __SUPPORT_H
#define __SUPPORT_H

#include <silc.h>

#include <lib/lib.h>

#include "silc-local-presence.h"

unsigned int verify_message_signature(SilcClientEntry sender,
		SilcMessagePayload payload);

struct event *silc_server_event_new(struct local_user *lu, const char *name);
char *i_silc_gen_key_path(struct local_presence *lp, bool private_key);

//bool i_silc_load_keys(struct i_silc_local_presence_auth *auth, SilcPKCS *pkcs,
//		SilcPublicKey *public_key, SilcPrivateKey *private_key);

#endif /* __SUPPORT_H */
