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
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#define WANTSOCKET
#define WANTARPA
#include "inet.h"

#ifndef WIN32
#include <signal.h>
#include <sys/wait.h>
#else
#include <winbase.h>
#endif

#include "xchat.h"
#include "plugin.h"
#include "fe.h"
#include "cfgfiles.h"
#include "network.h"
#include "notify.h"
#include "xchatc.h"
#include "inbound.h"
#include "outbound.h"
#include "text.h"
#include "util.h"
#include "server.h"

#ifdef USE_OPENSSL
#include <openssl/ssl.h>		  /* SSL_() */
#include <openssl/err.h>		  /* ERR_() */
#include "ssl.h"
#endif

#ifdef USE_JCODE
#include "jcode.h"
#endif

#ifdef WIN32
#include "identd.c"
#endif

#ifdef USE_OPENSSL
extern SSL_CTX *ctx;				  /* xchat.c */
/* local variables */
static struct session *g_sess = NULL;
#endif

static void server_stopconnecting (server * serv);
static gboolean
read_data (GIOChannel *source, GIOCondition condition, server *serv);


static int
close_socket_cb (int sok)
{
	closesocket (sok);
	return 0;
}

static void
close_socket (int sok)
{
	/* close the socket in 5 seconds so the QUIT message is not lost */
	fe_timeout_add (5000, close_socket_cb, (void *) sok);
}

static void
server_connected (server * serv)
{
	char hostname[256];
	char outbuf[512];

	serv->ping_recv = time (0);

#ifdef WIN32
	identd_start ();
#else
	sprintf (outbuf, "%s/auth/xchat_auth", g_get_home_dir ());
	if (access (outbuf, X_OK) == 0)
	{
		sprintf (outbuf, "/exec %s/auth/xchat_auth %s", g_get_home_dir (),
					prefs.username);
		handle_command (outbuf, serv->front_session, FALSE, FALSE);
	}
#endif

	serv->connected = TRUE;
	serv->iotag = fe_input_add (serv->sok, 1, 0, 1, read_data, serv);
	if (!serv->no_login)
	{
		EMIT_SIGNAL (XP_TE_CONNECTED, serv->front_session, NULL, NULL, NULL,
						 NULL, 0);
		if (serv->password[0])
		{
			sprintf (outbuf, "PASS %s\r\n", serv->password);
			tcp_send (serv, outbuf);
		}
		gethostname (hostname, sizeof (hostname) - 1);
		hostname[sizeof (hostname) - 1] = 0;
		if (hostname[0] == 0)
			strcpy (hostname, "0");
		snprintf (outbuf, sizeof (outbuf),
					 "NICK %s\r\n"
					 "USER %s %s %s :%s\r\n",
					 serv->nick, prefs.username, hostname,
					 serv->servername, prefs.realname);
		tcp_send (serv, outbuf);
	} else
	{
		EMIT_SIGNAL (XP_TE_SERVERCONNECTED, serv->front_session, NULL, NULL,
						 NULL, NULL, 0);
	}

	set_nonblocking (serv->sok);
	set_server_name (serv, serv->servername);
}

#ifdef USE_OPENSSL
#define	SSLTMOUT	10				  /* seconds */
void
ssl_cb_info (SSL * s, int where, int ret)
{
/*	char buf[128];*/


	return;							  /* FIXME: make debug level adjustable in serverlist or settings */

/*	snprintf (buf, sizeof (buf), "%s (%d)", SSL_state_string_long (s), where);
	if (g_sess)
		EMIT_SIGNAL (XP_TE_SERVTEXT, g_sess, buf, NULL, NULL, NULL, 0);
	else
		fprintf (stderr, "%s\n", buf);*/
}

static int
ssl_cb_verify (int ok, X509_STORE_CTX * ctx)
{
	char subject[256];
	char issuer[256];
	char buf[512];


	X509_NAME_oneline (X509_get_subject_name (ctx->current_cert), subject,
							 sizeof (subject));
	X509_NAME_oneline (X509_get_issuer_name (ctx->current_cert), issuer,
							 sizeof (issuer));

	snprintf (buf, sizeof (buf), "* Subject: %s", subject);
	EMIT_SIGNAL (XP_TE_SERVTEXT, g_sess, buf, NULL, NULL, NULL, 0);
	snprintf (buf, sizeof (buf), "* Issuer: %s", issuer);
	EMIT_SIGNAL (XP_TE_SERVTEXT, g_sess, buf, NULL, NULL, NULL, 0);

	return (TRUE);					  /* always ok */
}

static int
ssl_do_connect (server * serv)
{
	char buf[128];


	g_sess = serv->front_session;
	if (SSL_connect (serv->ssl) <= 0)
	{
		char err_buf[128];
		int err;

		g_sess = NULL;
		if ((err = ERR_get_error ()) > 0)
		{
			ERR_error_string (err, err_buf);
			snprintf (buf, sizeof (buf), "(%d) %s", err, err_buf);
			EMIT_SIGNAL (XP_TE_CONNFAIL, serv->front_session, buf, NULL,
							 NULL, NULL, 0);

			server_cleanup (serv);

			if (prefs.autoreconnectonfail)
				auto_reconnect (serv, FALSE, -1);

			return (0);				  /* remove it (0) */
		}
	}
	g_sess = NULL;

	if (SSL_is_init_finished (serv->ssl))
	{
		struct cert_info cert_info;
		struct chiper_info *chiper_info;
		int verify_error;
		int i;

		if (!_SSL_get_cert_info (&cert_info, serv->ssl))
		{
			snprintf (buf, sizeof (buf), "* Certification info:");
			EMIT_SIGNAL (XP_TE_SERVTEXT, serv->front_session, buf, NULL, NULL,
							 NULL, 0);
			snprintf (buf, sizeof (buf), "  Subject:");
			EMIT_SIGNAL (XP_TE_SERVTEXT, serv->front_session, buf, NULL, NULL,
							 NULL, 0);
			for (i = 0; cert_info.subject_word[i]; i++)
			{
				snprintf (buf, sizeof (buf), "    %s", cert_info.subject_word[i]);
				EMIT_SIGNAL (XP_TE_SERVTEXT, serv->front_session, buf, NULL, NULL,
								 NULL, 0);
			}
			snprintf (buf, sizeof (buf), "  Issuer:");
			EMIT_SIGNAL (XP_TE_SERVTEXT, serv->front_session, buf, NULL, NULL,
							 NULL, 0);
			for (i = 0; cert_info.issuer_word[i]; i++)
			{
				snprintf (buf, sizeof (buf), "    %s", cert_info.issuer_word[i]);
				EMIT_SIGNAL (XP_TE_SERVTEXT, serv->front_session, buf, NULL, NULL,
								 NULL, 0);
			}
			snprintf (buf, sizeof (buf), "  Public key algorithm: %s (%d bits)",
						 cert_info.algorithm, cert_info.algorithm_bits);
			EMIT_SIGNAL (XP_TE_SERVTEXT, serv->front_session, buf, NULL, NULL,
							 NULL, 0);
			if (cert_info.rsa_tmp_bits)
			{
				snprintf (buf, sizeof (buf),
							 "  Public key algorithm uses ephemeral key with %d bits",
							 cert_info.rsa_tmp_bits);
				EMIT_SIGNAL (XP_TE_SERVTEXT, serv->front_session, buf, NULL, NULL,
								 NULL, 0);
			}
			snprintf (buf, sizeof (buf), "  Sign algorithm %s (%d bits)",
						 cert_info.sign_algorithm, cert_info.sign_algorithm_bits);
			EMIT_SIGNAL (XP_TE_SERVTEXT, serv->front_session, buf, NULL, NULL,
							 NULL, 0);
			snprintf (buf, sizeof (buf), "  Valid since %s to %s",
						 cert_info.notbefore, cert_info.notafter);
			EMIT_SIGNAL (XP_TE_SERVTEXT, serv->front_session, buf, NULL, NULL,
							 NULL, 0);
		} else
		{
			snprintf (buf, sizeof (buf), " * No Certificate");
			EMIT_SIGNAL (XP_TE_SERVTEXT, serv->front_session, buf, NULL, NULL,
							 NULL, 0);
		}

		chiper_info = _SSL_get_cipher_info (serv->ssl);	/* static buffer */
		snprintf (buf, sizeof (buf), "* Chiper info:");
		EMIT_SIGNAL (XP_TE_SERVTEXT, serv->front_session, buf, NULL, NULL, NULL,
						 0);
		snprintf (buf, sizeof (buf), "  Version: %s, cipher %s (%u bits)",
					 chiper_info->version, chiper_info->chiper,
					 chiper_info->chiper_bits);
		EMIT_SIGNAL (XP_TE_SERVTEXT, serv->front_session, buf, NULL, NULL, NULL,
						 0);

		verify_error = SSL_get_verify_result (serv->ssl);
		switch (verify_error)
		{
		case X509_V_OK:
			/* snprintf (buf, sizeof (buf), "* Verify OK (?)"); */
			/* EMIT_SIGNAL (XP_TE_SERVTEXT, serv->front_session, buf, NULL, NULL, NULL, 0); */
			break;
		case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
		case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE:
		case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
			if (serv->accept_invalid_cert)
			{
				snprintf (buf, sizeof (buf), "* Verify E: %s.? (%d) -- Ignored",
							 X509_verify_cert_error_string (verify_error),
							 verify_error);
				EMIT_SIGNAL (XP_TE_SERVTEXT, serv->front_session, buf, NULL, NULL,
								 NULL, 0);
				break;
			}
		default:
			snprintf (buf, sizeof (buf), "%s.? (%d)",
						 X509_verify_cert_error_string (verify_error),
						 verify_error);
			EMIT_SIGNAL (XP_TE_CONNFAIL, serv->front_session, buf, NULL, NULL,
							 NULL, 0);

			server_cleanup (serv);

			return (0);
		}

		server_stopconnecting (serv);

		/* activate gtk poll */
		server_connected (serv);

		return (0);					  /* remove it (0) */
	} else
	{
		if (serv->ssl->session->time + SSLTMOUT < time (NULL))
		{
			snprintf (buf, sizeof (buf), "SSL handshake timed out");
			EMIT_SIGNAL (XP_TE_CONNFAIL, serv->front_session, buf, NULL,
							 NULL, NULL, 0);
			server_cleanup (serv); /* ->connecting = FALSE */

			if (prefs.autoreconnectonfail)
				auto_reconnect (serv, FALSE, -1);

			return (0);				  /* remove it (0) */
		}

		return (1);					  /* call it more (1) */
	}
}
#endif

static int
timeout_auto_reconnect (struct server *serv)
{
	if (is_server (serv))		  /* make sure it hasnt been closed during the delay */
	{
		serv->recondelay_tag = 0;
		if (!serv->connected && !serv->connecting && serv->front_session)
		{
			connect_server (serv->front_session, serv->hostname, serv->port,
								 FALSE);
		}
	}
	return 0;						  /* returning 0 should remove the timeout handler */
}

void
auto_reconnect (struct server *serv, int send_quit, int err)
{
	session *s;
	GSList *list;
	int del;

	if (serv->front_session == NULL)
		return;

	list = sess_list;
	while (list)				  /* make sure auto rejoin can work */
	{
		s = list->data;
		if (s->type == SESS_CHANNEL && s->channel[0])
		{
			strcpy (s->waitchannel, s->channel);
			strcpy (s->willjoinchannel, s->channel);
		}
		list = list->next;
	}

	if (serv->connected)
		disconnect_server (serv->front_session, send_quit, err);

	del = prefs.recon_delay * 1000;
	if (del < 1000)
		del = 500;				  /* so it doesn't block the gui */

#ifndef WIN32
	if (err == 0 || err == ECONNRESET || err == ETIMEDOUT)
#else
	if (err == 0 || err == WSAECONNRESET || err == WSAETIMEDOUT)
#endif
		serv->reconnect_away = serv->is_away;

	serv->recondelay_tag = fe_timeout_add (del, timeout_auto_reconnect, serv);
}

static gboolean
read_data (GIOChannel *source, GIOCondition condition, server *serv)
{
	int sok = serv->sok;
	int error, i, len;
	char lbuf[2050];
	char *temp;
#ifdef USE_JCODE
	char *jtemp;
#endif

	while (1)
	{
		if (!EMIT_SIGNAL (XP_IF_RECV, &len, (void *) sok, &lbuf,
								(void *) ((sizeof (lbuf)) - 2), NULL, 0))
		{
#ifdef USE_OPENSSL
			if (!serv->ssl)
#endif
				len = recv (sok, lbuf, sizeof (lbuf) - 2, 0);
#ifdef USE_OPENSSL
			else
				len = _SSL_recv (serv->ssl, lbuf, sizeof (lbuf) - 2);
#endif
		}
		if (len < 1)
		{
			if (len < 0)
			{
				if (would_block_again ())
					return TRUE;
				error = sock_error ();
			} else
			{
				error = 0;
			}
			if (prefs.autoreconnect)
				auto_reconnect (serv, FALSE, error);
			else
				disconnect_server (serv->front_session, FALSE, error);
			return TRUE;
		} else
		{
			i = 0;

			lbuf[len] = 0;

			while (i < len)
			{
				switch (lbuf[i])
				{
				case '\r':
					break;

				case '\n':
					serv->linebuf[serv->pos] = 0;
#ifdef USE_TRANS
					if (prefs.use_trans)
						serv2user (serv->linebuf);
#endif
					if (prefs.stripcolor)
					{
						temp = strip_color (serv->linebuf);
#ifdef USE_JCODE
						if (prefs.kanji_conv)
						{
							jtemp = kanji_conv_to_locale (temp);
							if (jtemp)
							{
								process_line (serv, jtemp);
								free (jtemp);
							} else
								process_line (serv, temp);
						} else
							process_line (serv, temp);
#else
						process_line (serv, temp);
#endif
						free (temp);
					} else
					{
#ifdef USE_JCODE
						if (prefs.kanji_conv)
						{
							jtemp = kanji_conv_to_locale (serv->linebuf);
							if (jtemp)
							{
								process_line (serv, jtemp);
								free (jtemp);
							} else
								process_line (serv, serv->linebuf);
						} else
							process_line (serv, serv->linebuf);
#else
						process_line (serv, serv->linebuf);
#endif
					}
					serv->pos = 0;
					break;

				default:
					serv->linebuf[serv->pos] = lbuf[i];
					if (serv->pos > 519)
						fprintf (stderr,
									"*** XCHAT WARNING: Buffer overflow - shit server!\n");
					else
						serv->pos++;
				}
				i++;
			}
		}
	}
}

void
flush_server_queue (struct server *serv)
{
	list_free (&serv->outbound_queue);
	serv->sendq_len = 0;
	fe_set_throttle (serv);
}

#ifdef WIN32

static int
waitline2 (GIOChannel *source, char *buf, int bufsize)
{
	int i = 0;
	int len;

	while (1)
	{
		if (g_io_channel_read (source, &buf[i], 1, &len) != G_IO_ERROR_NONE)
			return -1;
		if (buf[i] == '\n' || bufsize == i + 1)
		{
			buf[i] = 0;
			return i;
		}
		i++;
	}
}

#else

#define waitline2(source,buf,size) waitline(serv->childread,buf,size)

#endif

static gboolean
connected_signal (GIOChannel *source, GIOCondition condition, server *serv)
{
	session *sess = serv->front_session;
	char tbuf[128];
	char outbuf[512];
	char host[100];
	char ip[100];

	waitline2 (source, tbuf, sizeof tbuf);

	switch (tbuf[0])
	{
	case '1':						  /* unknown host */
		server_stopconnecting (serv);
		closesocket (serv->sok4);
		if (serv->sok6 >= 0)
			closesocket (serv->sok6);
		EMIT_SIGNAL (XP_TE_UKNHOST, sess, NULL, NULL, NULL, NULL, 0);
		if (prefs.autoreconnectonfail)
			auto_reconnect (serv, FALSE, -1);
		break;
	case '2':						  /* connection failed */
		waitline2 (source, tbuf, sizeof tbuf);
		server_stopconnecting (serv);
		closesocket (serv->sok4);
		if (serv->sok6 >= 0)
			closesocket (serv->sok6);
		EMIT_SIGNAL (XP_TE_CONNFAIL, sess, errorstring (atoi (tbuf)), NULL,
						 NULL, NULL, 0);
		if (prefs.autoreconnectonfail)
			auto_reconnect (serv, FALSE, -1);
		break;
	case '3':						  /* gethostbyname finished */
		waitline2 (source, host, sizeof host);
		waitline2 (source, ip, sizeof ip);
		waitline2 (source, outbuf, sizeof outbuf);
		EMIT_SIGNAL (XP_TE_CONNECT, sess, host, ip, outbuf, NULL, 0);
		break;
	case '4':						  /* success */
		waitline2 (source, tbuf, sizeof (tbuf));
		serv->sok = atoi (tbuf);
#ifdef USE_IPV6
		/* close the one we didn't end up using */
		if (serv->sok == serv->sok4)
			closesocket (serv->sok6);
		else
			closesocket (serv->sok4);
#endif
#ifdef USE_OPENSSL
#define	SSLDOCONNTMOUT	300
		if (serv->use_ssl)
		{
			char *err;

			/* it'll be a memory leak, if connection isn't terminated by
			   server_cleanup() */
			serv->ssl = _SSL_socket (ctx, serv->sok);
			if ((err = _SSL_set_verify (ctx, ssl_cb_verify, NULL)))
			{
				EMIT_SIGNAL (XP_TE_CONNFAIL, serv->front_session, err, NULL,
								 NULL, NULL, 0);

				server_cleanup (serv);	/* ->connecting = FALSE */
				return TRUE;
			}

			/* FIXME: it'll be needed by new servers */
			/* send(serv->sok, "STLS\r\n", 6, 0); sleep(1); */
			set_nonblocking (serv->sok);
			serv->ssl_do_connect_tag = fe_timeout_add (SSLDOCONNTMOUT,
																	 ssl_do_connect, serv);
		} else
		{
			serv->ssl = NULL;
#endif
			server_stopconnecting (serv);	/* ->connecting = FALSE */
			/* activate gtk poll */
			server_connected (serv);
#ifdef USE_OPENSSL
		}
#endif
		break;
	case '5':						  /* prefs ip discovered */
		waitline2 (source, tbuf, sizeof tbuf);
		prefs.local_ip = inet_addr (tbuf);
		break;
	case '7':						  /* gethostbyname (prefs.hostname) failed */
		sprintf (outbuf,
					"Cannot resolve hostname %s\nCheck your IP Settings!\n",
					prefs.hostname);
		PrintText (sess, outbuf);
		break;
	case '8':
		PrintText (sess, "Proxy traversal failed.\n");
		disconnect_server (sess, FALSE, -1);
		break;
	case '9':
		waitline2 (source, tbuf, sizeof tbuf);
		EMIT_SIGNAL (XP_TE_SERVERLOOKUP, sess, tbuf, NULL, NULL, NULL, 0);
		break;
	}

	return TRUE;
}

static void
server_stopconnecting (server * serv)
{
	if (serv->iotag)
	{
		fe_input_remove (serv->iotag);
		serv->iotag = 0;
	}

#ifndef WIN32
	/* kill the child process trying to connect */
	kill (serv->childpid, SIGKILL);
	waitpid (serv->childpid, NULL, 0);
#else
	TerminateThread ((HANDLE)serv->childpid, 0);
	CloseHandle ((HANDLE)serv->childpid);
#endif

	close (serv->childwrite);
	close (serv->childread);

#ifdef USE_OPENSSL
	if (serv->ssl_do_connect_tag)
	{
		fe_timeout_remove (serv->ssl_do_connect_tag);
		serv->ssl_do_connect_tag = 0;
	}
#endif

	fe_progressbar_end (serv->front_session);

	serv->connecting = FALSE;
}

/* kill all sockets & iotags of a server. Stop a connection attempt, or
   disconnect if already connected. */

int
server_cleanup (server * serv)
{
	fe_set_lag (serv, 0.0);

	if (serv->iotag)
	{
		fe_input_remove (serv->iotag);
		serv->iotag = 0;
	}
#ifdef USE_OPENSSL
	if (serv->ssl)
	{
		_SSL_close (serv->ssl);
		serv->ssl = NULL;
	}
#endif

	if (serv->connecting)
	{
		server_stopconnecting (serv);
		closesocket (serv->sok4);
		if (serv->sok6 >= 0)
			closesocket (serv->sok6);
		return 1;
	}

	if (serv->connected)
	{
		close_socket (serv->sok);
		serv->connected = FALSE;
		return 2;
	}

	/* is this server in a reconnect delay? remove it! */
	if (serv->recondelay_tag)
	{
		fe_timeout_remove (serv->recondelay_tag);
		serv->recondelay_tag = 0;
		return 3;
	}

	return 0;
}

void
disconnect_server (session * sess, int sendquit, int err)
{
	server *serv = sess->server;
	GSList *list;
	char tbuf[64];

	/* send our QUIT reason */
	if (sendquit && serv->connected)
		server_sendquit (sess);

	/* close all sockets & io tags */
	switch (server_cleanup (serv))
	{
	case 0:							  /* it wasn't even connected! */
		notc_msg (sess);
		return;
	case 1:							  /* it was in the process of connecting */
		sprintf (tbuf, "%d", sess->server->childpid);
		EMIT_SIGNAL (XP_TE_SCONNECT, sess, tbuf, NULL, NULL, NULL, 0);
		return;
	}

	flush_server_queue (serv);

	list = sess_list;
	while (list)					  /* print "Disconnected" to each window using this server */
	{
		sess = (struct session *) list->data;
		if (sess->server == serv)
			EMIT_SIGNAL (XP_TE_DISCON, sess, errorstring (err), NULL, NULL, NULL,
							 0);
		list = list->next;
	}

	serv->pos = 0;
	serv->motd_skipped = FALSE;
	serv->no_login = FALSE;
	serv->servername[0] = 0;

	list = sess_list;
	while (list)
	{
		sess = (struct session *) list->data;
		if (sess->server == serv)
		{
			if (sess->channel[0])
			{
				if (sess->type == SESS_CHANNEL)
				{
					clear_channel (sess);
				}
			} else
			{
				clear_channel (sess);
			}
		}
		list = list->next;
	}
	notify_cleanup ();
}

struct sock_connect
{
	char version;
	char type;
	unsigned short port;
	unsigned long address;
	char username[10];
};

/* traverse_socks() returns:
 *				0 success                *
 *          1 socks traversal failed */

static int
traverse_socks (int sok, char *serverAddr, int port)
{
	struct sock_connect sc;
	unsigned char buf[10];

	sc.version = 4;
	sc.type = 1;
	sc.port = htons (port);
	sc.address = inet_addr (serverAddr);
	strncpy (sc.username, prefs.username, 9);

	send (sok, (char *) &sc, 8 + strlen (sc.username) + 1, 0);
	buf[1] = 0;
	recv (sok, buf, 10, 0);
	if (buf[1] == 90)
		return 0;

	return 1;
}

struct sock5_connect1
{
	char version;
	char nmethods;
	char method;
};

static int
traverse_socks5 (int sok, char *serverAddr, int port)
{
	struct sock5_connect1 sc1;
	unsigned char *sc2;
	unsigned int packetlen, addrlen;
	unsigned char buf[10];

	sc1.version = 5;
	sc1.nmethods = 1;
	sc1.method = 0;
	send (sok, (char *) &sc1, 3, 0);
	if (recv (sok, buf, 2, 0) != 2)
		return 1;
	if (buf[0] != 5 && buf[1] != 0)
		return 1;

	addrlen = strlen (serverAddr);
	packetlen = 4 + 1 + addrlen + 2;
	sc2 = malloc (packetlen);
	sc2[0] = 5;						  /* version */
	sc2[1] = 1;						  /* command */
	sc2[2] = 0;						  /* reserved */
	sc2[3] = 3;						  /* address type */
	sc2[4] = (unsigned char) addrlen;	/* hostname length */
	memcpy (sc2 + 5, serverAddr, addrlen);
	*((unsigned short *) (sc2 + 5 + addrlen)) = htons (port);

	send (sok, sc2, packetlen, 0);
	/* consume all of the reply */
	if (recv (sok, buf, 4, 0) != 4)
		return 1;
	if (buf[0] != 5 && buf[1] != 0)
		return 1;
	if (buf[3] == 1)
	{
		if (recv (sok, buf, 6, 0) != 6)
			return 1;
	} else if (buf[3] == 4)
	{
		if (recv (sok, buf, 18, 0) != 18)
			return 1;
	} else if (buf[3] == 3)
	{
		if (recv (sok, buf, 1, 0) != 1)
			return 1;
		packetlen = buf[0] + 2;
		if (recv (sok, buf, packetlen, 0) != packetlen)
			return 1;
	}

	return 0;
}

static int
traverse_wingate (int sok, char *serverAddr, int port)
{
	char buf[128];

	snprintf (buf, sizeof (buf), "%s %d\r\n", serverAddr, port);
	send (sok, buf, strlen (buf), 0);

	return 0;
}

static int
traverse_http (int sok, char *serverAddr, int port)
{
	char buf[128];
	int n;

	n = snprintf (buf, sizeof (buf), "CONNECT %s:%d HTTP/1.1\r\n\r\n",
					  serverAddr, port);
	send (sok, buf, n, 0);
#ifndef WIN32
	waitline (sok, buf, sizeof (buf)); /* FIXME: win32 cant read() sok */
	/* "HTTP/1.0 200 OK" */
	if (strlen (buf) < 12)
		return 1;
	if (memcmp (buf, "HTTP/", 5) || memcmp (buf + 9, "200", 3))
		return 1;
	for (;;)
	{
		/* read until blank line */
		waitline (sok, buf, sizeof (buf));
		if (!buf[0] || (buf[0] == '\r' && !buf[1]))
			break;
	}
#endif
	return 0;
}

static int
traverse_proxy (int sok, char *ip, int port)
{
	switch (prefs.proxy_type)
	{
	case 1:
		return traverse_wingate (sok, ip, port);
	case 2:
		return traverse_socks (sok, ip, port);
	case 3:
		return traverse_socks5 (sok, ip, port);
	case 4:
		return traverse_http (sok, ip, port);
	}

	return 1;
}

/* this is the child process making the connection attempt */

static int
server_child (server * serv)
{
	netstore *ns_server;
	netstore *ns_proxy = NULL;
	netstore *ns_local;
	int port = serv->port;
	int error;
	int sok;
	char *hostname = serv->hostname;
	char *real_hostname = NULL;
	char *ip;
	char *proxy_ip = NULL;
	char *local_ip;
	int connect_port;
	FILE *fd;

	fd = fdopen (serv->childwrite, "w");

	ns_server = net_store_new ();

	/* is a hostname set? - bind to it */
	if (prefs.hostname[0])
	{
		ns_local = net_store_new ();
		local_ip = net_resolve (ns_local, prefs.hostname, 0, &real_hostname);
		if (local_ip != NULL)
		{
			fprintf (fd, "5\n%s\n", local_ip);
			net_bind (ns_local, serv->sok4, serv->sok6);
		} else
		{
			fprintf (fd, "7\n");
		}
		net_store_destroy (ns_local);
		fflush (fd);
	}

	/* first resolve where we want to connect to */
	if (!serv->dont_use_proxy && prefs.proxy_host[0] && prefs.proxy_type > 0)
	{
		fprintf (fd, "9\n%s\n", prefs.proxy_host);
		fflush (fd);
		ip = net_resolve (ns_server, prefs.proxy_host, prefs.proxy_port,
								&real_hostname);
		if (!ip)
		{
			fprintf (fd, "1\n");
			goto xit;
		}
		connect_port = prefs.proxy_port;

		/* if using socks4, attempt to resolve ip for irc server */
		if (prefs.proxy_type == 2)
		{
			ns_proxy = net_store_new ();
			proxy_ip = net_resolve (ns_proxy, hostname, port, &real_hostname);
			if (!proxy_ip)
			{
				fprintf (fd, "1\n");
				goto xit;
			}
		} else						  /* otherwise we can just use the hostname */
			proxy_ip = strdup (hostname);
	} else
	{
		ip = net_resolve (ns_server, hostname, port, &real_hostname);
		if (!ip)
		{
			fprintf (fd, "1\n");
			goto xit;
		}
		connect_port = port;
	}

	fprintf (fd, "3\n%s\n%s\n%d\n", real_hostname, ip, connect_port);
	fflush (fd);

	error = net_connect (ns_server, serv->sok4, serv->sok6, &sok);

	if (error != 0)
	{
		fprintf (fd, "2\n%d\n", sock_error ());
	} else
	{
		/* connect succeeded */
		if (proxy_ip)
		{
			switch (traverse_proxy (sok, proxy_ip, port))
			{
			case 0:
				fprintf (fd, "4\n%d\n", sok);	/* success */
				break;
			case 1:
				fprintf (fd, "8\n");	  /* socks traversal failed */
				break;
			}
		} else
		{
			fprintf (fd, "4\n%d\n", sok);
		}
	}

xit:

	fflush (fd);

	net_store_destroy (ns_server);
	if (ns_proxy)
		net_store_destroy (ns_proxy);

	/* no need to free ip/real_hostname, this process is exiting */
#ifdef WIN32
	/* under win32 we use a thread -> shared memory, must free! */
	if (proxy_ip)
		free (proxy_ip);
	if (ip)
		free (ip);
	if (real_hostname)
		free (real_hostname);
#endif

	return 0;
}

void
connect_server (session * sess, char *hostname, int port, int no_login)
{
	int pid, read_des[2];
	server *serv = sess->server;

	if (!hostname[0])
		return;

	sess = serv->front_session;

	if (serv->connected || serv->connecting || serv->recondelay_tag)
		disconnect_server (sess, TRUE, -1);

	fe_progressbar_start (sess);

	EMIT_SIGNAL (XP_TE_SERVERLOOKUP, sess, hostname, NULL, NULL, NULL, 0);

	strcpy (serv->servername, hostname);
	strcpy (serv->hostname, hostname);

	set_server_defaults (serv);
	serv->connecting = TRUE;
	serv->port = port;
	serv->no_login = no_login;

	fe_set_away (serv);
	flush_server_queue (serv);

	/* pipe is #defined on win32 in gwin32.h to use _pipe and O_BINARY */
	if (pipe (read_des) < 0)
		return;
#ifdef __EMX__ /* os/2 */
	setmode (read_des[0], O_BINARY);
	setmode (read_des[1], O_BINARY);
#endif
	serv->childread = read_des[0];
	serv->childwrite = read_des[1];

	/* create both sockets now, drop one later */
	net_sockets (&serv->sok4, &serv->sok6);

#ifdef WIN32
	pid = (int)CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE)server_child,
									 serv, 0, (DWORD *)&pid);
#else
	switch (pid = fork ())
	{
	case -1:
		return;

	case 0:
		/* this is the child */
		setuid (getuid ());
		server_child (serv);
		_exit (0);
	}
#endif
	serv->childpid = pid;
	/* the 3 tells input_add that it's a fd, not a socket */
	serv->iotag =
		fe_input_add (serv->childread, 3, 0, 0, connected_signal, serv);
}
