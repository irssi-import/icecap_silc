/*
 * Irssi2_silc - a SILC module for Irssi2
 * Copyright (C) 2005 Andrej Kacian
 *
 * - Functions related to struct i_silc_channel
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <silcincludes.h>
#include <silcclient.h>

#include "lib.h"
#include "event.h"
#include "tree.h"
#include "presence.h"
#include "local-presence.h"
#include "channel.h"
#include "network.h"
#include "gateway.h"
#include "tree.h"

#include "silc.h"
#include "support.h"
#include "silc-channel.h"
#include "silc-presence.h"

struct channel *
i_silc_channel_init(struct local_presence *lp __attr_unused__,
		 const char *name __attr_unused__)
{
	return i_new(struct channel, 1);
}

void i_silc_channel_deinit(struct channel *channel)
{
	i_free(channel);
}
