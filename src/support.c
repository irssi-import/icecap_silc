/*
 * Irssi2_silc - a SILC module for Irssi2
 * Copyright (C) 2005 Andrej Kacian
 *
 * - Support functions
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

#include <stdlib.h>

#include "lib.h"
#include "local-presence.h"
#include "local-user.h"
#include "event.h"

#include "silc.h"

unsigned int verify_message_signature(SilcClientEntry sender __attr_unused__,
		SilcMessageSignedPayload sig __attr_unused__,
		SilcMessagePayload payload __attr_unused__)
{
	/* FIXME: do signature check - return "dunno" for now */
	return -1;
}

/* A convenience function for creating "silc_event" events. */
struct event *silc_event_new(struct local_user *local_user, const char *name)
{
        struct event *event = event_new(local_user, SILC_EVENT);
        event_add(event, "event", name);
        return event;
}

char *i_silc_key_path(struct local_presence *lp, bool private_key)
{
        char *path = malloc(512);

        snprintf(path, 511, "%s/.irssi2/silc-%s.%s", getenv("HOME"), lp->name,
                        (private_key ? "prv" : "pub") );
        return path;
}

char *i_silc_gen_key_path(struct local_presence *lp, bool private_key)
{
        char *path = malloc(512);

        snprintf(path, 511, "%s/.irssi2/silc-gen-%s.%s", getenv("HOME"), lp->name,
                        (private_key ? "prv" : "pub") );
        return path;
}
