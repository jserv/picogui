/* X-Chat
 * Copyright (C) 1998 Peter Zelezny.
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "xchat.h"
#include "cfgfiles.h"
#include "util.h"
#include "plugin.h"
#include "util.h"
#include "modes.h"
#include "outbound.h"
#include "inbound.h"
#include "dcc.h"
#include "ctcp.h"
#include "xchatc.h"


static void
ctcp_reply (session *sess, char *tbuf, char *nick, char *word[],
				char *word_eol[], char *conf)
{
	auto_insert (tbuf, conf, word, word_eol, "", "", word_eol[5], "", "", nick);

	handle_command (tbuf, sess, 0, 0);
}

static int
ctcp_check (session *sess, char *tbuf, char *nick, char *word[],
				char *word_eol[], char *ctcp)
{
	int ret = 0;
	char *po;
	struct popup *pop;
	GSList *list = ctcp_list;

	po = strchr (ctcp, '\001');
	if (po)
		*po = 0;

	po = strchr (word_eol[5], '\001');
	if (po)
		*po = 0;

	while (list)
	{
		pop = (struct popup *) list->data;
		if (!strcasecmp (ctcp, pop->name))
		{
			ctcp_reply (sess, tbuf, nick, word, word_eol, pop->cmd);
			ret = 1;
		}
		list = list->next;
	}
	return ret;
}

void
handle_ctcp (struct session *sess, char *outbuf, char *to, char *nick,
				 char *msg, char *word[], char *word_eol[])
{
	char *po;
	session *chansess;

	if (!strncasecmp (msg, "VERSION", 7) && !prefs.hidever)
	{
		sprintf (outbuf, "NOTICE %s :\001VERSION xchat "VERSION" %s (PicoGUI)\001\r\n",
					nick, get_cpu_str ());
		tcp_send (sess->server, outbuf);
	}

	if (!ctcp_check (sess, outbuf, nick, word, word_eol, word[4] + 2))
	{
		if (!strncasecmp (msg, "ACTION", 6))
		{
			po = strchr (msg + 7, '\001');
			if (po)
				po[0] = 0;
			channel_action (sess, outbuf, to, nick, msg + 7, FALSE);
			return;
		}
		if (!strncasecmp (msg, "DCC", 3))
		{
			handle_dcc (sess, outbuf, nick, word, word_eol);
			return;
		}
		if (!strncasecmp (msg, "SOUND", 5))
		{
			po = strchr (word[5], '\001');
			if (po)
				po[0] = 0;
			EMIT_SIGNAL (XP_TE_CTCPSND, sess->server->front_session, word[5],
							 nick, NULL, NULL, 0);
			sprintf (outbuf, "%s/%s", prefs.sounddir, word[5]);
			if (strchr (word[5], '/') == 0 && access (outbuf, R_OK) == 0)
			{
				sprintf (outbuf, "%s %s/%s", prefs.soundcmd, prefs.sounddir,
							word[5]);
				xchat_exec (outbuf);
			}
			return;
		}
	}

	po = strchr (msg, '\001');
	if (po)
		po[0] = 0;
	if (!is_channel (sess->server, to))
	{
		EMIT_SIGNAL (XP_TE_CTCPGEN, sess->server->front_session, msg, nick,
						 NULL, NULL, 0);
	} else
	{
		chansess = find_session_from_channel (to, sess->server);
		if (chansess)
		{
			EMIT_SIGNAL (XP_TE_CTCPGENC, chansess, msg, nick, to, NULL, 0);
			return;
		}
		EMIT_SIGNAL (XP_TE_CTCPGENC, sess->server->front_session, msg, nick, to,
						 NULL, 0);
	}
}
