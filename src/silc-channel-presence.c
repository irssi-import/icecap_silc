/*
 * Irssi2_silc - a SILC module for Irssi2
 * Copyright (C) 2005 Andrej Kacian
 *
 * - Functions related to struct i_silc_channel_presence
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
#include "silc-channel-presence.h"

struct channel_presence *
i_silc_channel_presence_init(struct channel_connection *chconn,
		struct presence *presence)
{
	struct i_silc_channel_presence *silc_chpres;

	silc_chpres = i_new(struct i_silc_channel_presence, 1);
	return &silc_chpres->chpres;
}

void i_silc_channel_presence_deinit(struct channel_presence *chpres)
{
	i_free(chpres);
}
