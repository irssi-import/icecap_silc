#ifndef __SILC_GATEWAY_H
#define __SILC_GATEWAY_H

#include "gateway.h"

#define IS_SILC_GATEWAY(gateway) \
	(strcmp((gateway->network->protocol)->name, "SILC") == 0)

struct i_silc_gateway {
	struct gateway gateway;

	unsigned int max_line_length;
};

struct gateway
*i_silc_gateway_init(const char *hostname, struct event *event);
void i_silc_gateway_deinit(struct gateway *gw);

#endif /* __SILC_GATEWAY_H */
