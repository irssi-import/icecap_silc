#ifndef __SILC_CLIENT_H
#define __SILC_CLIENT_H

#include <silcincludes.h>
#include <silcclient.h>

SilcClient i_silc_client_init(void);
void i_silc_client_deinit(SilcClient client, SilcClientConnection conn);

void i_silc_client_nickname_parse(const char *nickname, char **return_nickname);
bool i_silc_client_id_is_me(struct i_silc_gateway_connection *silc_gwconn,
				SilcClientID *id);

#define SILC_PUBKEY "silc.pub"
#define SILC_PRVKEY "silc.prv"

#endif /* __SILC_CLIENT_H */
