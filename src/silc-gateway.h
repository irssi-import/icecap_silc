#ifndef __SILC_GATEWAY_H
#define __SILC_GATEWAY_H

#include "gateway.h"

struct i_silc_gateway {
	struct gateway gateway;

	unsigned int max_line_length;
};

struct gateway
*i_silc_gateway_init(const char *hostname, const struct event_arg *args);
void i_silc_gateway_deinit(struct gateway *gw);

#endif /* __SILC_GATEWAY_H */
