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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "xchat.h"
#include "xchatc.h"
#include "modes.h"
#include "plugin.h"
#include "fe.h"


/* does 'chan' have a valid prefix? e.g. # or & */

int
is_channel (server * serv, char *chan)
{
	if (strchr (serv->chantypes, chan[0]))
		return 1;
	return 0;
}

/* is the given char a valid nick mode char? e.g. @ or + */

static int
is_prefix_char (server * serv, char c)
{
	int pos = 0;
	char *np = serv->nick_prefixes;

	while (np[0])
	{
		if (np[0] == c)
			return pos;
		pos++;
		np++;
	}

	if (serv->bad_prefix)
	{
		if (strchr (serv->bad_nick_prefixes, c))
		/* valid prefix char, but mode unknown */
			return -2;
	}

	return -1;
}

/* returns '@' for ops etc... */

char
get_nick_prefix (server * serv, unsigned int access)
{
	int pos;
	char c;

	for (pos = 0; pos < USERACCESS_SIZE; pos++)
	{
		c = serv->nick_prefixes[pos];
		if (c == 0)
			break;
		if (access & (1 << pos))
			return c;
	}

	return ' ';
}

/* returns the access bitfield for a nickname. E.g.
	@nick would return 000010 in binary
	%nick would return 000100 in binary
	+nick would return 001000 in binary */

unsigned int
nick_access (server * serv, char *nick, int *modechars)
{
	int i;
	unsigned int access = 0;
	char *orig = nick;

	while (*nick)
	{
		i = is_prefix_char (serv, *nick);
		if (i == -1)
			break;

		/* -2 == valid prefix char, but mode unknown */
		if (i != -2)
			access |= (1 << i);

		nick++;
	}

	*modechars = nick - orig;

	return access;
}

/* returns the access number for a particular mode. e.g.
	mode 'a' returns 0
	mode 'o' returns 1
	mode 'h' returns 2
	mode 'v' returns 3
	Also puts the nick-prefix-char in 'prefix' */

int
mode_access (server * serv, char mode, char *prefix)
{
	int pos = 0;

	while (serv->nick_modes[pos])
	{
		if (serv->nick_modes[pos] == mode)
		{
			*prefix = serv->nick_prefixes[pos];
			return pos;
		}
		pos++;
	}

	*prefix = ' ';

	return -1;
}

static int
mode_timeout_cb (session *sess)
{
	char outbuf[512];

	if (is_session (sess))
	{
		sess->mode_timeout_tag = 0;
		sprintf (outbuf, "MODE %s\r\n", sess->channel);
		tcp_send (sess->server, outbuf);
		sess->ignore_mode = TRUE;
		sess->ignore_date = TRUE;
	}
	return 0;
}

static void
record_chan_mode (session *sess)/*, char sign, char mode, char *arg)*/
{
	/* Should we write a routine to add sign,mode to sess->current_modes?
		nah! too hard. Lets just issue a MODE #channel and read it back.
		We need to find out the new modes for the titlebar, but let's not
		flood ourselves off when someone decides to change 100 modes/min. */
	if (!sess->mode_timeout_tag)
		sess->mode_timeout_tag = fe_timeout_add (15000, mode_timeout_cb, sess);
}

/* handle one mode, e.g.
   handle_single_mode (serv,outbuf,'+','b',"elite","#warez","banneduser",) */

static void
handle_single_mode (server * serv, char *outbuf, char sign, char mode,
						  char *nick, char *chan, char *arg, int quiet,
						  int is_324)
{
	session *sess;

	outbuf[0] = sign;
	outbuf[1] = 0;
	outbuf[2] = mode;
	outbuf[3] = 0;

	sess = find_session_from_channel (chan, serv);
	if (!sess || !is_channel (serv, chan))
	{
		/* got modes for a chan we're not in! probably nickmode +isw etc */
		sess = serv->front_session;
		goto genmode;
	}

	/* is this a nick mode? */
	if (strchr (serv->nick_modes, mode))
	{
		/* update the user in the userlist */
		ul_update_entry (sess, /*nickname */ arg, mode, sign);
	} else
	{
		if (!is_324 && !sess->ignore_mode)
			record_chan_mode (sess);/*, sign, mode, arg);*/
	}

	switch (sign)
	{
	case '+':
		switch (mode)
		{
		case 'k':
			if (EMIT_SIGNAL (XP_CHANSETKEY, sess, chan, nick, arg, NULL, 0) == 1)
				return;
			strncpy (sess->channelkey, arg, 60);
			fe_update_channel_key (sess);
			fe_update_mode_buttons (sess, mode, sign);
			if (!quiet)
				EMIT_SIGNAL (XP_TE_CHANSETKEY, sess, nick, arg, NULL, NULL, 0);
			return;
		case 'l':
			if (EMIT_SIGNAL (XP_CHANSETLIMIT, sess, chan, nick, arg, NULL, 0) == 1)
				return;
			sess->limit = atoi (arg);
			fe_update_channel_limit (sess);
			fe_update_mode_buttons (sess, mode, sign);
			if (!quiet)
				EMIT_SIGNAL (XP_TE_CHANSETLIMIT, sess, nick, arg, NULL, NULL, 0);
			return;
		case 'o':
			if (EMIT_SIGNAL (XP_CHANOP, sess, chan, nick, arg, NULL, 0) == 1)
				return;
			if (!quiet)
				EMIT_SIGNAL (XP_TE_CHANOP, sess, nick, arg, NULL, NULL, 0);
			return;
		case 'h':
			if (EMIT_SIGNAL (XP_CHANHOP, sess, chan, nick, arg, NULL, 0) == 1)
				return;
			if (!quiet)
				EMIT_SIGNAL (XP_TE_CHANHOP, sess, nick, arg, NULL, NULL, 0);
			return;
		case 'v':
			if (EMIT_SIGNAL (XP_CHANVOICE, sess, chan, nick, arg, NULL, 0) == 1)
				return;
			if (!quiet)
				EMIT_SIGNAL (XP_TE_CHANVOICE, sess, nick, arg, NULL, NULL, 0);
			return;
		case 'b':
			if (EMIT_SIGNAL (XP_CHANBAN, sess, chan, nick, arg, NULL, 0) == 1)
				return;
			if (!quiet)
				EMIT_SIGNAL (XP_TE_CHANBAN, sess, nick, arg, NULL, NULL, 0);
			return;
		case 'e':
			if (EMIT_SIGNAL (XP_CHANEXEMPT, sess, chan, nick, arg, NULL, 0) == 1)
				return;
			if (!quiet)
				EMIT_SIGNAL (XP_TE_CHANEXEMPT, sess, nick, arg, NULL, NULL, 0);
			return;
		case 'I':
			if (EMIT_SIGNAL (XP_CHANINVITE, sess, chan, nick, arg, NULL, 0) == 1)
				return;
			if (!quiet)
				EMIT_SIGNAL (XP_TE_CHANINVITE, sess, nick, arg, NULL, NULL, 0);
			return;
		}
		break;
	case '-':
		switch (mode)
		{
		case 'k':
			if (EMIT_SIGNAL (XP_CHANRMKEY, sess, chan, nick, NULL, NULL, 0) == 1)
				return;
			sess->channelkey[0] = 0;
			fe_update_channel_key (sess);
			fe_update_mode_buttons (sess, mode, sign);
			if (!quiet)
				EMIT_SIGNAL (XP_TE_CHANRMKEY, sess, nick, NULL, NULL, NULL, 0);
			return;
		case 'l':
			if (EMIT_SIGNAL (XP_CHANRMLIMIT, sess, chan, nick, NULL, NULL, 0) == 1)
				return;
			sess->limit = 0;
			fe_update_channel_limit (sess);
			fe_update_mode_buttons (sess, mode, sign);
			if (!quiet)
				EMIT_SIGNAL (XP_TE_CHANRMLIMIT, sess, nick, NULL, NULL, NULL, 0);
			return;
		case 'o':
			if (EMIT_SIGNAL (XP_CHANDEOP, sess, chan, nick, arg, NULL, 0) == 1)
				return;
			if (!quiet)
				EMIT_SIGNAL (XP_TE_CHANDEOP, sess, nick, arg, NULL, NULL, 0);
			return;
		case 'h':
			if (EMIT_SIGNAL (XP_CHANDEHOP, sess, chan, nick, arg, NULL, 0) == 1)
				return;
			if (!quiet)
				EMIT_SIGNAL (XP_TE_CHANDEHOP, sess, nick, arg, NULL, NULL, 0);
			return;
		case 'v':
			if (EMIT_SIGNAL (XP_CHANDEVOICE, sess, chan, nick, arg, NULL, 0) == 1)
				return;
			if (!quiet)
				EMIT_SIGNAL (XP_TE_CHANDEVOICE, sess, nick, arg, NULL, NULL, 0);
			return;
		case 'b':
			if (EMIT_SIGNAL (XP_CHANUNBAN, sess, chan, nick, arg, NULL, 0) == 1)
				return;
			if (!quiet)
				EMIT_SIGNAL (XP_TE_CHANUNBAN, sess, nick, arg, NULL, NULL, 0);
			return;
		case 'e':
			if (EMIT_SIGNAL (XP_CHANRMEXEMPT, sess, chan, nick, arg, NULL, 0) == 1)
				return;
			if (!quiet)
				EMIT_SIGNAL (XP_TE_CHANRMEXEMPT, sess, nick, arg, NULL, NULL, 0);
			return;
		case 'I':
			if (EMIT_SIGNAL (XP_CHANRMINVITE, sess, chan, nick, arg, NULL, 0) == 1)
				return;
			if (!quiet)
				EMIT_SIGNAL (XP_TE_CHANRMINVITE, sess, nick, arg, NULL, NULL, 0);
			return;
		}
	}

	fe_update_mode_buttons (sess, mode, sign);

 genmode:
	if (!quiet)
	{
		if (*arg)
		{
			char *buf = malloc (strlen (chan) + strlen (arg) + 2);
			sprintf (buf, "%s %s", chan, arg);
			EMIT_SIGNAL (XP_TE_CHANMODEGEN, sess, nick, outbuf, outbuf + 2, buf, 0);
			free (buf);
		} else
			EMIT_SIGNAL (XP_TE_CHANMODEGEN, sess, nick, outbuf, outbuf + 2, chan, 0);
	}
}

/* does this mode have an arg? like +b +l +o */

static int
mode_has_arg (server * serv, char sign, char mode)
{
	char *cm;
	int type;

	/* if it's a nickmode, it must have an arg */
	if (strchr (serv->nick_modes, mode))
		return 1;

	/* see what numeric 005 CHANMODES=xxx said */
	cm = serv->chanmodes;
	type = 0;
	while (*cm)
	{
		if (*cm == ',')
		{
			type++;
		} else if (*cm == mode)
		{
			switch (type)
			{
			case 0:					  /* type A */
			case 1:					  /* type B */
				return 1;
			case 2:					  /* type C */
				if (sign == '+')
					return 1;
			case 3:					  /* type D */
				return 0;
			}
		}
		cm++;
	}

	return 0;
}

/* handle a MODE or numeric 324 from server */

void
handle_mode (server * serv, char *outbuf, char *word[], char *word_eol[],
				 char *nick, int numeric_324)
{
	session *sess;
	char *chan;
	char *modes;
	char *argstr;
	char sign;
	int len;
	int arg;
	int i, num_args;
	int num_modes;
	int offset = 3;
	int all_modes_have_args = FALSE;

	/* numeric 324 has everything 1 word later (as opposed to MODE) */
	if (numeric_324)
		offset++;

	chan = word[offset];
	modes = word[offset + 1];
	if (*modes == ':')
		modes++;

	if (*modes == 0)
		return;	/* beyondirc's blank modes */

	sess = find_session_from_channel (chan, serv);
	if (!sess)
		sess = serv->front_session;
	/* remove trailing space */
	len = strlen (word_eol[offset]) - 1;
	if (word_eol[offset][len] == ' ')
		word_eol[offset][len] = 0;

	if (prefs.raw_modes && !numeric_324)
		EMIT_SIGNAL (XP_TE_RAWMODES, sess, nick, word_eol[offset], 0, 0, 0);

	if (numeric_324)
	{
		free (sess->current_modes);
		sess->current_modes = strdup (word_eol[offset+1]);
		fe_set_title (sess);
	}

	sign = *modes;
	modes++;
	arg = 1;

	/* count the number of arguments (e.g. after the -o+v) */
	num_args = 0;
	i = 1;
	while (i < PDIWORDS)
	{
		i++;
		if (!(*word[i + offset]))
			break;
		num_args++;
	}

	/* count the number of modes (without the -/+ chars */
	num_modes = 0;
	i = 0;
	while (i < strlen (modes))
	{
		if (modes[i] != '+' && modes[i] != '-')
			num_modes++;
		i++;
	}

	if (num_args == num_modes)
		all_modes_have_args = TRUE;

	while (1)
	{
		switch (*modes)
		{
		case 0:
			return;
		case '-':
		case '+':
			sign = *modes;
			break;
		default:
			argstr = "";
			if (all_modes_have_args || mode_has_arg (serv, sign, *modes))
			{
				arg++;
				argstr = word[arg + offset];
			}
			handle_single_mode (serv, outbuf, sign, *modes, nick, chan,
									  argstr, numeric_324 || prefs.raw_modes,
									  numeric_324);
		}

		modes++;
	}
}

/* handle the 005 numeric */

void
handle_005 (server * serv, char *word[])
{
	int w;
	char *pre;

	w = 4;							  /* start at the 4th word */
	while (*word[w] && w < PDIWORDS)
	{
		if (strncmp (word[w], "MODES=", 6) == 0)
		{
			if (atoi (word[w] + 6) >= 6)
				serv->six_modes = TRUE;
		} else if (strncmp (word[w], "CHANTYPES=", 10) == 0)
		{
			free (serv->chantypes);
			serv->chantypes = strdup (word[w] + 10);
		} else if (strncmp (word[w], "CHANMODES=", 10) == 0)
		{
			free (serv->chanmodes);
			serv->chanmodes = strdup (word[w] + 10);
		} else if (strncmp (word[w], "PREFIX=", 7) == 0)
		{
			pre = strchr (word[w] + 7, ')');
			if (pre)
			{
				pre[0] = 0;			  /* NULL out the ')' */
				free (serv->nick_prefixes);
				free (serv->nick_modes);
				serv->nick_prefixes = strdup (pre + 1);
				serv->nick_modes = strdup (word[w] + 8);
			} else
			{
				/* bad! some ircds don't give us the modes. */
				/* in this case, we use it only to strip /NAMES */
				serv->bad_prefix = TRUE;
				if (serv->bad_nick_prefixes)
					free (serv->bad_nick_prefixes);
				serv->bad_nick_prefixes = strdup (word[w] + 7);
			}
		} else if (strncmp (word[w], "WATCH=", 6) == 0)
		{
			serv->supports_watch = TRUE;
		}

		w++;
	}
}
