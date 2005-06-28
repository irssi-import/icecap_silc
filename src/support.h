#ifndef __SUPPORT_H
#define __SUPPORT_H

unsigned int verify_message_signature(SilcClientEntry sender,
		SilcMessageSignedPayload sig, SilcMessagePayload payload);

struct event *silc_event_new(struct local_user *local_user, const char *name);

char *i_silc_key_path(struct local_presence *lp, bool private_key);
char *i_silc_gen_key_path(struct local_presence *lp, bool private_key);

#define IS_SILC_PROTOCOL(protocol) \
	(strcmp((protocol)->name, "SILC") == 0)

#endif /* __SUPPORT_H */
