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
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#define WANTARPA
#define WANTDNS
#include "inet.h"

#include "xchat.h"
#include "util.h"
#include "ignore.h"
#include "plugin.h"
#include "perlc.h"
#include "fe.h"
#include "modes.h"
#include "notify.h"
#include "outbound.h"
#include "inbound.h"
#include "text.h"
#include "ctcp.h"
#include "xchatc.h"


/* black n white(0/1) are bad colors for nicks, and we'll use color 2 for us */
/* also light/dark gray (14/15) */
/* 5,7,8 are all shades of yellow which happen to look dman near the same */

static int rcolors[] = { 3, 4, 6, 8, 9, 10, 11, 12, 13 };

static int
color_of (char *name)
{
	int i = 0, sum = 0;

	while (name[i])
		sum += name[i++];
	sum %= sizeof (rcolors) / sizeof (int);
	return rcolors[sum];
}

void
clear_channel (struct session *sess)
{
	if (prefs.persist_chans)
		safe_strcpy (sess->waitchannel, sess->channel, CHANLEN);
	sess->channel[0] = 0;

	if (sess->logfd != -1)
	{
		end_logging (sess->logfd);
		sess->logfd = -1;
	}

	free (sess->current_modes);
	sess->current_modes = strdup ("");

	if (sess->mode_timeout_tag)
	{
		fe_timeout_remove (sess->mode_timeout_tag);
		sess->mode_timeout_tag = 0;
	}

	fe_clear_channel (sess);
	clear_user_list (sess);
	fe_set_nonchannel (sess, FALSE);
	fe_set_title (sess);
}

void
set_topic(struct session *sess, char *topic)
{
	if (sess->topic)
		free(sess->topic);
	sess->topic = strdup(topic);
	fe_set_topic(sess, topic);
}

static session *
find_session_from_nick (char *nick, server *serv)
{
	session *sess;
	GSList *list = sess_list;

	sess = find_session_from_channel (nick, serv);
	if (sess)
		return sess;

	if (serv->front_session)
	{
		if (find_name (serv->front_session, nick))
			return serv->front_session;
	}

	if (menu_sess && menu_sess->server == serv)
	{
		if (find_name (menu_sess, nick))
			return menu_sess;
	}

	while (list)
	{
		sess = list->data;
		if (sess->server == serv)
		{
			if (find_name (sess, nick))
				return sess;
		}
		list = list->next;
	}
	return 0;
}

void
private_msg (struct server *serv, char *tbuf, char *from, char *ip,
				 char *text)
{
	struct session *sess;

	if (EMIT_SIGNAL (XP_PRIVMSG, serv, from, ip, text, NULL, 0) == 1)
		return;

	sess = find_session_from_channel (from, serv);

	if (((sess) && fe_is_beep (sess)) || prefs.beepmsg)
		fe_beep ();

	if (sess || prefs.autodialog)
	{
		/*0=ctcp  1=priv will set autodialog=0 here is flud detected */
		if (!sess)
		{
			if (flood_check (from, ip, serv, menu_sess, 1))
				sess = new_ircwindow (serv, from, SESS_DIALOG);	/* Create a dialog session */
			else
				sess = serv->front_session;
		}
		if (prefs.logging
			&& sess->logfd != -1
			&& ip
			&& *ip
			&& (!sess->topic || strcmp(sess->topic, ip)))
		{
			char temp[512];
			snprintf(temp, sizeof(temp), "[%s has address %s]\n", from, ip);
			write(sess->logfd, temp, strlen(temp));
		}
		channel_msg (serv, tbuf, from, from, text, FALSE);
		if (ip && ip[0])
			set_topic (sess, ip);
		return;
	}
	sess = find_session_from_nick (from, serv);
	if (!sess)
		sess = serv->front_session;
	EMIT_SIGNAL (XP_TE_PRIVMSG, sess, from, text, NULL, NULL, 0);
}

static int
SearchNick (char *text, char *nicks)
{
	char S[300];	/* size of bluestring in xchatprefs */
	char *n;
	char *p;
	char *t;
	size_t ns;

	if (nicks == NULL)
		return 0;

	text = strip_color (text);

	safe_strcpy (S, nicks, sizeof (S));
	n = strtok (S, ",");
	while (n != NULL)
	{
		t = text;
		ns = strlen (n);
		while ((p = nocasestrstr (t, n)))
		{
			if ((p == text || !isalnum (*(p - 1))) && !isalnum (*(p + ns)))
			{
				free (text);
				return 1;
			}

			t = p + 1;
		}

		n = strtok (NULL, ",");
	}
	free (text);
	return 0;
}

static int
is_hilight (char *from, char *text, char *chan, session *sess, server *serv)
{
	if ((prefs.hilightnick && SearchNick (text, serv->nick)) ||
			SearchNick (text, prefs.bluestring))
	{
		if (EMIT_SIGNAL (XP_HIGHLIGHT, sess, chan, from, text, NULL, 0)
			 == 1)
			return 2;
		if (sess != current_tab && sess->is_tab)
		{
			sess->nick_said = TRUE;
			fe_set_hilight (sess);
		}
		return 1;
	}
	return 0;
}

void
channel_action (session *sess, char *tbuf, char *chan, char *from,
					 char *text, int fromme)
{
	int hilight = FALSE;
	session *def = sess;
	server *serv = sess->server;

	if (EMIT_SIGNAL (XP_CHANACTION, sess, chan, from, text, NULL, fromme) == 1)
		return;

	if (is_channel (serv, chan) || fromme)
	{
		sess = find_session_from_channel (chan, serv);
		if (!fromme && sess)
			if (fe_is_beep (sess) || prefs.beepchans)
				fe_beep ();
	}
	else
	{
        	/* it's a private action! */
		sess = find_session_from_channel (from, serv);
		if (((sess) && fe_is_beep (sess)) || prefs.beepmsg)
			fe_beep ();
	}

	if (!sess && !is_channel (serv, chan) && prefs.autodialog)
		sess = new_ircwindow (serv, from, SESS_DIALOG);

	if (!sess)
		sess = def;

	if (!fromme)
	{
		switch (is_hilight (from, text, chan, sess, serv))
		{
		case 2:
			return;	/* plugin ate the event */
		case 1:
			hilight = TRUE;
		}
	}

	sess->highlight_tab = TRUE;

	if (hilight)
	{
		EMIT_SIGNAL (XP_TE_HCHANACTION, sess, from, text, NULL, NULL, 0);
	} else if (prefs.colorednicks)
	{
		sprintf (tbuf, "\003%d%s", color_of (from), from);
		EMIT_SIGNAL (XP_TE_CHANACTION, sess, tbuf, text, NULL, NULL, 0);
	} else
	{
		EMIT_SIGNAL (XP_TE_CHANACTION, sess, from, text, NULL, NULL, 0);
	}
}

void
channel_msg (struct server *serv, char *outbuf, char *chan, char *from,
				 char *text, char fromme)
{
	struct User *user;
	struct session *sess;
	int hilight = FALSE;
	char nickchar[2] = "\000";

	sess = find_session_from_channel (chan, serv);
	if (!sess)
		return;

	sess->highlight_tab = TRUE;

	user = find_name (sess, from);
	if (user)
	{
		nickchar[0] = user->prefix;
		if (nickchar[0] == ' ')
			nickchar[0] = 0;
		user->lasttalk = time (0);
	}

	if (EMIT_SIGNAL (XP_CHANMSG, serv, chan, from, text, NULL, fromme) == 1)
		return;

	if (!fromme)
	{
		switch (is_hilight (from, text, chan, sess, serv))
		{
		case 2:
			return;	/* plugin ate the event */
		case 1:
			hilight = TRUE;
		}
	}

	if (fromme)
		EMIT_SIGNAL (XP_TE_UCHANMSG, sess, from, text, nickchar, NULL, 0);
	else if (sess->type == SESS_DIALOG)
		EMIT_SIGNAL (XP_TE_DPRIVMSG, sess, from, text, nickchar, NULL, 0);
	else if (hilight)
		EMIT_SIGNAL (XP_TE_HCHANMSG, sess, from, text, nickchar, NULL, 0);
	else if (prefs.colorednicks)
	{
		sprintf (outbuf, "\003%d%s", color_of (from), from);
		EMIT_SIGNAL (XP_TE_CHANMSG, sess, outbuf, text, nickchar, NULL, 0);
	}
	else
		EMIT_SIGNAL (XP_TE_CHANMSG, sess, from, text, nickchar, NULL, 0);

	if (!fromme && sess->type != SESS_DIALOG)
		if (fe_is_beep (sess) || prefs.beepchans)
			fe_beep ();
}

void
user_new_nick (struct server *serv, char *nick, char *newnick, int quiet)
{
	int me;
	struct session *sess;
	GSList *list = sess_list;

	if (*newnick == ':')
		newnick++;
	if (!strcasecmp (nick, serv->nick))
	{
		me = TRUE;
		safe_strcpy (serv->nick, newnick, NICKLEN);
	} else
		me = FALSE;

	if (EMIT_SIGNAL (XP_CHANGENICK, serv, nick, newnick, NULL, NULL, me) == 1)
		return;

	while (list)
	{
		sess = (struct session *) list->data;
		if (sess->server == serv)
		{
			if (me || find_name (sess, nick))
			{
				if (!quiet)
				{
					if (me)
						EMIT_SIGNAL (XP_TE_UCHANGENICK, sess, nick, newnick, NULL,
										 NULL, 0);
					else
						EMIT_SIGNAL (XP_TE_CHANGENICK, sess, nick, newnick, NULL,
										 NULL, 0);
				}
				change_nick (sess, nick, newnick);
			}
			if (!strcasecmp (sess->channel, nick))
			{
				safe_strcpy (sess->channel, newnick, CHANLEN);
				fe_set_channel (sess);
			}
			fe_set_title (sess);
		}
		list = list->next;
	}

	fe_change_nick (serv, nick, newnick);
	dcc_change_nick (serv, nick, newnick);

	if (me)
		fe_set_nick (serv, newnick);
}

/* find a "<none>" tab */
static session *
find_unused_session (server *serv)
{
	session *sess;
	GSList *list = sess_list;
	while (list)
	{
		sess = (session *) list->data;
		if (sess->type == SESS_CHANNEL && sess->channel[0] == 0 &&
			 sess->server == serv)
		{
			if (!prefs.persist_chans || sess->waitchannel[0] == 0)
				return sess;
		}
		list = list->next;
	}
	return 0;
}

static session *
find_session_from_waitchannel (char *chan, struct server *serv)
{
	struct session *sess;
	GSList *list = sess_list;
	while (list)
	{
		sess = (struct session *) list->data;
		if (sess->server == serv && sess->channel[0] == 0 && sess->type == SESS_CHANNEL)
		{
			if (!strcasecmp (chan, sess->waitchannel))
				return sess;
		}
		list = list->next;
	}
	return 0;
}

static void
you_joined (server *serv, char *outbuf, char *chan, char *nick, char *ip)
{
	session *sess;

	/* already joined? probably a bnc */
	sess = find_session_from_channel (chan, serv);
	if (!sess)
	{
		/* see if a window is waiting to join this channel */
		sess = find_session_from_waitchannel (chan, serv);
		if (!sess)
		{
			/* find a "<none>" tab and use that */
			sess = find_unused_session (serv);
			if (!sess)
				/* last resort, open a new tab/window */
				sess = new_ircwindow (serv, NULL, SESS_CHANNEL);
		}
	}

	safe_strcpy (sess->channel, chan, CHANLEN);

	fe_set_channel (sess);
	fe_set_title (sess);
	fe_set_nonchannel (sess, TRUE);
	clear_user_list (sess);

	if (prefs.logging)
		setup_logging (sess);

	sess->waitchannel[0] = 0;
	sess->ignore_date = TRUE;
	sess->ignore_mode = TRUE;
	sess->ignore_names = TRUE;
	sess->end_of_names = FALSE;

	sprintf (outbuf, "MODE %s\r\n", chan);
	tcp_send (sess->server, outbuf);

	EMIT_SIGNAL (XP_TE_UJOIN, sess, nick, chan, ip, NULL, 0);

	if (prefs.userhost)
	{
		sprintf (outbuf, "WHO %s\r\n", sess->channel);
		tcp_send (serv, outbuf);
		sess->doing_who = TRUE;
	}
}

static void
you_kicked (struct server *serv, char *tbuf, char *chan, char *kicker,
				char *reason)
{
	struct session *sess = find_session_from_channel (chan, serv);
	if (sess)
	{
		EMIT_SIGNAL (XP_TE_UKICK, sess, serv->nick, chan, kicker, reason, 0);
		clear_channel (sess);
		if (prefs.autorejoin)
		{
			if (sess->channelkey[0] == '\0')
				sprintf (tbuf, "JOIN %s\r\n", chan);
			else
				sprintf (tbuf, "JOIN %s %s\r\n", chan, sess->channelkey);
			tcp_send (sess->server, tbuf);
			safe_strcpy (sess->waitchannel, chan, CHANLEN);
		}
	}
}

static void
you_parted (struct server *serv, char *chan, char *ip, char *reason)
{
	struct session *sess = find_session_from_channel (chan, serv);
	if (sess)
	{
		if (*reason)
			EMIT_SIGNAL (XP_TE_UPARTREASON, sess, serv->nick, ip, chan, reason,
							 0);
		else
			EMIT_SIGNAL (XP_TE_UPART, sess, serv->nick, ip, chan, NULL, 0);
		clear_channel (sess);
	}
}

static void
names_list (server *serv, char *chan, char *names)
{
	struct session *sess;
	char name[NICKLEN];
	int pos = 0;

	sess = find_session_from_channel (chan, serv);
	if (!sess)
	{
		EMIT_SIGNAL (XP_TE_USERSONCHAN, serv->front_session, chan, names, NULL,
						 NULL, 0);
		return;
	}
	if (!sess->ignore_names)
		EMIT_SIGNAL (XP_TE_USERSONCHAN, sess, chan, names, NULL, NULL, 0);

	if (sess->end_of_names)
	{
		sess->end_of_names = FALSE;
		clear_user_list (sess);
	}

	while (1)
	{
		switch (*names)
		{
		case 0:
			name[pos] = 0;
			if (pos != 0)
				add_name (sess, name, 0);
			return;
		case ' ':
			name[pos] = 0;
			pos = 0;
			add_name (sess, name, 0);
			break;
		default:
			name[pos] = *names;
			if (pos < (NICKLEN-1))
				pos++;
		}
		names++;
	}
}

static void
topic (server *serv, char *buf)
{
	session *sess;
	char *po, *new_topic;

	po = strchr (buf, ' ');
	if (po)
	{
		po[0] = 0;
		sess = find_session_from_channel (buf, serv);
		if (sess)
		{
			new_topic = strip_color (po + 2);
			set_topic (sess, new_topic);
			free (new_topic);
			EMIT_SIGNAL (XP_TE_TOPIC, sess, buf, po + 2, NULL, NULL, 0);
		}
	}
}

static void
new_topic (server *serv, char *nick, char *chan, char *topic)
{
	struct session *sess = find_session_from_channel (chan, serv);
	if (sess)
	{
		set_topic (sess, topic);
		EMIT_SIGNAL (XP_TE_NEWTOPIC, sess, nick, topic, chan, NULL, 0);
	}
}

static void
user_joined (server *serv, char *chan, char *user, char *ip)
{
	struct session *sess = find_session_from_channel (chan, serv);

	if (EMIT_SIGNAL (XP_JOIN, serv, chan, user, ip, NULL, 0) == 1)
		return;

	if (sess)
	{
		if (!fe_is_confmode (sess))
			EMIT_SIGNAL (XP_TE_JOIN, sess, user, chan, ip, NULL, 0);
		add_name (sess, user, ip);
	}
}

static void
user_kicked (server *serv, char *chan, char *user, char *kicker, char *reason)
{
	struct session *sess = find_session_from_channel (chan, serv);
	if (sess)
	{
		EMIT_SIGNAL (XP_TE_KICK, sess, kicker, user, chan, reason, 0);
		sub_name (sess, user);
	}
}

static void
user_parted (struct server *serv, char *chan, char *user, char *ip,
				 char *reason)
{
	struct session *sess = find_session_from_channel (chan, serv);
	if (sess)
	{
		if (!fe_is_confmode (sess))
		{
			if (*reason)
				EMIT_SIGNAL (XP_TE_PARTREASON, sess, user, ip, chan, reason, 0);
			else
				EMIT_SIGNAL (XP_TE_PART, sess, user, ip, chan, NULL, 0);
		}
		sub_name (sess, user);
	}
}

static void
channel_date (session *sess, char *chan, char *timestr)
{
	long n = atol (timestr);
	char *tim = ctime (&n);
	tim[19] = 0;	/* get rid of the \n */
	EMIT_SIGNAL (XP_TE_CHANDATE, sess, chan, tim, NULL, NULL, 0);
}

static void
topic_nametime (server *serv, char *chan, char *nick, char *date)
{
	long n = atol (date);
	char *tim = ctime (&n);
	struct session *sess = find_session_from_channel (chan, serv);
	tim[19] = 0;	/* get rid of the \n */
	EMIT_SIGNAL (XP_TE_TOPICDATE, sess, chan, nick, tim, NULL, 0);
}

void
set_server_name (struct server *serv, char *name)
{
	GSList *list = sess_list;
	struct session *sess;

	if (name[0] == 0)
		name = serv->hostname;

	strcpy (serv->servername, name);
	while (list)
	{
		sess = (struct session *) list->data;
		if (sess->server == serv)
			fe_set_title (sess);
		list = list->next;
	}
	if (serv->front_session->type == SESS_SERVER)
	{
		safe_strcpy (serv->front_session->channel, name, CHANLEN);
		fe_set_channel (serv->front_session);
	}
}

static void
user_quit (server *serv, char *nick, char *ip, char *reason)
{
	GSList *list = sess_list;
	struct session *sess;
	int was_on_front_session = FALSE;

	while (list)
	{
		sess = (struct session *) list->data;
		if (sess->server == serv)
		{
 			if (sess == menu_sess)
 				was_on_front_session = TRUE;
			if (sub_name (sess, nick))
			{
				if (!fe_is_confmode (sess))
					EMIT_SIGNAL (XP_TE_QUIT, sess, nick, reason, ip, NULL, 0);
			} else if (sess->type == SESS_DIALOG && !strcasecmp (sess->channel, nick))
			{
				EMIT_SIGNAL (XP_TE_QUIT, sess, nick, reason, ip, NULL, 0);
			}
		}
		list = list->next;
	}

	notify_set_offline (serv, nick, was_on_front_session);
}

static void
got_ping_reply (struct session *sess, char *outbuf,
					 char *timestring, char *from)
{
	unsigned long tim, nowtim, dif;
	int lag = 0;

	if (strncmp (timestring, "LAG", 3) == 0)
	{
		timestring += 3;
		lag = 1;
	}

	sscanf (timestring, "%lu", &tim);
	nowtim = make_ping_time ();
	dif = nowtim - tim;

	sess->server->ping_recv = time (0);

	if (lag)
	{
		sess->server->lag_sent = 0;
		sprintf (outbuf, "%ld.%ld", dif / 1000000, (dif / 100000) % 10);
		fe_set_lag (sess->server, (int)((float)atof (outbuf) * 10.0));
		return;
	}

	if (atol (timestring) == 0)
	{
		if (sess->server->lag_sent)
			sess->server->lag_sent = 0;
		else
			EMIT_SIGNAL (XP_TE_PINGREP, sess, from, "?", NULL, NULL, 0);
	} else
	{
		sprintf (outbuf, "%ld.%ld%ld", dif / 1000000, (dif / 100000) % 10, dif % 10);
		EMIT_SIGNAL (XP_TE_PINGREP, sess, from, outbuf, NULL, NULL, 0);
	}
}

static session *
find_session_from_type (int type, server *serv)
{
	session *sess;
	GSList *list = sess_list;
	while (list)
	{
		sess = list->data;
		if (sess->type == type && serv == sess->server)
			return sess;
		list = list->next;
	}
	return 0;
}

static void
notice (struct server *serv, char *outbuf, char *to, char *nick, char *msg,
		  char *ip)
{
	char *po,*ptr=to;
	struct session *sess = 0;

	if (is_channel (serv, ptr))
		sess = find_session_from_channel (ptr, serv);

	if (!sess && ptr[0] == '@')
	{
		ptr++;
		sess = find_session_from_channel (ptr, serv);
	}
	
	if (!sess)
	{
		ptr = 0;
		if (prefs.notices_tabs) {
			int stype = strcmp(nick, ip) ? SESS_NOTICES : SESS_SNOTICES;
			sess = find_session_from_type (stype, serv);
			if (!sess)
			{
				register unsigned int oldh = prefs.hideuserlist;
				prefs.hideuserlist = 1;
				if (stype == SESS_NOTICES)
					sess = new_ircwindow (serv, "(notices)", SESS_NOTICES);
				else
					sess = new_ircwindow (serv, "(snotices)", SESS_SNOTICES);
				prefs.hideuserlist = oldh;
				fe_set_channel (sess);
				fe_set_title (sess);
				fe_set_nonchannel (sess, FALSE);
				clear_user_list (sess);
				if (prefs.logging)
					setup_logging (sess);
			}
			/* Avoid redundancy with some Undernet notices */
			if (!strncmp (msg, "*** Notice -- ", 14))
				msg += 14;
		} else
		{
			sess = find_session_from_nick (nick, serv);
		}
		if (!sess)
			sess = serv->front_session;
	}

	if (msg[0] == 1)
	{
		msg++;
		if (!strncmp (msg, "PING", 4))
		{
			got_ping_reply (sess, outbuf, msg + 5, nick);
			return;
		}
	}
	po = strchr (msg, '\001');
	if (po)
		po[0] = 0;
	if (ptr)
		EMIT_SIGNAL (XP_TE_CHANNOTICE, sess, nick, to, msg, NULL, 0);
	else
		EMIT_SIGNAL (XP_TE_NOTICE, sess, nick, msg, NULL, NULL, 0);
}

static void
handle_away (server *serv, char *nick, char *msg)
{
	struct away_msg *away = find_away_message (serv, nick);
	struct session *sess = NULL;

	if (away && !strcmp (msg, away->message))	/* Seen the msg before? */
	{
		if (prefs.show_away_once && !serv->inside_whois)
			return;
	} else
	{
		save_away_message (serv, nick, msg);
	}

	if (!serv->inside_whois)
		sess = find_session_from_nick (nick, serv);
	if (!sess)
		sess = serv->front_session;

	EMIT_SIGNAL (XP_TE_WHOIS5, sess, nick, msg, NULL, NULL, 0);
}

static int
end_of_names (server *serv, char *chan)
{
	struct session *sess;
	GSList *list;

	if (!strcmp (chan, "*"))
	{
		list = sess_list;
		while (list)
		{
			sess = (struct session *) list->data;
			if (sess->server == serv)
			{
				sess->end_of_names = TRUE;
				sess->ignore_names = FALSE;
			}
			list = list->next;
		}
		return TRUE;
	}
	sess = find_session_from_channel (chan, serv);
	if (sess)
	{
		sess->end_of_names = TRUE;
		sess->ignore_names = FALSE;
		return TRUE;
	}
	return FALSE;
}

static void
check_willjoin_channels (struct server *serv, char *tbuf)
{
	char *po;
	struct session *sess;
	GSList *list = sess_list;
	while (list)
	{
		sess = (struct session *) list->data;
		if (sess->server == serv)
		{
			if (sess->willjoinchannel[0] != 0)
			{
				strcpy (sess->waitchannel, sess->willjoinchannel);
				sess->willjoinchannel[0] = 0;
				if (sess->channelkey[0] == '\0')
					sprintf (tbuf, "JOIN %s\r\n", sess->waitchannel);
				else
					sprintf (tbuf, "JOIN %s %s\r\n", sess->waitchannel,
								sess->channelkey);
				tcp_send (serv, tbuf);
				po = strchr (sess->waitchannel, ',');
				if (po)
					*po = 0;
			}
		}
		list = list->next;
	}
}

static void
next_nick (struct session *sess, char *outbuf, char *nick)
{
	sess->server->nickcount++;

	switch (sess->server->nickcount)
	{
	case 2:
		sprintf (outbuf, "NICK %s\r\n", prefs.nick2);
		tcp_send (sess->server, outbuf);
		EMIT_SIGNAL (XP_TE_NICKCLASH, sess, nick, prefs.nick2, NULL, NULL, 0);

		break;

	case 3:
		sprintf (outbuf, "NICK %s\r\n", prefs.nick3);
		tcp_send (sess->server, outbuf);
		EMIT_SIGNAL (XP_TE_NICKCLASH, sess, nick, prefs.nick3, NULL, NULL, 0);

		break;

	default:
		EMIT_SIGNAL (XP_TE_NICKFAIL, sess, NULL, NULL, NULL, NULL, 0);
	}
}

void
do_dns (struct session *sess, char *tbuf, char *nick, char *host)
{
	char *po;

	po = strrchr (host, '@');
	if (po)
		host = po + 1;
	EMIT_SIGNAL (XP_TE_RESOLVINGUSER, sess, nick, host, NULL, NULL, 0);
	sprintf (tbuf, "/exec %s %s", prefs.dnsprogram, host);
	handle_command (tbuf, sess, 0, 0);
}

static void
set_default_modes (server * serv, char *outbuf)
{
	int mode = FALSE;

	if (prefs.wallops)
	{
		sprintf (outbuf, "MODE %s +w", serv->nick);
		mode = TRUE;
	}
	if (prefs.servernotice)
	{
		if (mode)
			strcat (outbuf, "s");
		else
		{
			sprintf (outbuf, "MODE %s +s", serv->nick);
			mode = TRUE;
		}
	}
	if (prefs.invisible)
	{
		if (mode)
			strcat (outbuf, "i");
		else
		{
			sprintf (outbuf, "MODE %s +i", serv->nick);
			mode = TRUE;
		}
	}
	if (mode)
	{
		strcat (outbuf, "\r\n");
		tcp_send (serv, outbuf);
	}
}

static void
process_numeric (server * serv, session * sess, char *outbuf, int n,
					  char *word[], char *word_eol[], char *text)
{
	session *realsess;

	switch (n)
	{
	case 1:
		user_new_nick (serv, serv->nick, word[3], TRUE);
		set_server_name (serv, word_eol[1]);
		if (sess->type == SESS_SERVER && prefs.logging)
			setup_logging (sess);
		/* reset our away status */
		if (serv->reconnect_away)
		{
			handle_command ("/away", serv->front_session, FALSE, FALSE);
			serv->reconnect_away = FALSE;
		}
		goto def;
	case 4:	/* check the ircd type */
		serv->is_newtype = FALSE;
		serv->six_modes = FALSE;
		if (strncmp (word[5], "bahamut", 7) == 0)				/* DALNet */
		{
			serv->is_newtype = TRUE;		/* use the /list args */
		} else if (strncmp (word[5], "u2.10.", 6) == 0)		/* Undernet */
		{
			serv->is_newtype = TRUE;		/* use the /list args */
			serv->six_modes = TRUE;			/* allow 6 modes per line */
		}
		goto def;
	case 5:
		handle_005 (serv, word);
		goto def;
	case 301:
		handle_away (serv, word[4],
				(word_eol[5][0] == ':') ? word_eol[5] + 1 : word_eol[5]);
		break;
	case 303:
		word[4]++;
		notify_markonline (serv, word);
		break;
	case 305:
		serv->is_away = FALSE;
		serv->reconnect_away = FALSE;
		fe_set_away (serv);
		goto def;
	case 306:
		serv->is_away = TRUE;
		serv->away_time = time (NULL);
		fe_set_away (serv);
		goto def;
	case 312:
		EMIT_SIGNAL (XP_TE_WHOIS3, sess, word[4], word_eol[5], NULL, NULL, 0);
		break;
	case 311:
		serv->inside_whois = 1;
		/* FALL THROUGH */
	case 314:
		EMIT_SIGNAL (XP_TE_WHOIS1, sess,
						 word[4], word[5],
						 word[6], word_eol[8] + 1, 0);
		break;
	case 317:
		{
			long n = atol (word[6]);
			long idle = atol (word[5]);
			char *tim;
			sprintf (outbuf, "%02ld:%02ld:%02ld", idle / 3600, (idle / 60) % 60,
						idle % 60);
			if (n == 0)
				EMIT_SIGNAL (XP_TE_WHOIS4, serv->front_session,
								 word[4], outbuf, NULL, NULL, 0);
			else
			{
				tim = ctime (&n);
				tim[19] = 0; 	/* get rid of the \n */
				EMIT_SIGNAL (XP_TE_WHOIS4T, serv->front_session,
								 word[4], outbuf, tim, NULL, 0);
			}
		}
		break;
	case 318:
		serv->inside_whois = 0;
		EMIT_SIGNAL (XP_TE_WHOIS6, serv->front_session, word[4], NULL,
						 NULL, NULL, 0);
		break;
	case 313:
	case 319:
		EMIT_SIGNAL (XP_TE_WHOIS2, serv->front_session, word[4],
						 word_eol[5] + 1, NULL, NULL, 0);
		break;
	case 321:
		if (!fe_is_chanwindow (sess->server))
			EMIT_SIGNAL(XP_TE_CHANLISTHEAD, sess, NULL, NULL, NULL, NULL, 0);
		break;
	case 322:
		if (fe_is_chanwindow (sess->server))
		{
			fe_add_chan_list (sess->server, word[4],
									word[5],
									word_eol[6] + 1);
		} else
		{
			sprintf (outbuf, "%-16.16s %-7d %s\017\n",
						word[4],
						atoi (word[5]),
						word_eol[6] + 1);
			PrintText (sess, outbuf);
		}
		break;
	case 323:
		if (!fe_is_chanwindow (sess->server))
			goto def;
		fe_chan_list_end (sess->server);
		break;
	case 324:
		sess = find_session_from_channel (word[4], serv);
		if (!sess)
			sess = serv->front_session;
		if (sess->ignore_mode)
		{
			sess->ignore_mode = FALSE;
		} else
		{
			EMIT_SIGNAL (XP_TE_CHANMODES, sess, word[4], word_eol[5],
							 NULL, NULL, 0);
		}
		fe_update_mode_buttons (sess, 't', '-');
		fe_update_mode_buttons (sess, 'n', '-');
		fe_update_mode_buttons (sess, 's', '-');
		fe_update_mode_buttons (sess, 'i', '-');
		fe_update_mode_buttons (sess, 'p', '-');
		fe_update_mode_buttons (sess, 'm', '-');
		fe_update_mode_buttons (sess, 'l', '-');
		fe_update_mode_buttons (sess, 'k', '-');
		handle_mode (serv, outbuf, word, word_eol, "", TRUE);
		break;
	case 329:
		sess = find_session_from_channel (word[4], serv);
		if (sess)
		{
			if (sess->ignore_date)
				sess->ignore_date = FALSE;
			else
				channel_date (sess, word[4], word[5]);
		}
		break;
	case 332:
		topic (serv, text);
		break;
	case 333:
		topic_nametime (serv, word[4], word[5], word[6]);
		break;
	case 341:						  /* INVITE ACK */
		if (prefs.show_invite_in_front_session)
			if (menu_sess)
				sess = menu_sess;

		EMIT_SIGNAL (XP_TE_UINVITE, sess, word[4], word[5], serv->servername,
						 NULL, 0);
		break;
	case 352:						  /* WHO */
		if (!serv->skip_next_who)
		{
			struct session *who_sess;

			who_sess = find_session_from_channel (word[4], serv);
			if (who_sess)
			{
				sprintf (outbuf, "%s@%s", word[5], word[6]);
				if (!userlist_add_hostname
					 (who_sess, word[8], outbuf, word_eol[11], word[7]))
				{
					if (!who_sess->doing_who)
						goto def;
				}
			} else
			{
				if (serv->doing_who)
					do_dns (sess, outbuf, word[8], word[6]);
				else
					goto def;
			}
		} else
		{
			if (!strcasecmp (word[8], serv->nick))
			{
				struct hostent *HostAddr;

				HostAddr = gethostbyname (word[6]);
				if (HostAddr)
				{
					prefs.dcc_ip = ((struct in_addr *) HostAddr->h_addr)->s_addr;
					EMIT_SIGNAL (XP_TE_FOUNDIP, sess,
									 inet_ntoa (*
													((struct in_addr *) HostAddr->h_addr)),
									 NULL, NULL, NULL, 0);
				}
			}
		}
		break;
	case 315:						  /* END OF WHO */
		if (serv->skip_next_who)
		{
			serv->skip_next_who = FALSE;
		} else
		{
			struct session *who_sess;
			who_sess = find_session_from_channel (word[4], serv);
			if (who_sess)
			{
				if (who_sess->doing_who)
					who_sess->doing_who = FALSE;
				else
					goto def;
			} else
			{
				if (serv->doing_who)
					serv->doing_who = FALSE;
				else
					goto def;
			}
		}
		break;
	case 353:						  /* NAMES */
		{
			char *names, *chan;

			chan = word[5];
			names = word_eol[6];
			if (*names == ':')
				names++;
			names_list (serv, chan, names);
		}
		break;
	case 366:
		if (!end_of_names (serv, word[4]))
			goto def;
		break;
	case 367: /* banlist entry */
		{
			time_t ban_time = atol (word[7]);
			char *time_str = ctime (&ban_time);
			time_str[19] = 0;	/* get rid of the \n */
			if (ban_time == 0)
				time_str = "";
			sess = find_session_from_channel (word[4], serv);
			if (!sess)
				sess = serv->front_session;
		   if (!fe_is_banwindow (sess))
				EMIT_SIGNAL (XP_TE_BANLIST, sess, word[4], word[5], word[6], time_str, 0);
			else
				fe_add_ban_list (sess, word[5], word[6], time_str);
		}
		break;
	 case 368:
		sess = find_session_from_channel (word[4], serv);
		if (!sess)
			sess = serv->front_session;
		if (!fe_is_banwindow (sess))
			goto def;
		fe_ban_list_end (sess);
		break;
	case 376:
	case 422:						  /* end of motd */
		if (!serv->end_of_motd)
		{
			if (prefs.ip_from_server)
			{
				serv->skip_next_who = TRUE;
				sprintf (outbuf, "WHO %s\r\n", serv->nick);
				tcp_send (serv, outbuf);
			}
			set_default_modes (serv, outbuf);
			check_willjoin_channels (serv, outbuf);
			if (serv->supports_watch)
				notify_send_watches (serv);
			if (serv->eom_cmd)
			{
				handle_command (serv->eom_cmd, sess, FALSE, FALSE);
				free (serv->eom_cmd);
				serv->eom_cmd = NULL;
			}
			serv->end_of_motd = TRUE;
		}
		goto def;
	case 433:
		if (serv->end_of_motd)
			goto def;
		next_nick (sess, outbuf, word[4]);
		break;
	case 437:
		if (!is_channel (serv, word[4]) && !serv->end_of_motd)
			next_nick (sess, outbuf, word[4]);
		else
			goto def;
		break;
	case 471:
		EMIT_SIGNAL (XP_TE_USERLIMIT, sess, word[4], NULL, NULL, NULL, 0);
		break;
	case 473:
		EMIT_SIGNAL (XP_TE_INVITE, sess, word[4], NULL, NULL, NULL, 0);
		break;
	case 474:
		EMIT_SIGNAL (XP_TE_BANNED, sess, word[4], NULL, NULL, NULL, 0);
		break;
	case 475:
		EMIT_SIGNAL (XP_TE_KEYWORD, sess, word[4], NULL, NULL, NULL, 0);
		break;
	case 601:
		notify_set_offline (serv, word[4], FALSE);
		break;
	case 605:
		notify_set_offline (serv, word[4], TRUE);
		break;
	case 600:
	case 604:
		notify_set_online (serv, word[4]);
		break;

	default:
	 def:
		if (prefs.skipmotd && !serv->motd_skipped)
		{
			if (n == 375 || n == 372)
				return;
			if (n == 376)
			{
				serv->motd_skipped = TRUE;
				EMIT_SIGNAL (XP_TE_MOTDSKIP, sess, NULL, NULL, NULL, NULL, 0);
				return;
			}
		}
		if (n == 375 || n == 372 || n == 376 || n == 422)
		{
			EMIT_SIGNAL (XP_TE_MOTD, serv->front_session, text, NULL,
							 NULL, NULL, 0);
			return;
		}

		if (is_channel (serv, text))
		{
			char *chan = word[3];
			if (!strncasecmp (serv->nick, chan, strlen (serv->nick)))
				chan += strlen (serv->nick) + 1;
			if (*chan == ':')
				chan++;
			realsess = find_session_from_channel (chan, serv);
			if (!realsess)
				realsess = sess;
			EMIT_SIGNAL (XP_TE_SERVTEXT, realsess, text, NULL, NULL, NULL, 0);
		} else
		{
			EMIT_SIGNAL (XP_TE_SERVTEXT, serv->front_session, text, NULL,
							 NULL, NULL, 0);
		}
	}
}

/* process_line() */

void
process_line (struct server *serv, char *buf)
{
	session *sess;
#ifdef USE_PERL
	session *tmp_sess;
	char *type;
#endif
	char pdibuf[522];				  /* 1 line can't exceed 512 bytes!! */
	char outbuf[4096];
	char ip[128], nick[NICKLEN];
	char *word[PDIWORDS];
	char *word_eol[PDIWORDS];
	char *text, *ex, *cmd;
	int n;

	sess = serv->front_session;
	if (!sess)
	{
		/*fprintf (stderr,
					"*** XCHAT WARNING: process_line(), sess=0x0 for data: %s\n",
					buf);*/
		return;
	}

	fe_add_rawlog (serv, buf, FALSE);

	fe_checkurl (buf);

	/* split line into words and words_to_end_of_line */
	if (*buf == ':')
		process_data_init (pdibuf, buf + 1, word, word_eol, FALSE);
	else
		process_data_init (pdibuf, buf, word, word_eol, FALSE);

#ifdef USE_PERL
	/* tell perl that we got a line of data */
	if (*buf == ':' && is_channel (serv, word[3]))
		tmp_sess = find_session_from_channel (word[3], serv);
	else
		tmp_sess = sess;

	if (!tmp_sess)
		tmp_sess = sess;

	/* for server messages, the 2nd word is the "message type" */
	if (*buf == ':')
		type = word[2];
	else
		type = pdibuf;
	if (perl_inbound (tmp_sess, serv, buf, type))
		return;
#endif

	if (EMIT_SIGNAL (XP_INBOUND, sess, serv, buf, NULL, NULL, 0) == 1)
		return;

	if (*buf != ':')
	{
		if (!strncmp (buf, "NOTICE ", 7))
		{
			buf += 7;
		}
		if (!strncmp (buf, "PING ", 5))
		{
			sprintf (outbuf, "PONG %s\r\n", buf + 5);
			tcp_send (serv, outbuf);
			return;
		}
		if (!strncmp (buf, "ERROR", 5))
		{
			EMIT_SIGNAL (XP_TE_SERVERERROR, sess, buf + 7, NULL, NULL, NULL, 0);
			return;
		}
		EMIT_SIGNAL (XP_TE_SERVERGENMESSAGE, sess, buf, NULL, NULL, NULL, 0);
		return;
	}

	buf++;

	/* see if the second word is a numeric */
	n = atoi (word[2]);
	if (n)
	{
		text = word_eol[3];
		if (*text)
		{
			if (!strncasecmp (serv->nick, text, strlen (serv->nick)))
				text += strlen (serv->nick) + 1;
			if (*text == ':')
				text++;

			process_numeric (serv, sess, outbuf, n, word, word_eol, text);
		}
		return;
	}

	/* fill in the "ip" and "nick" buffers */
	ex = strchr (pdibuf, '!');
	if (!ex)							  /* no '!', must be a server message */
	{
		safe_strcpy (ip, pdibuf, sizeof (ip));
		safe_strcpy (nick, pdibuf, sizeof (nick));
	} else
	{
		safe_strcpy (ip, ex + 1, sizeof (ip));
		safe_strcpy (nick, pdibuf, sizeof (nick));
		if ((ex - pdibuf) < sizeof (nick))
			nick[ex - pdibuf] = 0; /* cut the buffer at the '!' */
	}

	cmd = word[2];

	if (!strcmp ("INVITE", cmd))
	{
		if (ignore_check (pdibuf, 0, 0, 0, 0, 1))
			return;

		if (prefs.show_invite_in_front_session)
			if (menu_sess)
				sess = menu_sess;

		EMIT_SIGNAL (XP_TE_INVITED, sess, word[4] + 1, nick, serv->servername,
						 NULL, 0);
		return;
	}
	if (!strcmp ("JOIN", cmd))
	{
		char *chan = word[3];

		if (*chan == ':')
			chan++;
		if (!strcasecmp (nick, serv->nick))
			you_joined (serv, outbuf, chan, nick, ip);
		else
			user_joined (serv, chan, nick, ip);
		return;
	}
	if (!strcmp ("MODE", cmd))
	{
		handle_mode (serv, outbuf, word, word_eol, nick, FALSE);	/* modes.c */
		return;
	}
	if (!strcmp ("NICK", cmd))
	{
		user_new_nick (serv, nick, word_eol[3], FALSE);
		return;
	}
	if (!strcmp ("NOTICE", cmd))
	{
		char *to = word[3];
		if (*to)
		{
			char *msg = word_eol[4];
			if (*msg == ':')
				msg++;
			if (prefs.fudgeservernotice && (strcmp (nick, serv->servername) == 0 || strchr (nick, '.')))
			{
				EMIT_SIGNAL (XP_TE_SERVERGENMESSAGE, sess, msg, NULL,
								 NULL, NULL, 0);
			} else
			{
				if (ignore_check (pdibuf, 0, 1, 0, 0, 0))
					return;
				notice (serv, outbuf, to, nick, msg, ip);
			}
			return;
		}
	}
	if (!strcmp ("PART", cmd))
	{
		char *chan = cmd + 5;
		char *reason = word_eol[4];

		if (*chan == ':')
			chan++;
		if (*reason == ':')
			reason++;
		if (!strcmp (nick, serv->nick))
			you_parted (serv, chan, ip, reason);
		else
			user_parted (serv, chan, nick, ip, reason);
		return;
	}
	if (!strcmp ("PRIVMSG", cmd))
	{
		char *to = word[3];
		if (*to)
		{
			char *msg = word_eol[4];
			if (*msg == ':')
				msg++;
			if (msg[0] == 1 && msg[strlen (msg) - 1] == 1)	/* ctcp */
			{
				if (strncasecmp (msg + 1, "ACTION", 6) != 0)
					flood_check (nick, ip, serv, sess, 0);
				if (ignore_check (pdibuf, 0, 0, 0, 1, 0))
					return;
				if (strncasecmp (msg + 1, "DCC ", 4) == 0)
					/* redo this with handle_quotes TRUE */
					process_data_init (pdibuf, buf, word, word_eol, TRUE);
				handle_ctcp (sess, outbuf, to, nick, msg + 1, word, word_eol);
			} else
			{
				if (is_channel (serv, to))
				{
					if (ignore_check (pdibuf, 0, 0, 1, 0, 0))
						return;
					channel_msg (serv, outbuf, to, nick, msg, FALSE);
				} else
				{
					if (ignore_check (pdibuf, 1, 0, 0, 0, 0))
						return;
					private_msg (serv, outbuf, nick, ip, msg);
				}
			}
			return;
		}
	}
	if (!strcmp ("PONG", cmd))
	{
		char *timestring = word[4];
		if (*timestring == ':')
			timestring++;
		got_ping_reply (serv->front_session, outbuf,
								timestring, word[3]);
		return;
	}
	if (!strcmp ("QUIT", cmd))
	{
		user_quit (serv, nick, ip,
			(word_eol[3][0] == ':') ? word_eol[3] + 1 : word_eol[3]);
		return;
	}
	if (!strcmp ("TOPIC", cmd))
	{
		new_topic (serv, nick, word[3],
			(word_eol[4][0] == ':') ? word_eol[4] + 1 : word_eol[4]);
		return;
	}
	if (!strcmp ("KICK", cmd))
	{
		char *kicked = word[4];
		char *reason = word_eol[5];
		if (*kicked)
		{
			if (*reason == ':')
				reason++;
			if (!strcmp (kicked, serv->nick))
 				you_kicked (serv, outbuf, word[3], nick, reason);
			else
				user_kicked (serv, word[3], kicked, nick, reason);
			return;
		}
	}
	if (!strcmp ("KILL", cmd))
	{
		EMIT_SIGNAL (XP_TE_KILL, sess, nick, word_eol[5], NULL, NULL, 0);
		return;
	}
	if (!strcmp ("WALLOPS", cmd))
	{
		text = word_eol[3];
		if (*text == ':')
			text++;
		EMIT_SIGNAL (XP_TE_WALLOPS, sess, nick, text, NULL, NULL, 0);
		return;
	}
	sprintf (outbuf, "(%s/%s) %s\n", nick, ip, word_eol[2]);
	PrintText (sess, outbuf);
}
