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
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define WANTSOCKET
#include "inet.h"

#ifndef WIN32
#include <sys/wait.h>
#include <signal.h>
#endif

#include "xchat.h"
#include "fe.h"
#include "util.h"
#include "cfgfiles.h"
#include "ignore.h"
#include "plugin.h"
#include "notify.h"
#include "server.h"
#include "pythonc.h"
#include "outbound.h"
#include "text.h"
#include "perlc.h"
#include "xchatc.h"

#ifdef USE_OPENSSL
#include <openssl/ssl.h>		  /* SSL_() */
#include "ssl.h"
#endif

#ifdef USE_JCODE
#include "jcode.h"
#endif

GSList *popup_list = 0;
GSList *button_list = 0;
GSList *dlgbutton_list = 0;
GSList *command_list = 0;
GSList *ctcp_list = 0;
GSList *replace_list = 0;
GSList *sess_list = 0;
GSList *serv_list = 0;
GSList *dcc_list = 0;
GSList *ignore_list = 0;
GSList *usermenu_list = 0;
GSList *urlhandler_list = 0;
static GSList *away_list = 0;

static int in_xchat_exit = FALSE;
int xchat_is_quitting = FALSE;
int auto_connect = TRUE;

struct session *current_tab;
struct session *menu_sess = 0;
struct xchatprefs prefs;

#ifdef USE_OPENSSL
SSL_CTX *ctx = NULL;
void ssl_cb_info (SSL * s, int where, int ret);
#endif

static void free_away_messages (server *serv);


/* actually send to the socket. This might do a character translation or
   send via SSL */

static int
tcp_send_real (struct server *serv, char *buf, int len)
{
	int ret = 1;
	unsigned char *tbuf = buf;
#ifdef USE_TRANS
#define TRANS_STAT_BUF_LEN 1024
	static unsigned char sbuf[TRANS_STAT_BUF_LEN];

	if (prefs.use_trans)
	{
		if (len >= TRANS_STAT_BUF_LEN)
			tbuf = malloc (len + 1);
		else
			tbuf = sbuf;
		if (!tbuf)
			return -1;
		strcpy (tbuf, buf);
		user2serv (tbuf);
	}
#endif

	fe_add_rawlog (serv, tbuf, TRUE);

	if (!EMIT_SIGNAL (XP_IF_SEND, (void *) serv->sok, tbuf, (void *)len,
							NULL, NULL, 0))
	{
#ifdef USE_OPENSSL
		if (!serv->ssl)
			ret = send (serv->sok, tbuf, len, 0);
		else
			ret = _SSL_send (serv->ssl, tbuf, len);
#else
		ret = send (serv->sok, tbuf, len, 0);
#endif
	}

#ifdef USE_TRANS
	if (prefs.use_trans)
	{
		if (tbuf != sbuf)
			free (tbuf);
	}
#endif

	return ret;
}

/* new throttling system, uses the same method as the Undernet
   ircu2.10 server; under test, a 200-line paste didn't flood
   off the client */

static int
tcp_send_queue (struct server *serv)
{
	char *buf, *p;
	int len, i;
	GSList *list;
	time_t now = time (0);

	/* did the server close since the timeout was added? */
	if (!is_server (serv))
		return 0;

	list = serv->outbound_queue;

	while (list)
	{
		buf = (char *) list->data;
		len = strlen (buf);

		if (serv->next_send < now)
			serv->next_send = now;
		if (serv->next_send - now >= 10)
			return 1;				  /* don't remove the timeout handler */

		for (p = buf, i = len; i && *p != ' '; p++, i--);
			serv->next_send += (2 + i / 120);
		serv->sendq_len -= len;
		fe_set_throttle (serv);

		tcp_send_real (serv, buf, len);

		serv->outbound_queue = g_slist_remove (serv->outbound_queue, buf);
		free (buf);

		list = serv->outbound_queue;
	}
	return 0;						  /* remove the timeout handler */
}

#ifdef USE_JCODE
static void
fe_add_ja_rawlog (struct server *serv, char *buf,  int outbound)
{
	char *jbuf =  kanji_conv_to_locale(buf);
	if (jbuf)
	{
		fe_add_rawlog (serv, jbuf, TRUE);
		free(jbuf);
	} else
			fe_add_rawlog (serv, buf, TRUE);
}
#endif

int
tcp_send_len (struct server *serv, char *buf, int len)
{
    char *dbuf;
    int noqueue = !serv->outbound_queue;
#ifdef USE_JCODE
    unsigned char *jbuf = NULL;

    if ( prefs.kanji_conv ) {
        jbuf = kanji_conv_auto(buf, "ISO-2022-JP");
        if (!jbuf)
            return 0;
        else {
            buf = strdup(jbuf); free(jbuf);
            len = strlen(buf);
        }
    } else
        buf = strdup(buf); /* freed after all */
#endif    

	if (!prefs.throttle)
		return tcp_send_real (serv, buf, len);

	dbuf = malloc (len + 1);
	memcpy (dbuf, buf, len);
	dbuf[len] = 0;

	/* only privmsg and notice go to the back of the queue */
	if (strncasecmp (dbuf, "PRIVMSG", 7) == 0 ||
		 strncasecmp (dbuf, "NOTICE", 6) == 0)
		serv->outbound_queue = g_slist_append (serv->outbound_queue, dbuf);
	else
		serv->outbound_queue = g_slist_prepend (serv->outbound_queue, dbuf);
	serv->sendq_len += len; /* tcp_send_queue uses strlen */

	if (tcp_send_queue (serv) && noqueue)
		fe_timeout_add (500, tcp_send_queue, serv);
    
	return 1;
}

int
tcp_send (struct server *serv, char *buf)
{
	return tcp_send_len (serv, buf, strlen (buf));
}

static int
is_in_list (GSList *list, void *data)
{
	while (list)
	{
		if (list->data == data)
			return TRUE;
		list = list->next;
	}
	return FALSE;
}

int
is_server (server * serv)
{
	return is_in_list (serv_list, serv);
}

int
is_session (session * sess)
{
	return is_in_list (sess_list, sess);
}

struct session *
find_dialog (struct server *serv, char *nick)
{
	GSList *list = sess_list;
	struct session *sess;

	while (list)
	{
		sess = (struct session *) list->data;
		if (sess->server == serv && sess->type == SESS_DIALOG)
		{
			if (!strcasecmp (nick, sess->channel))
				return (sess);
		}
		list = list->next;
	}
	return 0;
}

session *
find_session_from_channel (char *chan, server *serv)
{
	session *sess;
	GSList *list = sess_list;
	while (list)
	{
		sess = list->data;
		if (sess->type != SESS_SHELL && !strcasecmp (chan, sess->channel))
		{
			if (!serv || serv == sess->server)
				return sess;
		}
		list = list->next;
	}
	return 0;
}

#ifndef WIN32

static int
mail_items (char *file)
{
	FILE *fp;
	int items;
	char buf[512];

	fp = fopen (file, "r");
	if (!fp)
		return 1;

	items = 0;
	while (fgets (buf, sizeof buf, fp))
	{
		if (!strncmp (buf, "From ", 5))
			items++;
	}
	fclose (fp);

	return items;
}

static void
xchat_mail_check (void)
{
	static int last_size = -1;
	int size;
	struct stat st;
	char buf[512];
	char *maildir;

	maildir = getenv ("MAIL");
	if (!maildir)
	{
		snprintf (buf, sizeof (buf), "/var/spool/mail/%s", g_get_user_name ());
		maildir = buf;
	}

	if (stat (maildir, &st) < 0)
		return;

	size = st.st_size;

	if (last_size == -1)
	{
		last_size = size;
		return;
	}

	if (size > last_size)
	{
		sprintf (buf, "%d", mail_items (maildir));
		sprintf (buf + 16, "%d", size);
		if (menu_sess && menu_sess->type != SESS_SHELL)
			EMIT_SIGNAL (XP_TE_NEWMAIL, menu_sess, buf, buf + 16, NULL, NULL, 0);
	}

	last_size = size;
}

#endif

static int
lagcheck_update (void)
{
	server *serv;
	GSList *list = serv_list;
	
	if (!prefs.lagometer)
		return 1;

	while (list)
	{
		serv = list->data;
		if (serv->lag_sent)
			fe_set_lag (serv, -1);

		list = list->next;
	}
	return 1;
}

void
lag_check (void)
{
	server *serv;
	GSList *list = serv_list;
	unsigned long tim;
	char tbuf[256];
	time_t now = time (0);
	int lag;

	tim = make_ping_time ();

	while (list)
	{
		serv = list->data;
		if (serv->connected && serv->end_of_motd)
		{
			lag = now - serv->ping_recv;
			if (prefs.pingtimeout && lag > prefs.pingtimeout && lag > 0)
			{
				sprintf (tbuf, "%d", lag);
				EMIT_SIGNAL (XP_TE_PINGTIMEOUT, serv->front_session, tbuf, NULL,
								 NULL, NULL, 0);
				auto_reconnect (serv, FALSE, -1);
			} else
			{
				/*sprintf (tbuf, "PING LAG%lu :%s\r\n", tim, serv->servername);*/
				sprintf (tbuf, "PING LAG%lu\r\n", tim);
				tcp_send (serv, tbuf);
				serv->lag_sent = tim;
				fe_set_lag (serv, -1);
			}
		}
		list = list->next;
	}
}

static int
xchat_misc_checks (void)		  /* this gets called every 2 seconds */
{
	static int count = 0;

	dcc_check_timeouts ();

	count++;

	if (count == 7 && prefs.lagometer)	/* every 30 seconds */
		lag_check ();

	if (count == 15)							/* every 30 seconds */
	{
		count = 0;
#ifndef WIN32
		if (prefs.mail_check)
			xchat_mail_check ();
#endif
	}

	return 1;
}

/* executed when the first irc window opens */

static void
irc_init (session *sess)
{
	static int done_init = FALSE;

	if (done_init)
		return;

	done_init = TRUE;

#ifdef USE_PYTHON
	pys_init ();
#endif
#ifdef USE_PLUGIN
	module_setup ();
#endif
#ifdef USE_PERL
	perl_auto_load (sess);
#endif

	if (prefs.notify_timeout)
		notify_tag = fe_timeout_add (prefs.notify_timeout * 1000,
											  notify_checklist, 0);

	fe_timeout_add (2000, xchat_misc_checks, 0);
	fe_timeout_add (500, lagcheck_update, 0);
}

static session *
new_session (server *serv, char *from, int type)
{
	session *sess;

	sess = malloc (sizeof (struct session));
	memset (sess, 0, sizeof (struct session));

	sess->server = serv;
	sess->logfd = -1;
	sess->type = type;
	sess->current_modes = strdup ("");

	if (from != NULL)
		safe_strcpy (sess->channel, from, CHANLEN);

	sess_list = g_slist_prepend (sess_list, sess);

	fe_new_window (sess);

	return sess;
}

void
set_server_defaults (server *serv)
{
	if (serv->chantypes)
		free (serv->chantypes);
	if (serv->chanmodes)
		free (serv->chanmodes);
	if (serv->nick_prefixes)
		free (serv->nick_prefixes);
	if (serv->nick_modes)
		free (serv->nick_modes);

	serv->chantypes = strdup ("#&!+");
	serv->chanmodes = strdup ("beI,k,l");
	serv->nick_prefixes = strdup ("@%+");
	serv->nick_modes = strdup ("ohv");

	serv->nickcount = 1;
	serv->end_of_motd = FALSE;
	serv->is_away = FALSE;
	serv->supports_watch = FALSE;
	serv->bad_prefix = FALSE;
}

static server *
new_server (void)
{
	server *serv;

	serv = malloc (sizeof (struct server));
	memset (serv, 0, sizeof (struct server));

	serv->sok = -1;
	strcpy (serv->nick, prefs.nick1);
	set_server_defaults (serv);

	serv_list = g_slist_prepend (serv_list, serv);

	fe_new_server (serv);

	return serv;
}

session *
new_ircwindow (server *serv, char *name, int type)
{
	session *sess;

	switch (type)
	{
	case SESS_SHELL:
		sess = new_session (new_server (), name, SESS_SHELL);
		break;
	case SESS_SERVER:
		serv = new_server ();
		if (prefs.use_server_tab)
		{
			register unsigned int oldh = prefs.hideuserlist;
			prefs.hideuserlist = 1;
			sess = new_session (serv, name, SESS_SERVER);
			prefs.hideuserlist = oldh;
			serv->front_session = sess;
		} else
		{
			sess = new_session (serv, name, SESS_CHANNEL);
		}
		break;
	default:
/*	case SESS_CHANNEL:
	case SESS_DIALOG:
	case SESS_NOTICES:
	case SESS_SNOTICES:*/
		sess = new_session (serv, name, type);
		break;
	}

	irc_init (sess);

	return sess;
}

static void
kill_server_callback (server * serv)
{
	server_cleanup (serv);

/*	fe_server_callback (serv);*/

	serv_list = g_slist_remove (serv_list, serv);

	dcc_notify_kill (serv);
	flush_server_queue (serv);
	free_away_messages (serv);

	free (serv->nick_modes);
	free (serv->nick_prefixes);
	free (serv->chanmodes);
	free (serv->chantypes);
	if (serv->bad_nick_prefixes)
		free (serv->bad_nick_prefixes);
	if (serv->last_away_reason)
		free (serv->last_away_reason);
	if (serv->eom_cmd)
		free (serv->eom_cmd);
	free (serv->gui);
	free (serv);

	notify_cleanup ();
}

static void
log_notify_kill (session * sess)
{
	if (sess->logfd != -1)
		end_logging (sess->logfd);
}

static void
exec_notify_kill (session * sess)
{
#ifndef WIN32
	struct nbexec *re;
	if (sess->running_exec != NULL)
	{
		re = sess->running_exec;
		sess->running_exec = NULL;
		kill (re->childpid, SIGKILL);
		waitpid (re->childpid, NULL, WNOHANG);
		fe_input_remove (re->iotag);
		close (re->myfd);
		if (re->linebuf)
			free(re->linebuf);
		free (re);
	}
#endif
}

static void
send_quit_or_part (session * killsess)
{
	int willquit = TRUE;
	GSList *list;
	session *sess;
	server *killserv = killsess->server;

	/* check if this is the last session using this server */
	list = sess_list;
	while (list)
	{
		sess = (session *) list->data;
		if (sess->server == killserv && sess != killsess)
		{
			willquit = FALSE;
			list = 0;
		} else
			list = list->next;
	}

	if (xchat_is_quitting)
		willquit = TRUE;

	if (killserv->connected)
	{
		if (willquit)
		{
			if (!killserv->sent_quit)
			{
				flush_server_queue (killserv);
				server_sendquit (killsess);
				killserv->sent_quit = TRUE;
			}
		} else
		{
			if (killsess->type == SESS_CHANNEL && killsess->channel[0])
			{
				server_sendpart (killserv, killsess->channel, 0);
			}
		}
	}
}

void
kill_session_callback (session * killsess)
{
	server *killserv = killsess->server;
	session *sess;
	GSList *list;

	if (current_tab == killsess)
		current_tab = NULL;

	if (killserv->front_session == killsess)
	{
		/* front_session is closed, find a valid replacement */
		killserv->front_session = NULL;
		list = sess_list;
		while (list)
		{
			sess = (session *) list->data;
			if (sess != killsess && sess->server == killserv)
			{
				killserv->front_session = sess;
				break;
			}
			list = list->next;
		}
	}

	sess_list = g_slist_remove (sess_list, killsess);

	if (killsess->type == SESS_CHANNEL)
		free_userlist (killsess);

	fe_session_callback (killsess);

	exec_notify_kill (killsess);

#ifdef USE_PERL
	perl_notify_kill (killsess);
#endif

	log_notify_kill (killsess);

	send_quit_or_part (killsess);

	history_free (&killsess->history);
	free (killsess->gui);
	if (killsess->topic)
		free (killsess->topic);
	free (killsess->current_modes);
	free (killsess);

	if (!sess_list && !in_xchat_exit)
		xchat_exit ();						/* sess_list is empty, quit! */

	list = sess_list;
	while (list)
	{
		sess = (session *) list->data;
		if (sess->server == killserv)
			return;					  /* this server is still being used! */
		list = list->next;
	}

	kill_server_callback (killserv);
}

static void
free_sessions (void)
{
	GSList *list = sess_list;

	while (list)
	{
		fe_close_window (list->data);
		list = sess_list;
	}
}

struct away_msg *
find_away_message (struct server *serv, char *nick)
{
	struct away_msg *away;
	GSList *list = away_list;
	while (list)
	{
		away = (struct away_msg *) list->data;
		if (away->server == serv && !strcasecmp (nick, away->nick))
			return away;
		list = list->next;
	}
	return 0;
}

static void
free_away_messages (server *serv)
{
	GSList *list, *next;
	struct away_msg *away;

	list = away_list;
	while (list)
	{
		away = list->data;
		next = list->next;
		if (away->server == serv)
		{
			away_list = g_slist_remove (away_list, away);
			if (away->message)
				free (away->message);
			free (away);
			next = away_list;
		}
		list = next;
	}
}

void
save_away_message (struct server *serv, char *nick, char *msg)
{
	struct away_msg *away = find_away_message (serv, nick);

	if (away)						  /* Change message for known user */
	{
		if (away->message)
			free (away->message);
		away->message = strdup (msg);
	} else
		/* Create brand new entry */
	{
		away = malloc (sizeof (struct away_msg));
		if (away)
		{
			away->server = serv;
			safe_strcpy (away->nick, nick, sizeof (away->nick));
			away->message = strdup (msg);
			away_list = g_slist_prepend (away_list, away);
		}
	}
}

#define defaultconf_ctcp \
	"NAME TIME\n"				"CMD /nctcp %s TIME %t\n\n"\
	"NAME PING\n"				"CMD /nctcp %s PING %d\n\n"

#define defaultconf_popup \
	"NAME SUB\n"				"CMD Direct Client-To-Client\n\n"\
	"NAME Send File\n"		"CMD /dcc send %s\n\n"\
	"NAME Offer Chat\n"		"CMD /dcc chat %s\n\n"\
	"NAME Abort Chat\n"		"CMD /dcc close chat %s\n\n"\
	"NAME ENDSUB\n"			"CMD \n\n"\
	"NAME SUB\n"				"CMD CTCP\n\n"\
	"NAME Version\n"			"CMD /ctcp %s VERSION\n\n"\
	"NAME Userinfo\n"			"CMD /ctcp %s USERINFO\n\n"\
	"NAME Clientinfo\n"		"CMD /ctcp %s CLIENTINFO\n\n"\
	"NAME Ping\n"				"CMD /ping %s\n\n"\
	"NAME Time\n"				"CMD /ctcp %s TIME\n\n"\
	"NAME Finger\n"			"CMD /ctcp %s FINGER\n\n"\
	"NAME XDCC List\n"		"CMD /ctcp %s XDCC LIST\n\n"\
	"NAME CDCC List\n"		"CMD /ctcp %s CDCC LIST\n\n"\
	"NAME ENDSUB\n"			"CMD \n\n"\
	"NAME SUB\n"				"CMD Oper\n\n"\
	"NAME Kill this user\n"	"CMD /quote KILL %s :die!\n\n"\
	"NAME ENDSUB\n"			"CMD \n\n"\
	"NAME SUB\n"				"CMD Mode\n\n"\
	"NAME Give Voice\n"		"CMD /voice %s\n\n"\
	"NAME Take Voice\n"		"CMD /devoice %s\n"\
	"NAME SEP\n"				"CMD \n\n"\
	"NAME Give Ops\n"			"CMD /op %s\n\n"\
	"NAME Take Ops\n"			"CMD /deop %s\n\n"\
	"NAME ENDSUB\n"			"CMD \n\n"\
	"NAME SUB\n"				"CMD Ignore\n\n"\
	"NAME Ignore User\n"		"CMD /ignore %s!*@* ALL\n\n"\
	"NAME UnIgnore User\n"	"CMD /unignore %s!*@*\n\n"\
	"NAME ENDSUB\n"			"CMD \n\n"\
	"NAME SUB\n"				"CMD Kick/Ban\n\n"\
	"NAME Kick\n"				"CMD /kick %s\n\n"\
	"NAME Ban\n"				"CMD /ban %s\n\n"\
	"NAME SEP\n"				"CMD \n\n"\
	"NAME Ban *!*@*.host\n"	"CMD /ban %s 0\n\n"\
	"NAME Ban *!*@domain\n"	"CMD /ban %s 1\n\n"\
	"NAME Ban *!*user@*.host\n""CMD /ban %s 2\n\n"\
	"NAME Ban *!*user@domain\n""CMD /ban %s 3\n\n"\
	"NAME SEP\n"				"CMD \n\n"\
	"NAME KickBan *!*@*.host\n""CMD /kickban %s 0\n\n"\
	"NAME KickBan *!*@domain\n""CMD /kickban %s 1\n\n"\
	"NAME KickBan *!*user@*.host\n""CMD /kickban %s 2\n\n"\
	"NAME KickBan *!*user@domain\n""CMD /kickban %s 3\n\n"\
	"NAME ENDSUB\n"			"CMD \n\n"\
	"NAME SUB\n"				"CMD Info\n\n"\
	"NAME Who\n"				"CMD /quote WHO %s\n\n"\
	"NAME Whois\n"				"CMD /quote WHOIS %s\n\n"\
	"NAME DNS Lookup\n"		"CMD /dns %s\n\n"\
	"NAME Trace\n"				"CMD /quote TRACE %s\n\n"\
	"NAME UserHost\n"			"CMD /quote USERHOST %s\n\n"\
	"NAME ENDSUB\n"			"CMD \n\n"\
	"NAME SUB\n"				"CMD External\n\n"\
	"NAME Traceroute\n"		"CMD !x-terminal-emulator -e /bin/sh -c \"/usr/sbin/traceroute %h ; sleep 30\"\n\n"\
	"NAME Ping\n"				"CMD  !x-terminal-emulator -e /bin/sh -c \"ping -c 4 %h ; sleep 30\"\n\n"\
	"NAME Telnet\n"			"CMD !x-terminal-emulator -e telnet %h\n\n"\
	"NAME ENDSUB\n"			"CMD \n\n"\
	"NAME Open Query\n"		"CMD /query %s\n\n"

#define defaultconf_buttons \
	"NAME Op\n"					"CMD /op %a\n\n"\
	"NAME DeOp\n"				"CMD /deop %a\n\n"\
	"NAME Ban\n"				"CMD /ban %s\n\n"\
	"NAME Kick\n"				"CMD /kick %s\n\n"\
	"NAME Sendfile\n"			"CMD /dcc send %s\n\n"\
	"NAME Dialog\n"			"CMD /query %s\n\n"

#define defaultconf_dlgbuttons \
	"NAME Whois\n"				"CMD /whois %s\n\n"\
	"NAME Send\n"				"CMD /dcc send %s\n\n"\
	"NAME Chat\n"				"CMD /dcc chat %s\n\n"\
	"NAME Ping\n"				"CMD /ping %s\n\n"\
	"NAME Clear\n"				"CMD /clear\n\n"\

#define defaultconf_replace \
	"NAME teh\n"				"CMD the\n\n"\
	"NAME r\n"					"CMD are\n\n"\
	"NAME u\n"					"CMD you\n\n"

#define defaultconf_commands \
   "NAME ACTION\n"		"CMD /me &2\n\n"\
	"NAME AME\n"			"CMD /allchan /me &2\n\n"\
	"NAME AMSG\n"			"CMD /allchan &2\n\n"\
	"NAME BACK\n"			"CMD /away\n\n"\
   "NAME BANLIST\n"		"CMD /quote MODE %c +b\n\n"\
   "NAME CHAT\n"			"CMD /dcc chat %2\n\n"\
   "NAME DIALOG\n"		"CMD /query %2\n\n"\
   "NAME DMSG\n"			"CMD /msg =%2 &3\n\n"\
   "NAME EXIT\n"			"CMD /quit\n\n"\
   "NAME J\n"				"CMD /join &2\n\n"\
   "NAME KILL\n"			"CMD /quote KILL %2 :&3\n\n"\
   "NAME LEAVE\n"			"CMD /part &2\n\n"\
   "NAME M\n"				"CMD /msg &2\n\n"\
   "NAME ONOTICE\n"		"CMD /notice @%c &2\n\n"\
   "NAME RAW\n"			"CMD /quote &2\n\n"\
   "NAME SERVHELP\n"		"CMD /quote HELP\n\n"\
	"NAME SPING\n"			"CMD /ping\n\n"\
	"NAME SSLSERVER\n"	"CMD /server -ssl &2\n\n"\
   "NAME SV\n"				"CMD /echo xchat %v %m\n\n"\
   "NAME UMODE\n"			"CMD /mode %n &2\n\n"\
   "NAME UPTIME\n"		"CMD /quote STATS u\n\n"\
   "NAME VER\n"			"CMD /ctcp %2 VERSION\n\n"\
   "NAME VERSION\n"		"CMD /ctcp %2 VERSION\n\n"\
   "NAME WALLOPS\n"		"CMD /quote WALLOPS :&2\n\n"\
   "NAME WII\n"			"CMD /quote WHOIS %2 %2\n\n"

#define defaultconf_usermenu \
	"NAME SUB\n"					"CMD IRC Stuff\n\n"\
   "NAME Disconnect\n"			"CMD /discon\n\n"\
   "NAME Reconnect\n"			"CMD /reconnect\n\n"\
	"NAME Part Channel\n"		"CMD /part\n\n"\
   "NAME Cycle Channel\n"		"CMD /cycle\n\n"\
   "NAME Server Map\n"			"CMD /quote MAP\n\n"\
	"NAME Server Links\n"		"CMD /quote LINKS\n\n"\
   "NAME Ping Server\n"			"CMD /ping\n\n"\
   "NAME ENDSUB\n"				"CMD \n\n"\
	"NAME SUB\n"					"CMD Connect\n\n"\
   "NAME irc.xchat.org #Linux\n""CMD /servchan irc.xchat.org 6667 #linux\n\n"\
 	"NAME Go to EFNet\n"			"CMD /newserver irc.efnet.net\n\n"\
   "NAME ENDSUB\n"				"CMD \n\n"\
	"NAME SUB\n"					"CMD Settings\n\n"\
	"NAME TOGGLE Hide Version\n"		"CMD hide_version\n\n"\
	"NAME TOGGLE Colored Nicks\n"		"CMD colorednicks\n\n"\
	"NAME TOGGLE 1.4.x Nick Comp.\n"	"CMD old_nickcompletion\n\n"\
	"NAME TOGGLE Strip mIRC color\n"	"CMD stripcolor\n\n"\
	"NAME TOGGLE Filter Beeps\n"		"CMD filterbeep\n\n"\
	"NAME TOGGLE Raw MODE Display\n"	"CMD raw_modes\n\n"\
	"NAME TOGGLE Perl Warnings\n"		"CMD perlwarnings\n\n"\
	"NAME TOGGLE Mail Checker\n"		"CMD mail_check\n\n"\
   "NAME ENDSUB\n"				"CMD \n\n"\
	"NAME SUB\n"					"CMD External\n\n"\
   "NAME Run XMMS\n"				"CMD !xmms\n\n"\
	"NAME Open Terminal\n"                          "CMD !x-terminal-emulator\n\n"\
   "NAME ENDSUB\n"				"CMD \n\n"

#ifdef WIN32
#define defaultconf_urlhandlers \
	"NAME Open URL\n"							"CMD !start %s\n\n"\
	"NAME Connect as IRC server\n"		"CMD /newserver %s\n\n"
#else
#define defaultconf_urlhandlers \
	"NAME SUB\n"								"CMD Netscape...\n\n"\
   	"NAME Open in existing\n"			"CMD !netscape -remote 'openURL(%s)'\n\n"\
   	"NAME Open in new window\n"		"CMD !netscape -remote 'openURL(%s,new-window)'\n\n"\
   	"NAME Run new Netscape\n"			"CMD !netscape %s\n\n"\
	"NAME ENDSUB\n"							"CMD \n\n"\
	"NAME SUB\n"								"CMD Mozilla...\n\n"\
   	"NAME Open in existing\n"			"CMD !mozilla -remote 'openURL(%s)'\n\n"\
   	"NAME Open in new window\n"		"CMD !mozilla -remote 'openURL(%s,new-window)'\n\n"\
   	"NAME Run new Mozilla\n"			"CMD !mozilla %s\n\n"\
	"NAME ENDSUB\n"							"CMD \n\n"\
	"NAME SUB\n"								"CMD Galeon...\n\n"\
		"NAME Open in existing\n"			"CMD !galeon -x '%s'\n\n"\
   	"NAME Open in new window\n"		"CMD !galeon -w '%s'\n\n"\
   	"NAME Open in new tab\n"			"CMD !galeon -n '%s'\n\n"\
		"NAME Run new Galeon\n"				"CMD !galeon '%s'\n\n"\
	"NAME ENDSUB\n"							"CMD \n\n"\
	"NAME SUB\n"								"CMD Opera...\n\n"\
   	"NAME Open in existing\n"			"CMD !opera -remote 'openURL(%s)'\n\n"\
   	"NAME Open in new window\n"		"CMD !opera -remote 'openURL(%s,new-window)'\n\n"\
   	"NAME Run new Opera\n"				"CMD !opera %s\n\n"\
	"NAME ENDSUB\n"							"CMD \n\n"\
	"NAME SUB\n"								"CMD Send URL to...\n\n"\
   	"NAME Gnome URL Handler\n"			"CMD !gnome-moz-remote %s\n\n"\
   	"NAME Lynx\n"							"CMD !x-terminal-emulator -e lynx %s\n\n"\
   	"NAME Links\n"							"CMD !x-terminal-emulator -e links %s\n\n"\
   	"NAME w3m\n"							"CMD !x-terminal-emulator -e w3m %s\n\n"\
   	"NAME Mutt (for mail)\n"                                        "CMD !x-terminal-emulator -e mutt %s\n\n"\
	"NAME NcFTP\n" 						"CMD !x-terminal-emulator -e ncftp %s\n\n"\
   	"NAME gFTP\n"							"CMD !gftp %s\n\n"\
		"NAME KFM\n"							"CMD !kfmclient openURL %s\n\n"\
		"NAME Telnet\n"						"CMD !x-terminal-emulator -e telnet %s\n\n"\
		"NAME Ping\n"							"CMD !x-terminal-emulator -e ping -c 4 %s\n\n"\
	"NAME ENDSUB\n"							"CMD \n\n"\
	"NAME Connect as IRC server\n"		"CMD /newserver %s\n\n"
#endif

static void
xchat_init (void)
{
#ifdef WIN32
	WSADATA wsadata;

	WSAStartup(0x0101, &wsadata);
#else
	struct sigaction act;

	/* ignore SIGPIPE's */
	act.sa_handler = SIG_IGN;
	act.sa_flags = 0;
	sigemptyset (&act.sa_mask);
	sigaction (SIGPIPE, &act, NULL);
#endif

	signal_setup ();
	load_text_events ();
	notify_load ();
	ignore_load ();
	list_loadconf ("popup.conf", &popup_list, defaultconf_popup);
	list_loadconf ("ctcpreply.conf", &ctcp_list, defaultconf_ctcp);
	list_loadconf ("buttons.conf", &button_list, defaultconf_buttons);
	list_loadconf ("dlgbuttons.conf", &dlgbutton_list, defaultconf_dlgbuttons);
	list_loadconf ("commands.conf", &command_list, defaultconf_commands);
	list_loadconf ("replace.conf", &replace_list, defaultconf_replace);
	list_loadconf ("usermenu.conf", &usermenu_list, defaultconf_usermenu);
	list_loadconf ("urlhandlers.conf", &urlhandler_list,
						defaultconf_urlhandlers);

#ifdef USE_TRANS
	if (prefs.use_trans)
	{
		if (load_trans_table (prefs.trans_file) == 0)
			prefs.use_trans = 0;
	}
#endif

	/* if serverlist didnt open any irc windows .... */
	if (!fe_open_serverlist (NULL, auto_connect, prefs.skipserverlist))
	{
		/* and serverlist isn't open .... */
		if (prefs.skipserverlist)
			/* we'll have to open one. */
			new_ircwindow (NULL, NULL, SESS_SERVER);
	}
}

void
xchat_exit (void)
{
	xchat_is_quitting = TRUE;
	in_xchat_exit = TRUE;
#ifdef USE_PERL
	perl_end ();
#endif
#ifdef USE_PYTHON
	pys_kill ();
#endif
	fe_cleanup ();
	if (prefs.autosave)
	{
		save_config ();
		pevent_save (NULL);
	}
	notify_save ();
	ignore_save ();
	free_sessions ();
	fe_exit ();
}

#ifndef WIN32

static int
child_handler (int pid)
{
	if (waitpid (pid, 0, WNOHANG) == pid)
		return 0;					  /* remove timeout handler */
	return 1;						  /* keep the timeout handler */
}

#endif

void
xchat_exec (char *cmd)
{
#ifdef WIN32
	util_exec (cmd);
#else
	int pid = util_exec (cmd);
	if (pid != -1)
	/* zombie avoiding system. Don't ask! it has to be like this to work
      with zvt (which overrides the default handler) */
		fe_timeout_add (1000, child_handler, (void *)pid);
#endif
}

int
main (int argc, char *argv[])
{

#ifdef SOCKS
	SOCKSinit (argv[0]);
#endif

#ifdef USE_OPENSSL
	if (!(ctx = _SSL_context_init (ssl_cb_info, FALSE))) {
		fprintf(stderr, "_SSL_context_init failed\n");
		exit(1);
	}
#endif

	if (!fe_args (argc, argv))
		return 0;

	load_config ();

	fe_init ();

	xchat_init ();

	fe_main ();

#ifdef USE_DEBUG
	xchat_mem_list ();
#endif

#ifdef USE_OPENSSL
	_SSL_context_free (ctx);
#endif

	return 0;
}
