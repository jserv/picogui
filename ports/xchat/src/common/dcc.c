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
 *
 * Wayne Conrad, 3 Apr 1999: Color-coded DCC file transfer status windows
 * Bernhard Valenti <bernhard.valenti@gmx.net> 2000-11-21: Fixed DCC send behind nat
 *
 * 2001-03-08 Added support for getting "dcc_ip" config parameter.
 * Jim Seymour (jseymour@LinxNet.com)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#define WANTSOCKET
#define WANTARPA
#include "inet.h"

#include "xchat.h"
#include "perlc.h"
#include "util.h"
#include "plugin.h"
#include "fe.h"
#include "outbound.h"
#include "inbound.h"
#include "network.h"
#include "text.h"
#include "xchatc.h"


char *dcctypes[] = { "SEND", "RECV", "CHAT", "CHAT" };

struct dccstat_info dccstat[] = {
	{N_("Waiting"), 1 /*black */ },
	{N_("Active"), 12 /*cyan */ },
	{N_("Failed"), 4 /*red */ },
	{N_("Done"), 3 /*green */ },
	{N_("Connect"), 1 /*black */ },
	{N_("Aborted"), 4 /*red */ },
};

static struct DCC *new_dcc (void);


static void
dcc_calc_cps (struct DCC *dcc)
{
	time_t sec;

	sec = time (0) - dcc->starttime;
	if (sec < 1)
		sec = 1;
	if (dcc->type == TYPE_SEND)
		dcc->cps = (dcc->ack - dcc->resumable) / sec;
	else
		dcc->cps = (dcc->pos - dcc->resumable) / sec;
}

/* this is called from xchat.c:xchat_misc_checks() every 2 seconds. */

void
dcc_check_timeouts (void)
{
	struct DCC *dcc;
	time_t tim = time (0);
	GSList *next, *list = dcc_list;

	while (list)
	{
		dcc = (struct DCC *) list->data;
		next = list->next;

		if (dcc->dccstat == STAT_ACTIVE)
		{
			dcc_calc_cps (dcc);

			switch (dcc->type)
			{
			case TYPE_SEND:
				fe_dcc_update_send (dcc);
				break;
			case TYPE_RECV:
				fe_dcc_update_recv (dcc);
				break;
			}
		}

		switch (dcc->dccstat)
		{
		case STAT_ACTIVE:
			if (dcc->type == TYPE_SEND || dcc->type == TYPE_RECV)
			{
				if (prefs.dccstalltimeout > 0)
				{
					if (tim - dcc->lasttime > prefs.dccstalltimeout)
					{
						EMIT_SIGNAL (XP_TE_DCCSTALL, dcc->serv->front_session,
										 dcctypes[(int) dcc->type],
										 file_part (dcc->file), dcc->nick, NULL, 0);
						dcc_close (dcc, 0, TRUE);
					}
				}
			}
			break;
		case STAT_QUEUED:
			if (dcc->type == TYPE_SEND || dcc->type == TYPE_CHATSEND)
			{
				if (tim - dcc->offertime > prefs.dcctimeout)
				{
					if (prefs.dcctimeout > 0)
					{
						EMIT_SIGNAL (XP_TE_DCCTOUT, dcc->serv->front_session,
										 dcctypes[(int) dcc->type],
										 file_part (dcc->file), dcc->nick, NULL, 0);
						dcc_close (dcc, 0, TRUE);
					}
				}
			}
			break;
		}
		list = next;
	}
}

static int
dcc_connect_sok (struct DCC *dcc)
{
	int sok;
	struct sockaddr_in addr;

	sok = socket (AF_INET, SOCK_STREAM, 0);
	if (sok == -1)
		return -1;

	memset (&addr, 0, sizeof (addr));
	addr.sin_port = htons (dcc->port);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl (dcc->addr);

	set_nonblocking (sok);
	connect (sok, (struct sockaddr *) &addr, sizeof (addr));

	return sok;
}

static void
update_dcc_window (int type)
{
	switch (type)
	{
	case TYPE_SEND:
		fe_dcc_update_send_win ();
		break;
	case TYPE_RECV:
		fe_dcc_update_recv_win ();
		break;
	case TYPE_CHATRECV:
	case TYPE_CHATSEND:
		fe_dcc_update_chat_win ();
		break;
	}
}

void
dcc_close (struct DCC *dcc, int dccstat, int destroy)
{
	char type = dcc->type;

	if (dcc->wiotag)
	{
		fe_input_remove (dcc->wiotag);
		dcc->wiotag = 0;
	}

	if (dcc->iotag)
	{
		fe_input_remove (dcc->iotag);
		dcc->iotag = 0;
	}

	if (dcc->sok != -1)
	{
		closesocket (dcc->sok);
		dcc->sok = -1;
	}

	if (dcc->fp != -1)
	{
		close (dcc->fp);
		dcc->fp = -1;
	}

	dcc->dccstat = dccstat;
	if (dcc->dccchat)
	{
		free (dcc->dccchat);
		dcc->dccchat = NULL;
	}

	if (destroy)
	{
		dcc_list = g_slist_remove (dcc_list, dcc);
		if (dcc->file)
			free (dcc->file);
		if (dcc->destfile)
			free (dcc->destfile);
		free (dcc->nick);
		free (dcc);
		update_dcc_window (type);
		return;
	}

	switch (type)
	{
	case TYPE_SEND:
		fe_dcc_update_send (dcc);
		break;
	case TYPE_RECV:
		fe_dcc_update_recv (dcc);
		break;
	default:
		update_dcc_window (type);
	}
}

void
dcc_notify_kill (struct server *serv)
{
	struct server *replaceserv = 0;
	struct DCC *dcc;
	GSList *list = dcc_list;
	if (serv_list)
		replaceserv = (struct server *) serv_list->data;
	while (list)
	{
		dcc = (struct DCC *) list->data;
		if (dcc->serv == serv)
			dcc->serv = replaceserv;
		list = list->next;
	}
}

struct DCC *
dcc_write_chat (char *nick, char *text)
{
#ifdef USE_TRANS
	unsigned char *tbuf;
#define TRANS_STAT_BUF_LEN 1024
	static unsigned char sbuf[TRANS_STAT_BUF_LEN];
#endif
	struct DCC *dcc;
	int len;

	dcc = find_dcc (nick, "", TYPE_CHATRECV);
	if (!dcc)
		dcc = find_dcc (nick, "", TYPE_CHATSEND);
	if (dcc && dcc->dccstat == STAT_ACTIVE)
	{
		len = strlen (text);
		dcc->size += len;
#ifdef USE_TRANS
		if (prefs.use_trans)
		{
			if (len >= TRANS_STAT_BUF_LEN)
				tbuf = malloc (len + 1);
			else
				tbuf = sbuf;
			if (!tbuf)
			{
				return 0;
			}
			strcpy (tbuf, text);
			user2serv (tbuf);
			send (dcc->sok, tbuf, len, 0);
			if (tbuf != sbuf)
			{
				free (tbuf);
			}
		} else
		{
#endif
			send (dcc->sok, text, len, 0);
#ifdef USE_TRANS
		}
#endif
		send (dcc->sok, "\n", 1, 0);
		fe_dcc_update_chat_win ();
		return dcc;
	}
	return 0;
}

/* returns: 0 - ok
				1 - the dcc is closed! */

static int
dcc_chat_line (struct DCC *dcc, char *line, char *tbuf)
{
#ifdef USE_PERL
	int skip;
	char *host_n_nick_n_message;
	GSList *list = dcc_list;

	host_n_nick_n_message = malloc (strlen (dcc->nick) + strlen (line) + 26);

	sprintf (host_n_nick_n_message, "%s %d %s: %s",
				net_ip (dcc->addr), dcc->port, dcc->nick, line);

	skip = perl_dcc_chat (find_session_from_channel (dcc->nick, dcc->serv),
								 dcc->serv, host_n_nick_n_message);
	free (host_n_nick_n_message);

	/* Perl code may have closed the chat. */
	while (list) 
	{
		if (((struct DCC *)list->data) == dcc) break;
		list = list->next;
	}
	if (list == 0)
		return 1;

	if (skip)
		return 0;
#endif
	fe_checkurl (line);

	if (line[0] == 1 && !strncasecmp (line + 1, "ACTION", 6))
	{
		session *sess;
		char *po = strchr (line + 8, '\001');
		if (po)
			po[0] = 0;
		sess = find_session_from_channel (dcc->nick, dcc->serv);
		if (!sess)
			sess = dcc->serv->front_session;
		channel_action (sess, tbuf, dcc->serv->nick, dcc->nick,
							 line + 8, FALSE);
	} else
	{
#ifdef USE_TRANS
		if (prefs.use_trans)
			serv2user (line);
#endif
		private_msg (dcc->serv, tbuf, dcc->nick, "", line);
	}
	return 0;
}

static gboolean
dcc_read_chat (GIOChannel *source, GIOCondition condition, struct DCC *dcc)
{
	int i, len, dead;
	char tbuf[1226];
	char lbuf[1026];
	unsigned char *temp;

	while (1)
	{
		len = recv (dcc->sok, lbuf, sizeof (lbuf) - 2, 0);
		if (len < 1)
		{
			if (len < 0)
			{
				if (would_block_again ())
					return TRUE;
			}
			sprintf (tbuf, "%d", dcc->port);
			EMIT_SIGNAL (XP_TE_DCCCHATF, dcc->serv->front_session, dcc->nick,
							 net_ip (dcc->addr), tbuf, NULL, 0);
			dcc_close (dcc, STAT_FAILED, FALSE);
			return TRUE;
		}
		i = 0;
		lbuf[len] = 0;
		while (i < len)
		{
			switch (lbuf[i])
			{
			case '\r':
				break;
			case '\n':
				dcc->dccchat->linebuf[dcc->dccchat->pos] = 0;

				if (prefs.stripcolor)
				{
					temp = strip_color (dcc->dccchat->linebuf);
					dead = dcc_chat_line (dcc, temp, tbuf);
					free (temp);
				} else
				{
					dead = dcc_chat_line (dcc, dcc->dccchat->linebuf, tbuf);
				}

				if (dead) /* the dcc has been closed, don't use (DCC *)! */
					return TRUE;

				dcc->pos += dcc->dccchat->pos;
				dcc->dccchat->pos = 0;
				fe_dcc_update_chat_win ();
				break;
			default:
				dcc->dccchat->linebuf[dcc->dccchat->pos] = lbuf[i];
				if (dcc->dccchat->pos < 1022)
					dcc->dccchat->pos++;
			}
			i++;
		}
	}
}

static gboolean
dcc_read (GIOChannel *source, GIOCondition condition, struct DCC *dcc)
{
	char buf[4096];
	guint32 pos;
	int n;

	if (dcc->fp == -1)
	{
		if (dcc->resumable)
		{
			dcc->fp = open (dcc->destfile, O_WRONLY | O_APPEND | OFLAGS);
			dcc->pos = dcc->resumable;
			dcc->ack = dcc->resumable;
		} else
		{
			if (access (dcc->destfile, F_OK) == 0)
			{
				n = 0;
				do
				{
					n++;
					sprintf (buf, "%s.%d", dcc->destfile, n);
				}
				while (access (buf, F_OK) == 0);
				EMIT_SIGNAL (XP_TE_DCCRENAME, dcc->serv->front_session,
								 dcc->destfile, buf, NULL, NULL, 0);
				free (dcc->destfile);
				dcc->destfile = strdup (buf);
			}
			dcc->fp =
				open (dcc->destfile, OFLAGS | O_TRUNC | O_WRONLY | O_CREAT,
						prefs.dccpermissions);
		}
	}
	if (dcc->fp == -1)
	{
		EMIT_SIGNAL (XP_TE_DCCFILEERR, dcc->serv->front_session, dcc->destfile,
						 NULL, NULL, NULL, 0);
		dcc_close (dcc, STAT_FAILED, FALSE);
		return TRUE;
	}
	while (1)
	{
		n = recv (dcc->sok, buf, sizeof (buf), 0);
		if (n < 1)
		{
			if (n < 0)
			{
				if (would_block_again ())
					return TRUE;
			}
			EMIT_SIGNAL (XP_TE_DCCRECVERR, dcc->serv->front_session, dcc->file,
							 dcc->destfile, dcc->nick, NULL, 0);
			dcc_close (dcc, STAT_FAILED, FALSE);
			return TRUE;
		}

		write (dcc->fp, buf, n);
		dcc->pos += n;
		pos = htonl (dcc->pos);
		send (dcc->sok, (char *) &pos, 4, 0);

		dcc->lasttime = time (0);

		dcc_calc_cps (dcc);

		if (dcc->pos >= dcc->size)
		{
			sprintf (buf, "%d", dcc->cps);
			dcc_close (dcc, STAT_DONE, FALSE);
			EMIT_SIGNAL (XP_TE_DCCRECVCOMP, dcc->serv->front_session,
							 dcc->file, dcc->destfile, dcc->nick, buf, 0);
			return TRUE;
		}
	}
}

static gboolean
dcc_connect_finished (GIOChannel *source, GIOCondition condition, struct DCC *dcc)
{
	int er, sok = dcc->sok;
	char host[128];
	struct sockaddr_in addr;

	fe_input_remove (dcc->iotag);
	dcc->iotag = 0;

	memset (&addr, 0, sizeof (addr));
	addr.sin_port = htons (dcc->port);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl (dcc->addr);

	/* check if it's already connected */
	if (connect (sok, (struct sockaddr *) &addr, sizeof (addr)) != 0)
	{
		er = sock_error ();
#ifndef WIN32
		if (er != EISCONN)
#else
		if (er != WSAEISCONN)
#endif
		{
			EMIT_SIGNAL (XP_TE_DCCCONFAIL, dcc->serv->front_session,
							 dcctypes[(int) dcc->type], dcc->nick, errorstring (er),
							 NULL, 0);
			dcc->dccstat = STAT_FAILED;
			update_dcc_window (dcc->type);
			return TRUE;
		}
	}

	dcc->dccstat = STAT_ACTIVE;
	switch (dcc->type)
	{
	case TYPE_RECV:
		dcc->iotag = fe_input_add (dcc->sok, 1, 0, 1, dcc_read, dcc);
		break;

	case TYPE_CHATRECV:
		dcc->iotag = fe_input_add (dcc->sok, 1, 0, 1, dcc_read_chat, dcc);
		dcc->dccchat = malloc (sizeof (struct dcc_chat));
		dcc->dccchat->pos = 0;
		break;
	}
	update_dcc_window (dcc->type);
	dcc->starttime = time (0);
	dcc->lasttime = dcc->starttime;

	snprintf (host, sizeof host, "%s:%d", net_ip (dcc->addr), dcc->port);
	EMIT_SIGNAL (XP_TE_DCCCON, dcc->serv->front_session,
					 dcctypes[(int) dcc->type], dcc->nick, host, "to", 0);

	return TRUE;
}

static void
dcc_connect (struct DCC *dcc)
{
	if (dcc->dccstat == STAT_CONNECTING)
		return;
	dcc->dccstat = STAT_CONNECTING;
	dcc->sok = dcc_connect_sok (dcc);
	if (dcc->sok == -1)
	{
		dcc->dccstat = STAT_FAILED;
		update_dcc_window (dcc->type);
		return;
	}
	dcc->iotag = fe_input_add (dcc->sok, 0, 1, 1, dcc_connect_finished, dcc);
	if (dcc->type == TYPE_RECV)
		fe_dcc_update_recv (dcc);
	else
		fe_dcc_update_chat_win ();
}

static gboolean
dcc_send_data (GIOChannel *source, GIOCondition condition, struct DCC *dcc)
{
	char *buf;
	int len, sent, sok = dcc->sok;

	if (prefs.dcc_blocksize < 1) /* this is too little! */
		prefs.dcc_blocksize = 1024;

	if (prefs.dcc_blocksize > 102400)	/* this is too much! */
		prefs.dcc_blocksize = 102400;

	buf = malloc (prefs.dcc_blocksize);
	if (!buf)
		return TRUE;

	lseek (dcc->fp, dcc->pos, SEEK_SET);
	len = read (dcc->fp, buf, prefs.dcc_blocksize);
	if (len < 1)
		goto abortit;
	sent = send (sok, buf, len, 0);

	if (send < 0 && !(would_block ()))
	{
abortit:
		EMIT_SIGNAL (XP_TE_DCCSENDFAIL, dcc->serv->front_session,
						 file_part (dcc->file), dcc->nick, NULL, NULL, 0);
		dcc_close (dcc, STAT_FAILED, FALSE);
		free (buf);
		return TRUE;
	}
	if (sent > 0)
	{
		dcc->pos += sent;
		dcc->lasttime = time (0);
		dcc_calc_cps (dcc);
	}

	/* have we sent it all yet? */
	if (dcc->pos >= dcc->size)
	{
		/* it's all sent now, so remove the WRITE/SEND handler */
		if (dcc->wiotag)
		{
			fe_input_remove (dcc->wiotag);
			dcc->wiotag = 0;
		}
	}

	free (buf);

	return TRUE;
}

static gboolean
dcc_read_ack (GIOChannel *source, GIOCondition condition, struct DCC *dcc)
{
	int len;
	guint32 ack;
	char buf[16];
	int sok = dcc->sok;

	len = recv (sok, (char *) &ack, 4, MSG_PEEK);
	if (len < 1)
	{
		if (len < 0)
		{
			if (would_block_again ())
				return TRUE;
		}
		EMIT_SIGNAL (XP_TE_DCCSENDFAIL, dcc->serv->front_session,
						 file_part (dcc->file), dcc->nick, NULL, NULL, 0);
		dcc_close (dcc, STAT_FAILED, FALSE);
		return TRUE;
	}
	if (len < 4)
		return TRUE;
	recv (sok, (char *) &ack, 4, 0);
	dcc->ack = ntohl (ack);

	/* fix for BitchX */
	if (dcc->ack < dcc->resumable)
		dcc->ackoffset = TRUE;
	if (dcc->ackoffset)
		dcc->ack += dcc->resumable;

	if (!dcc->fastsend)
	{
		if (dcc->ack < dcc->pos)
			return TRUE;
		dcc_send_data (NULL, 0, (gpointer)dcc);
	}

	if (dcc->pos >= dcc->size && dcc->ack >= dcc->size)
	{
		dcc_close (dcc, STAT_DONE, FALSE);
		sprintf (buf, "%d", dcc->cps);
		EMIT_SIGNAL (XP_TE_DCCSENDCOMP, dcc->serv->front_session,
						 file_part (dcc->file), dcc->nick, buf, NULL, 0);
	}

	return TRUE;
}

static gboolean
dcc_accept (GIOChannel *source, GIOCondition condition, struct DCC *dcc)
{
	char host[128];
	struct sockaddr_in CAddr;
	int sok;
	size_t len;

	len = sizeof (CAddr);
	sok = accept (dcc->sok, (struct sockaddr *) &CAddr, &len);
	fe_input_remove (dcc->iotag);
	dcc->iotag = 0;
	closesocket (dcc->sok);
	if (sok < 0)
	{
		dcc->sok = -1;
		dcc_close (dcc, STAT_FAILED, FALSE);
		return TRUE;
	}
	set_nonblocking (sok);
	dcc->sok = sok;
	dcc->addr = ntohl (CAddr.sin_addr.s_addr);
	dcc->dccstat = STAT_ACTIVE;
	dcc->lasttime = dcc->starttime = time (0);
	dcc->fastsend = prefs.fastdccsend;
	switch (dcc->type)
	{
	case TYPE_SEND:
		if (dcc->fastsend)
			dcc->wiotag = fe_input_add (sok, 0, 1, 0, dcc_send_data, dcc);
		dcc->iotag = fe_input_add (sok, 1, 0, 1, dcc_read_ack, dcc);
		dcc_send_data (NULL, 0, (gpointer)dcc);
		break;

	case TYPE_CHATSEND:
		dcc->iotag = fe_input_add (dcc->sok, 1, 0, 1, dcc_read_chat, dcc);
		dcc->dccchat = malloc (sizeof (struct dcc_chat));
		dcc->dccchat->pos = 0;
		break;
	}

	update_dcc_window (dcc->type);

	snprintf (host, sizeof host, "%s:%d", net_ip (dcc->addr), dcc->port);
	EMIT_SIGNAL (XP_TE_DCCCON, dcc->serv->front_session,
					 dcctypes[(int) dcc->type], dcc->nick, host, "from", 0);

	return TRUE;
}

static int
dcc_listen_init (struct DCC *dcc, session *sess)
{
	size_t len;
	unsigned long my_addr;
	struct sockaddr_in SAddr;
	int i, bindretval = -1;

	dcc->sok = socket (AF_INET, SOCK_STREAM, 0);
	if (dcc->sok == -1)
		return FALSE;

	memset (&SAddr, 0, sizeof (struct sockaddr_in));

	len = sizeof (SAddr);
	getsockname (dcc->serv->sok, (struct sockaddr *) &SAddr, &len);

	SAddr.sin_family = AF_INET;

	/*if local_ip is specified use that*/
	if (prefs.local_ip != 0)
	{
		my_addr = prefs.local_ip;
		SAddr.sin_addr.s_addr = prefs.local_ip;
	}
	/*otherwise use the default*/
	else
		my_addr = SAddr.sin_addr.s_addr;

	/*if we have a valid portrange try to use that*/
	if (prefs.first_dcc_send_port > 0)
	{
		SAddr.sin_port = 0;
		i = 0;
		while ((prefs.last_dcc_send_port > ntohs(SAddr.sin_port)) &&
				(bindretval == -1))
		{
			SAddr.sin_port = htons (prefs.first_dcc_send_port + i);
			i++;
			/*printf("Trying to bind against port: %d\n",ntohs(SAddr.sin_port));*/
			bindretval = bind (dcc->sok, (struct sockaddr *) &SAddr, sizeof (SAddr));
		}
	}

	/*if we didnt bind yet, try a random port*/
	if (bindretval == -1)
	{
		SAddr.sin_port = 0;
		bindretval = bind (dcc->sok, (struct sockaddr *) &SAddr, sizeof (SAddr));
	}

	if (bindretval == -1)
	{
		/* failed to bind */
		PrintText (sess, "Failed to bind to any address or port.\n");
		return FALSE;
	}

	len = sizeof (SAddr);
	getsockname (dcc->sok, (struct sockaddr *) &SAddr, &len);

	dcc->port = ntohs (SAddr.sin_port);

	/*if we have a dcc_ip, we use that, so the remote client can connect*/
	/*we would tell the client to connect to our LAN ip otherwise*/
	if (prefs.ip_from_server != 0 && prefs.dcc_ip != 0)
		dcc->addr = prefs.dcc_ip;
	else if (prefs.dcc_ip_str[0])
		dcc->addr = inet_addr ((const char *) prefs.dcc_ip_str);
	else
	/*else we use the one we bound to*/
		dcc->addr = my_addr;

	dcc->addr = ntohl (dcc->addr);

	set_nonblocking (dcc->sok);
	listen (dcc->sok, 1);
	set_blocking (dcc->sok);

	dcc->iotag = fe_input_add (dcc->sok, 1, 0, 1, dcc_accept, dcc);

	return TRUE;
}

static struct session *dccsess;
static char *dccto;				  /* lame!! */
static char *dcctbuf;

static void
dcc_send_wild (char *file)
{
	dcc_send (dccsess, dcctbuf, dccto, file);
}

/* tbuf is at least 400 bytes */

void
dcc_send (struct session *sess, char *tbuf, char *to, char *file)
{
	struct stat st;
	struct DCC *dcc = new_dcc ();
	if (!dcc)
		return;

	dcc->file = expand_homedir (file);

	if (strchr (dcc->file, '*') || strchr (dcc->file, '?'))
	{
		char path[256];
		char wild[256];

		strcpy (wild, file_part (dcc->file));
		path_part (dcc->file, path);
		path[strlen (path) - 1] = 0;	/* remove trailing slash */
		dccsess = sess;
		dccto = to;
		dcctbuf = tbuf;
		for_files (path, wild, dcc_send_wild);
		dcc_close (dcc, 0, TRUE);
		return;
	}
	if (stat (dcc->file, &st) != -1)
	{
		if (*file_part (dcc->file) && !S_ISDIR (st.st_mode))
		{
			if (st.st_size > 0)
			{
				dcc->offertime = time (0);
				dcc->serv = sess->server;
				dcc->dccstat = STAT_QUEUED;
				dcc->size = st.st_size;
				dcc->type = TYPE_SEND;
				dcc->fp = open (dcc->file, OFLAGS | O_RDONLY);
				if (dcc->fp != -1)
				{
					if (dcc_listen_init (dcc, sess))
					{
						char havespaces = 0;
						file = dcc->file;
						while (*file)
						{
							if (*file == ' ')
							{
								if (prefs.dcc_send_fillspaces)
						    		*file = '_';
							  	else
							   	havespaces = 1;
							}
							file++;
						}
						dcc->nick = strdup (to);
						if (prefs.autoopendccsendwindow)
							fe_dcc_open_send_win (TRUE);
						else
							fe_dcc_update_send_win ();
						
						if (havespaces)
						{
							snprintf (tbuf, 400,
									 "PRIVMSG %s :\001DCC SEND \"%s\" %lu %d %d\001\r\n",
									 to, file_part (dcc->file), dcc->addr, dcc->port,
									 dcc->size);
						}
						else
						{
							snprintf (tbuf, 400,
									 "PRIVMSG %s :\001DCC SEND %s %lu %d %d\001\r\n",
									 to, file_part (dcc->file), dcc->addr, dcc->port,
									 dcc->size);
						}
						tcp_send (sess->server, tbuf);
						EMIT_SIGNAL (XP_TE_DCCOFFER, sess, file_part (dcc->file),
										 to, NULL, NULL, 0);
					} else
					{
						dcc_close (dcc, 0, TRUE);
					}
					return;
				}
			}
		}
	}
	snprintf (tbuf, 400, _("Cannot access %s\n"), dcc->file);
	PrintText (sess, tbuf);
	dcc_close (dcc, 0, TRUE);
}

static struct DCC *
find_dcc_from_port (int port, int type)
{
	struct DCC *dcc;
	GSList *list = dcc_list;
	while (list)
	{
		dcc = (struct DCC *) list->data;
		if (dcc->port == port &&
			 dcc->dccstat == STAT_QUEUED && dcc->type == type)
			return dcc;
		list = list->next;
	}
	return 0;
}

struct DCC *
find_dcc (char *nick, char *file, int type)
{
	GSList *list = dcc_list;
	struct DCC *dcc;
	while (list)
	{
		dcc = (struct DCC *) list->data;
		if (nick == NULL || !strcasecmp (nick, dcc->nick))
		{
			if (type == -1 || dcc->type == type)
			{
				if (!file[0])
					return dcc;
				if (!strcasecmp (file, file_part (dcc->file)))
					return dcc;
				if (!strcasecmp (file, dcc->file))
					return dcc;
			}
		}
		list = list->next;
	}
	return 0;
}

/* called when we receive a NICK change from server */

void
dcc_change_nick (struct server *serv, char *oldnick, char *newnick)
{
	struct DCC *dcc;
	GSList *list = dcc_list;

	while (list)
	{
		dcc = (struct DCC *) list->data;
		if (dcc->serv == serv)
		{
			if (!strcasecmp (dcc->nick, oldnick))
			{
				if (dcc->nick)
					free (dcc->nick);
				dcc->nick = strdup (newnick);
			}
		}
		list = list->next;
	}
}

void
dcc_get (struct DCC *dcc)
{
	switch (dcc->dccstat)
	{
	case STAT_QUEUED:
		if (dcc->type != TYPE_CHATSEND)
		{
			dcc->resumable = 0;
			dcc->pos = 0;
			dcc_connect (dcc);
		}
		break;
	case STAT_DONE:
	case STAT_FAILED:
	case STAT_ABORTED:
		dcc_close (dcc, 0, TRUE);
		break;
	}
}

void
dcc_get_nick (struct session *sess, char *nick)
{
	struct DCC *dcc;
	GSList *list = dcc_list;
	while (list)
	{
		dcc = (struct DCC *) list->data;
		if (!strcasecmp (nick, dcc->nick))
		{
			if (dcc->dccstat == STAT_QUEUED && dcc->type == TYPE_RECV)
			{
				dcc->resumable = 0;
				dcc->pos = 0;
				dcc->ack = 0;
				dcc_connect (dcc);
				return;
			}
		}
		list = list->next;
	}
	if (sess)
		EMIT_SIGNAL (XP_TE_DCCIVAL, sess, NULL, NULL, NULL, NULL, 0);
}

static struct DCC *
new_dcc (void)
{
	struct DCC *dcc = malloc (sizeof (struct DCC));
	if (!dcc)
		return 0;
	memset (dcc, 0, sizeof (struct DCC));
	dcc->sok = -1;
	dcc->fp = -1;
	dcc_list = g_slist_prepend (dcc_list, dcc);
	return (dcc);
}

void
dcc_chat (struct session *sess, char *nick)
{
	char outbuf[400];
	struct DCC *dcc;

	dcc = find_dcc (nick, "", TYPE_CHATSEND);
	if (dcc)
	{
		switch (dcc->dccstat)
		{
		case STAT_ACTIVE:
		case STAT_QUEUED:
		case STAT_CONNECTING:
			EMIT_SIGNAL (XP_TE_DCCCHATREOFFER, sess, nick, NULL, NULL, NULL, 0);
			return;
		case STAT_ABORTED:
		case STAT_FAILED:
			dcc_close (dcc, 0, TRUE);
		}
	}
	dcc = find_dcc (nick, "", TYPE_CHATRECV);
	if (dcc)
	{
		switch (dcc->dccstat)
		{
		case STAT_QUEUED:
			dcc_connect (dcc);
			break;
		case STAT_FAILED:
		case STAT_ABORTED:
			dcc_close (dcc, 0, TRUE);
		}
		return;
	}
	/* offer DCC CHAT */

	dcc = new_dcc ();
	if (!dcc)
		return;
	dcc->starttime = dcc->offertime = time (0);
	dcc->serv = sess->server;
	dcc->dccstat = STAT_QUEUED;
	dcc->type = TYPE_CHATSEND;
	dcc->nick = strdup (nick);
	if (dcc_listen_init (dcc, sess))
	{
		if (prefs.autoopendccchatwindow)
			fe_dcc_open_chat_win (TRUE);
		else
			fe_dcc_update_chat_win ();
		snprintf (outbuf, sizeof (outbuf),
					 "PRIVMSG %s :\001DCC CHAT chat %lu %d\001\r\n", nick,
					 dcc->addr, dcc->port);
		tcp_send (dcc->serv, outbuf);
		EMIT_SIGNAL (XP_TE_DCCCHATOFFERING, sess, nick, NULL, NULL, NULL, 0);
	} else
	{
		dcc_close (dcc, 0, TRUE);
	}
}

static void
dcc_malformed (struct session *sess, char *nick, char *data)
{
	EMIT_SIGNAL (XP_TE_MALFORMED_FROM, sess, nick, NULL, NULL, NULL, 0);
	EMIT_SIGNAL (XP_TE_MALFORMED_PACKET, sess, data, NULL, NULL, NULL, 0);
}

void
dcc_resume (struct DCC *dcc)
{
	char tbuf[400];

	if (dcc->dccstat == STAT_QUEUED && dcc->resumable)
	{
		if (strchr (dcc->file, ' ') != NULL)
		{	/*filename contains spaces*/
			snprintf (tbuf, sizeof (tbuf),
						 "PRIVMSG %s :\001DCC RESUME \"%s\" %d %d\001\r\n", dcc->nick,
						 dcc->file, dcc->port, dcc->resumable);
		}
		else
		{
			snprintf (tbuf, sizeof (tbuf),
						 "PRIVMSG %s :\001DCC RESUME %s %d %d\001\r\n", dcc->nick,
						 dcc->file, dcc->port, dcc->resumable);
		}
		tcp_send (dcc->serv, tbuf);
	}
}

void
handle_dcc (struct session *sess, char *outbuf, char *nick, char *word[],
				char *word_eol[])
{
	struct DCC *dcc;
	char *type = word[5];
	int port, size;
	unsigned long addr;

	if (!strcasecmp (type, "CHAT"))
	{
		port = atoi (word[8]);
		sscanf (word[7], "%lu", &addr);

		if (!addr || port < 1024)
		{
			dcc_malformed (sess, nick, word_eol[4] + 2);
			return;
		}
		dcc = find_dcc (nick, "", TYPE_CHATSEND);
		if (dcc)
			dcc_close (dcc, 0, TRUE);

		dcc = find_dcc (nick, "", TYPE_CHATRECV);
		if (dcc)
			dcc_close (dcc, 0, TRUE);

		dcc = new_dcc ();
		if (dcc)
		{
			dcc->serv = sess->server;
			dcc->type = TYPE_CHATRECV;
			dcc->dccstat = STAT_QUEUED;
			dcc->addr = addr;
			dcc->port = port;
			dcc->nick = strdup (nick);
			dcc->starttime = time (0);

			EMIT_SIGNAL (XP_TE_DCCCHATOFFER, sess->server->front_session, nick,
							 NULL, NULL, NULL, 0);
			if (prefs.autoopendccchatwindow)
				fe_dcc_open_chat_win (TRUE);
			else
				fe_dcc_update_chat_win ();
			if (prefs.autodccchat)
				dcc_connect (dcc);
		}
		return;
	}
	if (!strcasecmp (type, "RESUME"))
	{
		port = atoi (word[7]);
		dcc = find_dcc_from_port (port, TYPE_SEND);
		if (!dcc)
			dcc = find_dcc (nick, word[6], TYPE_SEND);
		if (dcc)
		{
			dcc->resumable = atoi (word[8]);
			if (dcc->resumable < dcc->size)
			{
				dcc->pos = dcc->resumable;
				dcc->ack = dcc->resumable;
				lseek (dcc->fp, dcc->pos, SEEK_SET);
				if (strchr (file_part (dcc->file), ' ') != NULL)
				{	/*filename contains spaces*/
					snprintf (outbuf, 400,
								 "PRIVMSG %s :\001DCC ACCEPT \"%s\" %d %d\001\r\n", dcc->nick,
								 file_part (dcc->file), port, dcc->resumable);
				}
				else
				{
					snprintf (outbuf, 400,
								 "PRIVMSG %s :\001DCC ACCEPT %s %d %d\001\r\n", dcc->nick,
								 file_part (dcc->file), port, dcc->resumable);
				}
				tcp_send (dcc->serv, outbuf);
			}
			sprintf (outbuf, "%d", dcc->pos);
			EMIT_SIGNAL (XP_TE_DCCRESUMEREQUEST, sess, nick,
							 file_part (dcc->file), outbuf, NULL, 0);
		}
		return;
	}
	if (!strcasecmp (type, "ACCEPT"))
	{
		port = atoi (word[7]);
		dcc = find_dcc_from_port (port, TYPE_RECV);
		if (dcc && dcc->dccstat == STAT_QUEUED)
		{
			dcc_connect (dcc);
			return;
		}
	}
	if (!strcasecmp (type, "SEND"))
	{
		char *file = file_part (word[6]);
		port = atoi (word[8]);
		size = atoi (word[9]);

		sscanf (word[7], "%lu", &addr);

		if (!addr || !size || port < 1024)
		{
			dcc_malformed (sess, nick, word_eol[4] + 2);
			return;
		}
		dcc = new_dcc ();
		if (dcc)
		{
			struct stat st;

			dcc->file = strdup (file);

			dcc->destfile =
				malloc (strlen (prefs.dccdir) + strlen (nick) + strlen (file) +
						  4);

			strcpy (dcc->destfile, prefs.dccdir);
			if (prefs.dccdir[strlen (prefs.dccdir) - 1] != '/')
				strcat (dcc->destfile, "/");
			if (prefs.dccwithnick)
			{
				strcat (dcc->destfile, nick);
				strcat (dcc->destfile, ".");
			}
			strcat (dcc->destfile, file);

			dcc->resumable = 0;
			if (stat (dcc->destfile, &st) != -1)
			{
				if (st.st_size < size)
					dcc->resumable = st.st_size;
			}

			dcc->pos = dcc->resumable;
			dcc->serv = sess->server;
			dcc->type = TYPE_RECV;
			dcc->dccstat = STAT_QUEUED;
			dcc->addr = addr;
			dcc->port = port;
			dcc->size = size;
			dcc->nick = strdup (nick);
			if (prefs.autodccsend)
			{
				if (prefs.autoresume && dcc->resumable)
				{
					/* don't resume the same file from two people! */
					GSList *list = dcc_list;
					struct DCC *d;
					while (list)
					{
						d = list->data;
						if (d->type == TYPE_RECV && d->dccstat != STAT_ABORTED &&
							 d->dccstat != STAT_DONE && d->dccstat != STAT_FAILED)
						{
							if (d != dcc && strcmp (d->destfile, dcc->destfile) == 0)
								goto dontresume;
						}
						list = list->next;
					}
					dcc_resume (dcc);
				} else
				{
dontresume:
					dcc->resumable = 0;
					dcc->pos = 0;
					dcc_connect (dcc);
				}
			}
			if (prefs.autoopendccrecvwindow)
				fe_dcc_open_recv_win (TRUE);
			else
				fe_dcc_update_recv_win ();
		}
		sprintf (outbuf, "%d", size);
		EMIT_SIGNAL (XP_TE_DCCSENDOFFER, sess->server->front_session, nick,
						 file, outbuf, NULL, 0);
	} else
		EMIT_SIGNAL (XP_TE_DCCGENERICOFFER, sess->server->front_session,
						 word_eol[4] + 2, nick, NULL, NULL, 0);
}

void
dcc_show_list (struct session *sess, char *outbuf)
{
	int i = 0;
	struct DCC *dcc;
	GSList *list = dcc_list;

	EMIT_SIGNAL (XP_TE_DCCHEAD, sess, NULL, NULL, NULL, NULL, 0);
	while (list)
	{
		dcc = (struct DCC *) list->data;
		i++;
		snprintf (outbuf, 255, " %-5.5s %-10.10s %-7.7s %-7d %-7d %s\n",
					 dcctypes[(int) dcc->type], dcc->nick,
					 _(dccstat[(int) dcc->dccstat].name), dcc->size, dcc->pos,
					 file_part (dcc->file));
		PrintText (sess, outbuf);
		list = list->next;
	}
	if (!i)
		PrintText (sess, _("No active DCCs\n"));
}
