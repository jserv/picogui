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
#include <ctype.h>
#include <limits.h>

#define WANTSOCKET
#define WANTARPA
#include "inet.h"

#ifndef WIN32
#include <sys/wait.h>
#endif

#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "xchat.h"
#include "plugin.h"
#include "ignore.h"
#include "util.h"
#include "fe.h"
#include "cfgfiles.h"			  /* get_xdir() */
#include "network.h"				/* net_ip() */
#include "modes.h"
#include "notify.h"
#include "inbound.h"
#include "text.h"
#include "xchatc.h"
#include "server.h"
#include "perlc.h"
#include "pythonc.h"
#include "outbound.h"


#ifdef MEMORY_DEBUG
extern int current_mem_usage;
#endif

#ifdef USE_TRANS

static unsigned char trans_serv2user[256];
static unsigned char trans_user2serv[256];

void
serv2user (unsigned char *s)
{
	for (; *s; ++s)
		*s = trans_serv2user[*s];
}

void
user2serv (unsigned char *s)
{
	for (; *s; ++s)
		*s = trans_user2serv[*s];
}

int
load_trans_table (char *full_path)
{
	int tf, i, st, val = 0, t;
	char r;

	if ((tf = open (full_path, OFLAGS | O_RDONLY)) != -1)
	{
		st = 0;
		i = 0;
		t = 0;
		while (read (tf, &r, 1) == 1)
		{
			switch (st)
			{
			case 0:					  /*nothing yet... */
				if (r == '0')
					st = 1;
				break;
			case 1:
				if (r == 'x')
					st = 2;
				else
					st = 0;
				break;
			case 2:
				if (r <= '9' && r >= '0')
					val = 16 * (r - '0');
				else if (r <= 'f' && r >= 'a')
					val = 16 * (r - 'a' + 10);
				else if (r <= 'F' && r >= 'A')
					val = 16 * (r - 'A' + 10);
				else
				{
					st = 0;
					break;
				}
				st = 3;
				break;
			case 3:
				if (r <= '9' && r >= '0')
					val += r - '0';
				else if (r <= 'f' && r >= 'a')
					val += r - 'a' + 10;
				else if (r <= 'F' && r >= 'A')
					val += r - 'A' + 10;
				else
				{
					st = 0;
					break;
				}
				st = 0;
				if (t == 0)
					trans_serv2user[i++] = val;
				else
					trans_user2serv[i++] = val;
				if (i == 256)
				{
					if (t == 1)
					{
						close (tf);
						return 1;
					}
					t = 1;
					i = 0;
				}
				break;
			default:				  /* impossible */
				close (tf);
				return 0;
			}
		}
		close (tf);
	}
	for (tf = 0; tf < 256; ++tf)
	{
		trans_user2serv[tf] = tf;
		trans_serv2user[tf] = tf;
	}
	return 0;
}

#endif /* !USE_TRANS */

void
notj_msg (struct session *sess)
{
	PrintText (sess, _("No channel joined. Try /join #<channel>\n"));
}

void
notc_msg (struct session *sess)
{
	PrintText (sess, _("Not connected. Try /server <host> [<port>]\n"));
}

static char *
random_line (char *file_name)
{
	FILE *fh;
	char buf[512];
	int lines, ran;

	if (!file_name[0])
		goto nofile;

	snprintf (buf, sizeof (buf), "%s/%s", get_xdir (), file_name);
	fh = fopen (buf, "r");
	if (!fh)
	{
	 nofile:
		/* reason is not a file, an actual reason! */
		return strdup (file_name);
	}

	/* count number of lines in file */
	lines = 0;
	while (fgets (buf, sizeof (buf), fh))
		lines++;

	if (lines < 1)
		goto nofile;

	/* go down a random number */
	rewind (fh);
	srand (time (0));
	ran = rand () % lines;
	do
	{
		fgets (buf, sizeof (buf), fh);
		lines--;
	}
	while (lines > ran);
	fclose (fh);
	buf[strlen (buf) - 1] = 0;	  /* remove the trailing '\n' */
	return strdup (buf);
}

void
server_sendpart (server * serv, char *channel, char *reason)
{
	char tbuf[512];

	if (!reason)
	{
		reason = random_line (prefs.partreason);
		snprintf (tbuf, sizeof (tbuf), "PART %s :%s\r\n", channel, reason);
		free (reason);
	} else
	{
		/* reason set by /quit, /close argument */
		snprintf (tbuf, sizeof (tbuf), "PART %s :%s\r\n", channel, reason);
	}
	tcp_send (serv, tbuf);
}

void
server_sendquit (session * sess)
{
	char tbuf[512];
	char *rea;

	if (!sess->quitreason)
	{
		rea = random_line (prefs.quitreason);
		snprintf (tbuf, sizeof (tbuf), "QUIT :%s\r\n", rea);
		free (rea);
	} else
	{
		/* reason set by /quit, /close argument */
		snprintf (tbuf, sizeof (tbuf), "QUIT :%s\r\n", sess->quitreason);
	}
	tcp_send (sess->server, tbuf);
}

void
process_data_init (unsigned char *buf, char *cmd, char *word[],
						 char *word_eol[], int handle_quotes)
{
	int wordcount = 2;
	int space = FALSE;
	int quote = FALSE;
	int j = 0;

	word[1] = cmd;
	word_eol[1] = buf;

	while (1)
	{
		switch (*cmd)
		{
		case 0:
		 jump:
			buf[j] = 0;
			for (j = wordcount; j < PDIWORDS; j++)
			{
				word[j] = "\000\000";
				word_eol[j] = "\000\000";
			}
			return;
		case '\042':
			if (!handle_quotes)
				goto def;
			if (quote)
				quote = FALSE;
			else
				quote = TRUE;
			break;
		case ' ':
			if (!quote)
			{
				if (!space)
				{
					buf[j] = 0;
					j++;

					word[wordcount] = &buf[j];
					word_eol[wordcount] = cmd + 1;
					wordcount++;

					if (wordcount == PDIWORDS - 1)
						goto jump;

					space = TRUE;
				}
				break;
			}
		default:
def:
			space = FALSE;
			buf[j] = *cmd;
			j++;
		}
		cmd++;
	}
}

static int cmd_addbutton (struct session *sess, char *tbuf, char *word[],
								  char *word_eol[]);
static int cmd_allchannels (struct session *sess, char *tbuf, char *word[],
									 char *word_eol[]);
static int cmd_allservers (struct session *sess, char *tbuf, char *word[],
									char *word_eol[]);
static int cmd_away (struct session *sess, char *tbuf, char *word[],
							char *word_eol[]);
static int cmd_ban (struct session *sess, char *tbuf, char *word[],
						  char *word_eol[]);
static int cmd_unban (struct session *sess, char *tbuf, char *word[],
                                                  char *word_eol[]);
static int cmd_clear (struct session *sess, char *tbuf, char *word[],
							 char *word_eol[]);
static int cmd_close (struct session *sess, char *tbuf, char *word[],
							 char *word_eol[]);
static int cmd_ctcp (struct session *sess, char *tbuf, char *word[],
							char *word_eol[]);
static int cmd_country (struct session *sess, char *tbuf, char *word[],
								char *word_eol[]);
static int cmd_cycle (struct session *sess, char *tbuf, char *word[],
							 char *word_eol[]);
static int cmd_dcc (struct session *sess, char *tbuf, char *word[],
						  char *word_eol[]);
static int cmd_debug (struct session *sess, char *tbuf, char *word[],
							 char *word_eol[]);
static int cmd_delbutton (struct session *sess, char *tbuf, char *word[],
								  char *word_eol[]);
static int cmd_deop (struct session *sess, char *tbuf, char *word[],
							char *word_eol[]);
static int cmd_dehop (struct session *sess, char *tbuf, char *word[],
							 char *word_eol[]);
static int cmd_devoice (struct session *sess, char *tbuf, char *word[],
								char *word_eol[]);
static int cmd_discon (struct session *sess, char *tbuf, char *word[],
							  char *word_eol[]);
static int cmd_dns (struct session *sess, char *tbuf, char *word[],
						  char *word_eol[]);
static int cmd_echo (struct session *sess, char *tbuf, char *word[],
							char *word_eol[]);
#ifndef WIN32
static int cmd_exec (struct session *sess, char *tbuf, char *word[],
							char *word_eol[]);
static int cmd_execk (struct session *sess, char *tbuf, char *word[],
							 char *word_eol[]);
#ifndef __EMX__
static int cmd_execs (struct session *sess, char *tbuf, char *word[],
							 char *word_eol[]);
static int cmd_execc (struct session *sess, char *tbuf, char *word[],
							 char *word_eol[]);
static int cmd_execw (struct session *sess, char *tbuf, char *word[],
							 char *word_eol[]);
#endif
#endif
static int cmd_gate (struct session *sess, char *tbuf, char *word[],
							char *word_eol[]);
static int cmd_help (struct session *sess, char *tbuf, char *word[],
				  char *word_eol[]);
static int cmd_hop (struct session *sess, char *tbuf, char *word[],
						  char *word_eol[]);
static int cmd_ignore (struct session *sess, char *tbuf, char *word[],
							  char *word_eol[]);
static int cmd_invite (struct session *sess, char *tbuf, char *word[],
							  char *word_eol[]);
static int cmd_join (struct session *sess, char *tbuf, char *word[],
							char *word_eol[]);
static int cmd_kick (struct session *sess, char *tbuf, char *word[],
							char *word_eol[]);
static int cmd_kickban (struct session *sess, char *tbuf, char *word[],
								char *word_eol[]);
static int cmd_lastlog (struct session *sess, char *tbuf, char *word[],
								char *word_eol[]);
static int cmd_list (struct session *sess, char *tbuf, char *word[],
							char *word_eol[]);
static int cmd_load (struct session *sess, char *tbuf, char *word[],
							char *word_eol[]);
int cmd_loaddll (struct session *sess, char *tbuf, char *word[],
					  char *word_eol[]);
static int cmd_lagcheck (struct session *sess, char *tbuf, char *word[],
							 char *word_eol[]);
static int cmd_mdeop (struct session *sess, char *tbuf, char *word[],
							 char *word_eol[]);
static int cmd_mdehop (struct session *sess, char *tbuf, char *word[],
							  char *word_eol[]);
static int cmd_me (struct session *sess, char *tbuf, char *word[],
						 char *word_eol[]);
static int cmd_mkick (struct session *sess, char *tbuf, char *word[],
							 char *word_eol[]);
static int cmd_mkickb (struct session *sess, char *tbuf, char *word[],
							  char *word_eol[]);
static int cmd_msg (struct session *sess, char *tbuf, char *word[],
						  char *word_eol[]);
static int cmd_names (struct session *sess, char *tbuf, char *word[],
							 char *word_eol[]);
static int cmd_nctcp (struct session *sess, char *tbuf, char *word[],
							 char *word_eol[]);
static int cmd_newserver (struct session *sess, char *tbuf, char *word[],
								  char *word_eol[]);
static int cmd_nick (struct session *sess, char *tbuf, char *word[],
							char *word_eol[]);
static int cmd_notice (struct session *sess, char *tbuf, char *word[],
							  char *word_eol[]);
static int cmd_notify (struct session *sess, char *tbuf, char *word[],
							  char *word_eol[]);
static int cmd_op (struct session *sess, char *tbuf, char *word[],
						 char *word_eol[]);
static int cmd_part (struct session *sess, char *tbuf, char *word[],
							char *word_eol[]);
static int cmd_ping (struct session *sess, char *tbuf, char *word[],
							char *word_eol[]);
static int cmd_query (struct session *sess, char *tbuf, char *word[],
							 char *word_eol[]);
static int cmd_quit (struct session *sess, char *tbuf, char *word[],
							char *word_eol[]);
static int cmd_quote (struct session *sess, char *tbuf, char *word[],
							 char *word_eol[]);
static int cmd_reconnect (struct session *sess, char *tbuf, char *word[],
								  char *word_eol[]);
int cmd_rmdll (struct session *sess, char *tbuf, char *word[],
					char *word_eol[]);
static int cmd_say (struct session *sess, char *tbuf, char *word[],
						  char *word_eol[]);
int cmd_scpinfo (struct session *sess, char *tbuf, char *word[],
					  char *word_eol[]);
int cmd_set (struct session *sess, char *tbuf, char *word[],
				 char *word_eol[]);
static int cmd_settab (struct session *sess, char *tbuf, char *word[],
							  char *word_eol[]);
static int cmd_servchan (struct session *sess, char *tbuf, char *word[],
								 char *word_eol[]);
static int cmd_server (struct session *sess, char *tbuf, char *word[],
							  char *word_eol[]);
static int cmd_timer (struct session *sess, char *tbuf, char *word[],
							 char *word_eol[]);
static int cmd_topic (struct session *sess, char *tbuf, char *word[],
							 char *word_eol[]);
static int cmd_unignore (struct session *sess, char *tbuf, char *word[],
								 char *word_eol[]);
int cmd_unloadall (struct session *sess, char *tbuf, char *word[],
						 char *word_eol[]);
static int cmd_userlist (struct session *sess, char *tbuf, char *word[],
								 char *word_eol[]);
static int cmd_wallchop (struct session *sess, char *tbuf, char *word[],
								 char *word_eol[]);
static int cmd_wallchan (struct session *sess, char *tbuf, char *word[],
								 char *word_eol[]);
static int cmd_voice (struct session *sess, char *tbuf, char *word[],
							 char *word_eol[]);
static int cmd_flushq (struct session *sess, char *tbuf, char *word[],
							  char *word_eol[]);

struct commands xc_cmds[] = {

	{"ADDBUTTON", cmd_addbutton, 0, 0,
	 N_("/ADDBUTTON <name> <action>, adds a button under the user-list\n")},
	{"ALLCHAN", cmd_allchannels, 0, 0,
	 N_("/ALLCHAN <cmd>, sends a command to all channels you're in\n")},
	{"ALLSERV", cmd_allservers, 0, 0,
	 N_("/ALLSERV <cmd>, sends a command to all servers you're in\n")},
	{"AWAY", cmd_away, 1, 0, N_("/AWAY [<reason>], sets you away\n")},
	{"BAN", cmd_ban, 1, 1,
	 N_("/BAN <mask> [<bantype>], bans everyone matching the mask from the current channel. If they are already on the channel this doesn't kick them (needs chanop)\n")},
	{"UNBAN", cmd_unban, 1, 1,
	 N_("/UNBAN <mask> [<mask>...], unbans the specified masks.\n")},
	{"CLEAR", cmd_clear, 0, 0, N_("/CLEAR, Clears the current text window\n")},
	{"CLOSE", cmd_close, 0, 0, N_("/CLOSE, Closes the current window/tab\n")},

	{"CTCP", cmd_ctcp, 1, 0,
	 N_("/CTCP <nick> <message>, send the CTCP message to nick, common messages are VERSION and USERINFO\n")},
	{"COUNTRY", cmd_country, 0, 0,
	 N_("/COUNTRY <code>, finds a country code, eg: au = australia\n")},
	{"CYCLE", cmd_cycle, 1, 1,
	 N_("/CYCLE, parts current channel and immediately rejoins\n")},
	{"DCC", cmd_dcc, 0, 0,
	 N_("\n" "/DCC GET <nick>          - receive an offered file\n"
	 "/DCC SEND <nick> <file>  - send a file to someone\n"
	 "/DCC LIST                - show DCC list\n"
	 "/DCC CHAT <nick>         - offer DCC CHAT to someone\n"
	 "/DCC CLOSE <type> <nick> <file>         example:\n"
	 "         /dcc close send johnsmith file.tar.gz\n")},
	{"DEBUG", cmd_debug, 0, 0, 0},

	{"DELBUTTON", cmd_delbutton, 0, 0,
	 N_("/DELBUTTON <name>, deletes a button from under the user-list\n")},
	{"DEHOP", cmd_dehop, 1, 1,
	 N_("/DEHOP <nick>, removes chanhalf-op status from the nick on the current channel (needs chanop)\n")},
	{"DEOP", cmd_deop, 1, 1,
	 N_("/DEOP <nick>, removes chanop status from the nick on the current channel (needs chanop)\n")},
	{"DEVOICE", cmd_devoice, 1, 1,
	 N_("/DEVOICE <nick>, removes voice status from the nick on the current channel (needs chanop)\n")},
	{"DISCON", cmd_discon, 0, 0, N_("/DISCON, Disconnects from server\n")},
	{"DNS", cmd_dns, 0, 0, N_("/DNS <nick|host|ip>, Finds a users IP number\n")},
	{"ECHO", cmd_echo, 0, 0, N_("/ECHO <text>, Prints text locally\n")},
#ifndef WIN32
	{"EXEC", cmd_exec, 0, 0,
	 N_("/EXEC [-o] <command>, runs the command. If -o flag is used then output is sent to current channel, else is printed to current text box\n")},
	{"EXECKILL", cmd_execk, 0, 0,
	 N_("/EXECKILL [-9], kills a running exec in the current session. If -9 is given the process is SIGKILL'ed\n")},
#ifndef __EMX__
	{"EXECSTOP", cmd_execs, 0, 0, N_("/EXECSTOP, sends the process SIGSTOP\n")},
	{"EXECCONT", cmd_execc, 0, 0, N_("/EXECCONT, sends the process SIGCONT\n")},
	{"EXECWRITE", cmd_execw, 0, 0, N_("/EXECWRITE, sends data to the processes stdin\n")},
#endif
#endif
	{"FLUSHQ", cmd_flushq, 0, 0,
	 N_("/FLUSHQ, flushes the current server's send queue\n")},
	{"GATE", cmd_gate, 0, 0,
	 N_("/GATE <host> [<port>], proxies through a host, port defaults to 23\n")},
	{"HELP", cmd_help, 0, 0, 0},
	{"HOP", cmd_hop, 1, 1,
	 N_("/HOP <nick>, gives chanhalf-op status to the nick (needs chanop)\n")},
	{"IGNORE", cmd_ignore, 0, 0,
	 N_("/IGNORE <mask> <types..> <options..>\n"
	 "    mask - host mask to ignore, eg: *!*@*.aol.com\n"
	 "    types - types of data to ignore, one or all of:\n"
	 "            PRIV, CHAN, NOTI, CTCP, INVI, ALL\n"
	 "    options - NOSAVE, QUIET\n")},

	{"INVITE", cmd_invite, 1, 0,
	 N_("/INVITE <nick> [<channel>], invites someone to a channel, by default the current channel (needs chanop)\n")},
	{"JOIN", cmd_join, 1, 0, N_("/JOIN <channel>, joins the channel\n")},
	{"KICK", cmd_kick, 1, 1,
	 N_("/KICK <nick>, kicks the nick from the current channel (needs chanop)\n")},
	{"KICKBAN", cmd_kickban, 1, 1,
	 N_("/KICKBAN <nick>, bans then kicks the nick from the current channel (needs chanop)\n")},
	{"LAGCHECK", cmd_lagcheck, 0, 0,
	 N_("/LAGCHECK, forces a new lag check\n")},
	{"LASTLOG", cmd_lastlog, 0, 0,
	 N_("/LASTLOG <string>, searches for a string in the buffer.\n")},
	{"LIST", cmd_list, 1, 0, ""},
#ifdef USE_PLUGIN
	{"LISTDLL", module_list, 0, 0,
	 N_("/LISTDLL, Lists all currenly loaded plugins\n")},
#endif
	{"LOAD", cmd_load, 0, 0, N_("/LOAD <file>, loads a Perl script\n")},
#ifdef USE_PLUGIN
	{"LOADDLL", cmd_loaddll, 0, 0, N_("/LOADDLL <file>, loads a plugin\n")},
#endif

	{"MDEHOP", cmd_mdehop, 1, 1,
	 N_("/MDEHOP, Mass deop's all chanhalf-ops in the current channel (needs chanop)\n")},
	{"MDEOP", cmd_mdeop, 1, 1,
	 N_("/MDEOP, Mass deop's all chanops in the current channel (needs chanop)\n")},
	{"MKICK", cmd_mkick, 1, 1,
	 N_("/MKICK, Mass kicks everyone except you in the current channel (needs chanop)\n")},
	{"MKICKB", cmd_mkickb, 1, 1,
	 N_("/MKICKB, Sets a ban of *@* and mass kicks everyone except you in the current channel (needs chanop)\n")},
	{"ME", cmd_me, 0, 0,
	 N_("/ME <action>, sends the action to the current channel (actions are written in the 3rd person, like /me jumps)\n")},
	{"MSG", cmd_msg, 0, 0, N_("/MSG <nick> <message>, sends a private message\n")},

	{"NAMES", cmd_names, 1, 0,
	 N_("/NAMES, Lists the nicks on the current channel\n")},
	{"NCTCP", cmd_nctcp, 1, 0,
	 N_("/NCTCP <nick> <message>, Sends a CTCP notice\n")},
	{"NEWSERVER", cmd_newserver, 0, 0, N_("/NEWSERVER <hostname> [<port>]\n")},
	{"NICK", cmd_nick, 0, 0, N_("/NICK <nickname>, sets your nick\n")},

	{"NOTICE", cmd_notice, 1, 0,
	 N_("/NOTICE <nick/channel> <message>, sends a notice. Notices are a type of message that should be auto reacted to\n")},
	{"NOTIFY", cmd_notify, 0, 0,
	 N_("/NOTIFY [<nick>], lists your notify list or adds someone to it\n")},
	{"OP", cmd_op, 1, 1,
	 N_("/OP <nick>, gives chanop status to the nick (needs chanop)\n")},
	{"PART", cmd_part, 1, 1,
	 N_("/PART [<channel>] [<reason>], leaves the channel, by default the current one\n")},
	{"PING", cmd_ping, 1, 0,
	 N_("/PING <nick | channel>, CTCP pings nick or channel\n")},
#ifdef USE_PYTHON

	{"PKILL", pys_pkill, 0, 0,
	 N_("/PKILL <name>, kills the script of the given name\n")},
	{"PLIST", pys_plist, 0, 0, N_("/PLIST, lists the current python scripts\n")},
	{"PLOAD", pys_load, 0, 0, N_("/PLOAD loads a python script\n")},
#endif

	{"QUERY", cmd_query, 0, 0,
	 N_("/QUERY <nick>, opens up a new privmsg window to someone\n")},
	{"QUIT", cmd_quit, 0, 0,
	 N_("/QUIT [<reason>], disconnects from the current server\n")},
	{"QUOTE", cmd_quote, 1, 0,
	 N_("/QUOTE <text>, sends the text in raw form to the server\n")},
#ifdef USE_OPENSSL
	{"RECONNECT", cmd_reconnect, 0, 0,
	 N_("/RECONNECT [-ssl] [<host>] [<port>] [<password>], Can be called just as /RECONNECT to reconnect to the current server or with /RECONNECT ALL to reconnect to all the open servers\n")}, 
#else 
        {"RECONNECT", cmd_reconnect, 0, 0,
	N_("/RECONNECT [<host>] [<port>] [<password>], Can be called just as /RECONNECT to reconnect to the current server or with /RECONNECT ALL to reconnect to all the open servers\n")},
#endif
#ifdef USE_PLUGIN
	{"RMDLL", cmd_rmdll, 0, 0, N_("/RMDLL <dll name>, unloads a plugin\n")},
#endif

	{"SAY", cmd_say, 0, 0,
	 N_("/SAY <text>, sends the text to the object in the current window\n")},
#ifdef USE_PERL
	{"SCPINFO", cmd_scpinfo, 0, 0,
	 N_("/SCPINFO, Lists some information about current Perl bindings\n")},
#endif
	{"SET", cmd_set, 0, 0, N_("/SET <variable> [<value>]\n")},
	{"SETTAB", cmd_settab, 0, 0, ""},
#ifdef USE_OPENSSL
	{"SERVCHAN", cmd_servchan, 0, 0,
	 N_("/SERVCHAN [-ssl] <host> <port> <channel>, connects and joins a channel\n")},
#else
	{"SERVCHAN", cmd_servchan, 0, 0,
	 N_("/SERVCHAN <host> <port> <channel>, connects and joins a channel\n")},
#endif
#ifdef USE_OPENSSL
	{"SERVER", cmd_server, 0, 0,
	 N_("/SERVER [-ssl] <host> [<port>] [<password>], connects to a server, the default port is 6667 for normal connections, and 994 for ssl connections.\n")},
#else
	{"SERVER", cmd_server, 0, 0,
	 N_("/SERVER <host> [<port>] [<password>], connects to a server, the default port is 6667.\n")},
#endif
	{"TIMER", cmd_timer, 0, 0, N_("/TIMER <seconds> <command>\n")},
	{"TOPIC", cmd_topic, 1, 1,
	 N_("/TOPIC [<topic>], sets the topic if one is given, else shows the current topic\n")},
	{"UNIGNORE", cmd_unignore, 0, 0, N_("/UNIGNORE <mask> [QUIET]\n")},
#ifdef USE_PERL
	{"UNLOADALL", cmd_unloadall, 0, 0, N_("/UNLOADALL, Unloads all perl scripts\n")},
#endif
	{"USERLIST", cmd_userlist, 1, 1, 0},
	{"WALLCHOP", cmd_wallchop, 1, 1,
	 N_("/WALLCHOP <message>, sends the message to all chanops on the current channel\n")},
	{"WALLCHAN", cmd_wallchan, 1, 1,
	 N_("/WALLCHAN <message>, writes the message to all channels\n")},
	{"VOICE", cmd_voice, 1, 1,
	 N_("/VOICE <nick>, gives voice status to someone (needs chanop)\n")},
	{0, 0, 0, 0, 0}
};

static void
help (struct session *sess, char *helpcmd, int quiet)
{
	int i = 0;
	while (1)
	{
		if (!xc_cmds[i].name)
			break;
		if (!strcasecmp (helpcmd, xc_cmds[i].name))
		{
			if (xc_cmds[i].help)
			{
				PrintText (sess, _("Usage:\n"));
                if (*xc_cmds[i].help != '\0') 
                    PrintText (sess, _(xc_cmds[i].help));
				return;
			} else
			{
				if (!quiet)
					PrintText (sess, _("\nNo help available on that command.\n"));
				return;
			}
		}
		i++;
	}
	if (!quiet)
		PrintText (sess, _("No such command.\n"));
}

static void
send_channel_modes3 (struct session *sess, char *tbuf,
							char *word[], int start, int end, char sign, char mode)
{
	int left;
	int i = start;

	while (1)
	{
		left = end - i;
		switch (left)
		{
		case 0:
			return;
		case 1:
			sprintf (tbuf, "MODE %s %c%c %s\r\n", sess->channel, sign, mode,
						word[i]);
			break;
		case 2:
			sprintf (tbuf, "MODE %s %c%c%c %s %s\r\n", sess->channel, sign, mode,
						mode, word[i], word[i + 1]);
			i++;
			break;
		default:
			sprintf (tbuf, "MODE %s %c%c%c%c %s %s %s\r\n", sess->channel, sign,
						mode, mode, mode, word[i], word[i + 1], word[i + 2]);
			i += 2;
			break;
		}
		tcp_send (sess->server, tbuf);
		if (left < 3)
			return;
		i++;
	}
}

static void
send_channel_modes6 (struct session *sess, char *tbuf,
							char *word[], int start, int end, char sign, char mode)
{
	int left;
	int i = start;

	while (1)
	{
		left = end - i;
		switch (left)
		{
		case 0:
			return;
		case 1:
			sprintf (tbuf, "MODE %s %c%c %s\r\n", sess->channel, sign, mode,
						word[i]);
			i += 1;
			break;
		case 2:
			sprintf (tbuf, "MODE %s %c%c%c %s %s\r\n", sess->channel, sign, mode,
						mode, word[i], word[i + 1]);
			i += 2;
			break;
		case 3:
			sprintf (tbuf, "MODE %s %c%c%c%c %s %s %s\r\n", sess->channel, sign,
						mode, mode, mode, word[i], word[i + 1], word[i + 2]);
			i += 3;
			break;
		case 4:
			sprintf (tbuf, "MODE %s %c%c%c%c%c %s %s %s %s\r\n", sess->channel,
						sign, mode, mode, mode, mode,
						word[i], word[i + 1], word[i + 2], word[i + 3]);
			i += 4;
			break;
		case 5:
			sprintf (tbuf, "MODE %s %c%c%c%c%c%c %s %s %s %s %s\r\n",
						sess->channel, sign, mode, mode, mode, mode, mode, word[i],
						word[i + 1], word[i + 2], word[i + 3], word[i + 4]);
			i += 5;
			break;
		default:
			sprintf (tbuf, "MODE %s %c%c%c%c%c%c%c %s %s %s %s %s %s\r\n",
						sess->channel, sign, mode, mode, mode, mode, mode, mode,
						word[i], word[i + 1], word[i + 2], word[i + 3], word[i + 4],
						word[i + 5]);
			i += 6;
			break;
		}
		tcp_send (sess->server, tbuf);
		if (left < 6)
			return;
	}
}

void
send_channel_modes (struct session *sess, char *tbuf,
						  char *word[], int start, int end, char sign, char mode)
{
	if (sess->server->six_modes)
		send_channel_modes6 (sess, tbuf, word, start, end, sign, mode);
	else
		send_channel_modes3 (sess, tbuf, word, start, end, sign, mode);
}

int
cmd_addbutton (struct session *sess, char *tbuf, char *word[],
					char *word_eol[])
{
	if (*word[2] && *word_eol[3])
	{
		if (sess->type == SESS_DIALOG)
		{
			list_addentry (&dlgbutton_list, word_eol[3], word[2]);
			fe_dlgbuttons_update (sess);
		} else
		{
			list_addentry (&button_list, word_eol[3], word[2]);
			fe_buttons_update (sess);
		}
		return TRUE;
	}
	return FALSE;
}

int
cmd_allchannels (struct session *sess, char *tbuf, char *word[],
					  char *word_eol[])
{
	GSList *list = sess_list;

	if (!*word_eol[2])
		return FALSE;

	while (list)
	{
		sess = list->data;
		if (sess->type == SESS_CHANNEL && sess->channel[0] && sess->server->connected)
		{
			handle_command (word_eol[2], sess, TRUE, FALSE);
		}
		list = list->next;
	}

	return TRUE;
}

int
cmd_allservers (struct session *sess, char *tbuf, char *word[],
					 char *word_eol[])
{
	GSList *list;
	server *serv;

	if (!*word_eol[2])
		return FALSE;

	list = serv_list;
	while (list)
	{
		serv = list->data;
		if (serv->connected)
			handle_command (word_eol[2], serv->front_session, TRUE, FALSE);
		list = list->next;
	}

	return TRUE;
}

int
cmd_away (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	GSList *list;
	char *reason = word_eol[2];
	int back = FALSE;
	unsigned int gone;

	if (!(*reason) && sess->server->is_away)
	{
		/* mark back */
		tcp_send_len (sess->server, "AWAY\r\n", 6);
		back = TRUE;
	} else
	{
		if (!(*reason))
		{
			if (sess->server->reconnect_away)
				reason = sess->server->last_away_reason;
			else
				/* must manage memory pointed to by random_line() */
				reason = random_line (prefs.awayreason);
		}
		sprintf (tbuf, "AWAY :%s\r\n", reason);
		tcp_send (sess->server, tbuf);
	}

	if (prefs.show_away_message)
	{
		if (back)
		{
			gone = time (NULL) - sess->server->away_time;
			sprintf (tbuf, "/me is back (gone %.2d:%.2d:%.2d)", gone / 3600,
						(gone / 60) % 60, gone % 60);
		} else
		{
			sprintf (tbuf, "/me is away: %s", reason);
		}

		list = sess_list;
		while (list)
		{
			/* am I the right server and not a dialog box */
			if (((struct session *) list->data)->server == sess->server
				 && ((struct session *) list->data)->type == SESS_CHANNEL
				 && ((struct session *) list->data)->channel[0])
			{
				handle_command (tbuf, (struct session *) list->data,
									 FALSE, FALSE);
			}
			list = list->next;
		}
	}

	if (sess->server->last_away_reason != reason)
	{
		free (sess->server->last_away_reason);
		if (reason == word_eol[2])
			sess->server->last_away_reason = strdup (reason);
		else
			sess->server->last_away_reason = reason;
	}

	return TRUE;
}

static void
ban (session * sess, char *tbuf, char *mask, char *bantypestr, int deop)
{
	int bantype;
	struct User *user;
	char *at, *dot, *lastdot;
	char username[64], fullhost[128], domain[128], *mode, *p2;

	user = find_name (sess, mask);
	if (user && user->hostname)  /* it's a nickname, let's find a proper ban mask */
	{
		if (deop)
		{
			mode = "-o+b";
			p2 = user->nick;
		} else
		{
			mode = "+b";
			p2 = "";
		}

		mask = user->hostname;

		at = strchr (mask, '@');
		if (!at)
			return;					  /* can't happen? */
		*at = 0;

		if (mask[0] == '~' ||
		    mask[0] == '+' ||
		    mask[0] == '=' ||
		    mask[0] == '^' ||
		    mask[0] == '-')
			strcpy (username, mask+1);
		else
			strcpy (username, mask);
		*at = '@';
		strcpy (fullhost, at + 1);

		dot = strchr (fullhost, '.');
		if (dot)
			strcpy (domain, dot);
		else
			strcpy (domain, fullhost);

		if (*bantypestr)
			bantype = atoi (bantypestr);
		else
			bantype = prefs.bantype;

		tbuf[0] = 0;
		if (inet_addr (fullhost) != -1)	/* "fullhost" is really a IP number */
		{
			lastdot = strrchr (fullhost, '.');
			if (!lastdot)
				return;				  /* can't happen? */

			*lastdot = 0;
			strcpy (domain, fullhost);
			*lastdot = '.';

			switch (bantype)
			{
			case 0:
				sprintf (tbuf, "MODE %s %s %s *!*@%s.*\r\n", sess->channel, mode, p2, domain);
				break;

			case 1:
				sprintf (tbuf, "MODE %s %s %s *!*@%s\r\n", sess->channel, mode, p2, fullhost);
				break;

			case 2:
				sprintf (tbuf, "MODE %s %s %s *!*%s@%s.*\r\n", sess->channel, mode, p2, 
							username, domain);
				break;

			case 3:
				sprintf (tbuf, "MODE %s %s %s *!*%s@%s\r\n", sess->channel, mode, p2, username,
							fullhost);
				break;
			}
		} else
		{
			switch (bantype)
			{
			case 0:
				sprintf (tbuf, "MODE %s %s %s *!*@*%s\r\n", sess->channel, mode, p2, domain);
				break;

			case 1:
				sprintf (tbuf, "MODE %s %s %s *!*@%s\r\n", sess->channel, mode, p2, fullhost);
				break;

			case 2:
				sprintf (tbuf, "MODE %s %s %s *!*%s@*%s\r\n", sess->channel, mode, p2, username,
							domain);
				break;

			case 3:
				sprintf (tbuf, "MODE %s %s %s *!*%s@%s\r\n", sess->channel, mode, p2, username,
							fullhost);
				break;
			}
		}

	} else
	{
		sprintf (tbuf, "MODE %s +b %s\r\n", sess->channel, mask);
	}
	tcp_send (sess->server, tbuf);
}

int
cmd_ban (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	char *mask = word[2];

	if (*mask)
	{
		ban (sess, tbuf, mask, word[3], 0);
	} else
	{
		sprintf (tbuf, "MODE %s +b\r\n", sess->channel);	/* banlist */
		tcp_send (sess->server, tbuf);
	}

	return TRUE;
}

int
cmd_unban (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	/* Allow more than one mask in /unban -- tvk */
	int i = 2;

	while (1)
	{
		if (!*word[i])
		{
			if (i == 2)
				return FALSE;
			send_channel_modes (sess, tbuf, word, 2, i, '-', 'b');
			return TRUE;
		}
		i++;
	}
}

int
cmd_clear (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	GSList *list = sess_list;
	char *reason = word_eol[2];

	if (strncasecmp (reason, "all", 3) == 0)
	{
		while (list)
		{
			sess = list->data;
			if (sess->type != SESS_SHELL && !sess->nick_said)
				fe_text_clear (list->data);
			list = list->next;
		}
	} else
	{
		fe_text_clear (sess);
	}

	return TRUE;
}

int
cmd_close (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	GSList *list;

	if (strcmp (word[2], "-m") == 0)
	{
		list = sess_list;
		while (list)
		{
			sess = list->data;
			list = list->next;
			if (sess->type == SESS_DIALOG)
				fe_close_window (sess);
		}
	} else
	{
		if (*word_eol[2])
			sess->quitreason = word_eol[2];
		fe_close_window (sess);
	}

	return TRUE;
}

int
cmd_ctcp (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	char *to = word[2];
	if (*to)
	{
		char *msg = word_eol[3];
		if (*msg)
		{
			char *cmd = msg;

			while (1)
			{
				if (*cmd == ' ' || *cmd == 0)
					break;
				*cmd = toupper (*cmd);
				cmd++;
			}

			sprintf (tbuf, "PRIVMSG %s :\001%s\001\r\n", to, msg);
			tcp_send (sess->server, tbuf);

			EMIT_SIGNAL (XP_TE_CTCPSEND, sess, to, msg, NULL, NULL, 0);

			return TRUE;
		}
	}
	return FALSE;
}

int
cmd_country (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	char *code = word[2];
	if (*code)
	{
		sprintf (tbuf, "%s = %s\n", code, country (code));
		PrintText (sess, tbuf);
		return TRUE;
	}
	return FALSE;
}

int
cmd_cycle (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	char *key = sess->channelkey;
	char *chan = sess->channel;
	if (*chan && sess->type == SESS_CHANNEL)
	{
		sprintf (tbuf, "PART %s\r\nJOIN %s %s\r\n", chan, chan, key);
		tcp_send (sess->server, tbuf);
		return TRUE;
	}
	return FALSE;
}

int
cmd_dcc (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	int goodtype;
	struct DCC *dcc = 0;
	char *type = word[2];
	if (*type)
	{
		if (!strcasecmp (type, "HELP"))
			return FALSE;
		if (!strcasecmp (type, "CLOSE"))
		{
			if (*word[3] && *word[4])
			{
				goodtype = 0;
				if (!strcasecmp (word[3], "SEND"))
				{
					dcc = find_dcc (word[4], word[5], TYPE_SEND);
					if (dcc)
						dcc_close (dcc, 0, TRUE);
					goodtype = TRUE;
				}
				if (!strcasecmp (word[3], "GET"))
				{
					dcc = find_dcc (word[4], word[5], TYPE_RECV);
					if (dcc)
						dcc_close (dcc, 0, TRUE);
					goodtype = TRUE;
				}
				if (!strcasecmp (word[3], "CHAT"))
				{
					dcc = find_dcc (word[4], "", TYPE_CHATRECV);
					if (!dcc)
						dcc = find_dcc (word[4], "", TYPE_CHATSEND);
					if (dcc)
						dcc_close (dcc, 0, TRUE);
					goodtype = TRUE;
				}

				if (!goodtype)
					return FALSE;

				if (!dcc)
					EMIT_SIGNAL (XP_TE_NODCC, sess, NULL, NULL, NULL, NULL, 0);
				return TRUE;

			}
			return FALSE;
		}
		if (!strcasecmp (type, "CHAT"))
		{
			char *nick = word[3];
			if (*nick)
				dcc_chat (sess, nick);
			return TRUE;
		}
		if (!strcasecmp (type, "LIST"))
		{
			dcc_show_list (sess, tbuf);
			return TRUE;
		}
		if (!strcasecmp (type, "GET"))
		{
			char *nick = word[3];
			char *file = word[4];
			if (!*file)
			{
				if (*nick)
					dcc_get_nick (sess, nick);
			} else
			{
				dcc = find_dcc (nick, file, TYPE_RECV);
				if (dcc)
					dcc_get (dcc);
				else
					EMIT_SIGNAL (XP_TE_NODCC, sess, NULL, NULL, NULL, NULL, 0);
			}
			return TRUE;
		}
		if (!strcasecmp (type, "SEND"))
		{
			char *nick = word[3];
			if (*nick)
			{
				int i = 4;
				char *file;
				while (1)
				{
					file = word[i];
					if (!*file && i == 4)
					{
						fe_dcc_send_filereq (sess, nick);
						return TRUE;
					}
					if (!*file)
						break;
					dcc_send (sess, tbuf, nick, file);
					i++;
				}
			}
			return TRUE;
		}
	} else
		dcc_show_list (sess, tbuf);
	return TRUE;
}

static int
cmd_debug (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	struct session *s;
	struct server *v;
	GSList *list = sess_list;

	PrintText (sess, "Session   T Channel    WaitChan  WillChan  Server\n");
	while (list)
	{
		s = (struct session *) list->data;
		sprintf (tbuf, "0x%lx %1x %-10.10s %-10.10s %-10.10s 0x%lx\n",
					(unsigned long) s, s->type, s->channel, s->waitchannel,
					s->willjoinchannel, (unsigned long) s->server);
		PrintText (sess, tbuf);
		list = list->next;
	}

	list = serv_list;
	PrintText (sess, "Server    Sock  Name\n");
	while (list)
	{
		v = (struct server *) list->data;
		sprintf (tbuf, "0x%lx %-5ld %s\n",
					(unsigned long) v, (unsigned long) v->sok, v->servername);
		PrintText (sess, tbuf);
		list = list->next;
	}

	sprintf (tbuf,
				"\nfront_session: %lx\n"
				"current_tab: %lx\n\n",
				(unsigned long) sess->server->front_session,
				(unsigned long) current_tab);
	PrintText (sess, tbuf);
#ifdef MEMORY_DEBUG
	sprintf (tbuf, "current mem: %d\n\n", current_mem_usage);
	PrintText (sess, tbuf);
#endif  /* !MEMORY_DEBUG */

	return TRUE;
}

int
cmd_delbutton (struct session *sess, char *tbuf, char *word[],
					char *word_eol[])
{
	if (*word[2])
	{
		if (sess->type == SESS_DIALOG)
		{
			if (list_delentry (&dlgbutton_list, word[2]))
				fe_dlgbuttons_update (sess);
		} else
		{
			if (list_delentry (&button_list, word[2]))
				fe_buttons_update (sess);
		}
		return TRUE;
	}
	return FALSE;
}

int
cmd_dehop (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	int i = 2;

	while (1)
	{
		if (!*word[i])
		{
			if (i == 2)
				return FALSE;
			send_channel_modes (sess, tbuf, word, 2, i, '-', 'h');
			return TRUE;
		}
		i++;
	}
}

int
cmd_deop (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	int i = 2;

	while (1)
	{
		if (!*word[i])
		{
			if (i == 2)
				return FALSE;
			send_channel_modes (sess, tbuf, word, 2, i, '-', 'o');
			return TRUE;
		}
		i++;
	}
}

int
cmd_mdehop (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	struct User *user;
	char **nicks = malloc (sizeof (char *) * sess->hops);
	int i = 0;
	GSList *list = sess->userlist;

	while (list)
	{
		user = (struct User *) list->data;
		if (user->hop && (strcmp (user->nick, sess->server->nick) != 0))
		{
			nicks[i] = user->nick;
			i++;
		}
		list = list->next;
	}

	send_channel_modes (sess, tbuf, nicks, 0, i, '-', 'h');

	free (nicks);

	return TRUE;
}

int
cmd_mdeop (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	struct User *user;
	char **nicks = malloc (sizeof (char *) * sess->ops);
	int i = 0;
	GSList *list = sess->userlist;

	while (list)
	{
		user = (struct User *) list->data;
		if (user->op && (strcmp (user->nick, sess->server->nick) != 0))
		{
			nicks[i] = user->nick;
			i++;
		}
		list = list->next;
	}

	send_channel_modes (sess, tbuf, nicks, 0, i, '-', 'o');

	free (nicks);

	return TRUE;
}

int
cmd_mkick (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	struct User *user;
	char *reason = word_eol[2];
	GSList *list;

	list = sess->userlist;
	while (list)
	{
		user = (struct User *) list->data;
		if (user->op)
		{
			if (strcmp (user->nick, sess->server->nick) != 0)
			{
				if (*reason)
					sprintf (tbuf, "KICK %s %s :%s\r\n", sess->channel, user->nick,
								reason);
				else
					sprintf (tbuf, "KICK %s %s\r\n", sess->channel, user->nick);
				tcp_send (sess->server, tbuf);
			}
		}
		list = list->next;
	}

	list = sess->userlist;
	while (list)
	{
		user = (struct User *) list->data;
		if (!user->op)
		{
			if (strcmp (user->nick, sess->server->nick) != 0)
			{
				if (*reason)
					sprintf (tbuf, "KICK %s %s :%s\r\n", sess->channel, user->nick,
								reason);
				else
					sprintf (tbuf, "KICK %s %s\r\n", sess->channel, user->nick);
				tcp_send (sess->server, tbuf);
			}
		}
		list = list->next;
	}

	return TRUE;
}

int
cmd_mkickb (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	sprintf (tbuf, "MODE %s +b *@*\r\n", sess->channel);
	tcp_send (sess->server, tbuf);

	return cmd_mkick (sess, tbuf, word, word_eol);
}

int
cmd_devoice (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	int i = 2;

	while (1)
	{
		if (!*word[i])
		{
			if (i == 2)
				return FALSE;
			send_channel_modes (sess, tbuf, word, 2, i, '-', 'v');
			return TRUE;
		}
		i++;
	}
}

int
cmd_discon (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	disconnect_server (sess, TRUE, -1);
	return TRUE;
}

int
cmd_dns (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	char *nick = word[2];
	struct User *user;

	if (*nick)
	{
		if (strchr (nick, '.') == NULL)
		{
			user = find_name (sess, nick);
			if (user && user->hostname)
			{
				do_dns (sess, tbuf, user->nick, user->hostname);
			} else
			{
				sprintf (tbuf, "WHO %s\r\n", nick);
				tcp_send (sess->server, tbuf);
				sess->server->doing_who = TRUE;
			}
		} else
		{
			sprintf (tbuf, "/exec %s %s", prefs.dnsprogram, nick);
			handle_command (tbuf, sess, 0, 0);
		}
		return TRUE;
	}
	return FALSE;
}

int
cmd_echo (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	PrintText (sess, word_eol[2]);
	return TRUE;
}

#ifndef WIN32

static void
exec_check_process (struct session *sess)
{
	int val;

	if (sess->running_exec == NULL)
		return;
	val = waitpid (sess->running_exec->childpid, NULL, WNOHANG);
	if (val == -1 || val > 0)
	{
		close (sess->running_exec->myfd);
		fe_input_remove (sess->running_exec->iotag);
		free (sess->running_exec);
		sess->running_exec = NULL;
	}
}

#ifndef __EMX__
int
cmd_execs (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	int r;

	exec_check_process (sess);
	if (sess->running_exec == NULL)
	{
		EMIT_SIGNAL (XP_TE_NOCHILD, sess, NULL, NULL, NULL, NULL, 0);
		return FALSE;
	}
	r = kill (sess->running_exec->childpid, SIGSTOP);
	if (r == -1)
		PrintText (sess, "Error in kill(2)\n");

	return TRUE;
}

int
cmd_execc (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	int r;

	exec_check_process (sess);
	if (sess->running_exec == NULL)
	{
		EMIT_SIGNAL (XP_TE_NOCHILD, sess, NULL, NULL, NULL, NULL, 0);
		return FALSE;
	}
	r = kill (sess->running_exec->childpid, SIGCONT);
	if (r == -1)
		PrintText (sess, "Error in kill(2)\n");

	return TRUE;
}

int
cmd_execk (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	int r;

	exec_check_process (sess);
	if (sess->running_exec == NULL)
	{
		EMIT_SIGNAL (XP_TE_NOCHILD, sess, NULL, NULL, NULL, NULL, 0);
		return FALSE;
	}
	if (strcmp (word[2], "-9") == 0)
		r = kill (sess->running_exec->childpid, SIGKILL);
	else
		r = kill (sess->running_exec->childpid, SIGTERM);
	if (r == -1)
		PrintText (sess, "Error in kill(2)\n");

	return TRUE;
}

/* OS/2 Can't have the /EXECW command because it uses pipe(2) not socketpair
   and thus it is simplex --AGL */
int
cmd_execw (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	int len;
	char *temp;
	exec_check_process (sess);
	if (sess->running_exec == NULL)
	{
		EMIT_SIGNAL (XP_TE_NOCHILD, sess, NULL, NULL, NULL, NULL, 0);
		return FALSE;
	}
	len = strlen(word_eol[2]);
	temp = malloc(len + 2);
	sprintf(temp, "%s\n", word_eol[2]);
	PrintText(sess, temp);
	write(sess->running_exec->myfd, temp, len + 1);
	free(temp);

	return TRUE;
}
#endif /* !__EMX__ */

/* convert ANSI escape color codes to mIRC codes */

static short escconv[] =
/* 0 1 2 3 4 5  6 7  0 1 2 3 4  5  6  7 */
{  1,4,3,5,2,10,6,1, 1,7,9,8,12,11,13,1 };

static void
exec_handle_colors (char *buf, int len)
{
	char numb[16];
	char *nbuf;
	int i = 0, j = 0, k = 0, firstn = 0, col, colf = 0, colb = 0;
	int esc = FALSE, backc = FALSE, bold = FALSE;

	/* any escape codes in this text? */
	if (strchr (buf, 27) == 0)
		return;

	nbuf = malloc (len + 1);

	while (i < len)
	{
		switch (buf[i])
		{
		case '\r':
			break;
		case 27:
			esc = TRUE;
			break;
		case ';':
			if (!esc)
				goto norm;
			backc = TRUE;
			numb[k] = 0;
			firstn = atoi (numb);
			k = 0;
			break;
		case '[':
			if (!esc)
				goto norm;
			break;
		default:
			if (esc)
			{
				if (buf[i] >= 'A' && buf[i] <= 'z')
				{
					if (buf[i] == 'm')
					{
						/* ^[[0m */
						if (k == 0 || (numb[0] == '0' && k == 1))
						{
							nbuf[j] = '\017';
							j++;
							bold = FALSE;
							goto cont;
						}

						numb[k] = 0;
						col = atoi (numb);
						backc = FALSE;

						if (firstn == 1)
							bold = TRUE;

						if (firstn >= 30 && firstn <= 37)
							colf = firstn - 30;

						if (col >= 40)
						{
							colb = col - 40;
							backc = TRUE;
						}

						if (col >= 30 && col <= 37)
							colf = col - 30;

						if (bold)
							colf += 8;

						if (backc)
						{
							colb = escconv[colb % 14];
							colf = escconv[colf % 14];
							j += sprintf (&nbuf[j], "\003%d,%02d", colf, colb);
						} else
						{
							colf = escconv[colf % 14];
							j += sprintf (&nbuf[j], "\003%02d", colf);
						}
					}
cont:				esc = FALSE;
					backc = FALSE;
					k = 0;
				} else
				{
					if (isdigit (buf[i]) && k < (sizeof (numb) - 1))
					{
						numb[k] = buf[i];
						k++;
					}
				}
			} else
			{
norm:			nbuf[j] = buf[i];
				j++;
			}
		}
		i++;
	}

	nbuf[j] = 0;
	memcpy (buf, nbuf, j + 1);
	free (nbuf);
}

static gboolean
exec_data (GIOChannel *source, GIOCondition condition, struct nbexec *s)
{
	char *buf, *readpos, *rest;
	int rd, len;
	int sok = s->myfd;

	len = s->buffill;
	if (len) {
		/* append new data to buffered incomplete line */
		buf = malloc(len + 2050);
		memcpy(buf, s->linebuf, len);
		readpos = buf + len;
		free(s->linebuf);
		s->linebuf = NULL;
	}
	else
		readpos = buf = malloc(2050);
	
	rd = read (sok, readpos, 2048);
	if (rd < 1)
	{
		/* The process has died */
		kill(s->childpid, SIGKILL);
		if (len) {
			buf[len] = '\0';
			exec_handle_colors(buf, len);
			if (s->tochannel)
				handle_multiline (s->sess, buf, FALSE, TRUE);
			else
				PrintText (s->sess, buf);
		}
		free(buf);
		waitpid (s->childpid, NULL, 0);
		s->sess->running_exec = NULL;
		fe_input_remove (s->iotag);
		close (sok);
		free (s);
		return TRUE;
	}
	len += rd;
	buf[len] = '\0';
	
	rest = strrchr(buf, '\n');
	if (rest)
		rest++;
	else
		rest = buf;
	if (*rest) {
		s->buffill = len - (rest - buf); /* = strlen(rest) */
		s->linebuf = malloc(s->buffill);
		memcpy(s->linebuf, rest, s->buffill);
		*rest = '\0';
		len -= s->buffill; /* possibly 0 */
	}
	else
		s->buffill = 0;

	if (len) {
		exec_handle_colors (buf, len);
		if (s->tochannel)
			handle_multiline (s->sess, buf, FALSE, TRUE);
		else
			PrintText (s->sess, buf);
	}
	
	free(buf);
	return TRUE;
}

int
cmd_exec (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	int tochannel = FALSE;
	char *cmd = word_eol[2];
	int fds[2], pid = 0;
	struct nbexec *s;

	if (*cmd)
	{
		if (access ("/bin/sh", X_OK) != 0)
		{
			fe_message (_("I need /bin/sh to run!\n"), FALSE);
			return TRUE;
		}
		exec_check_process (sess);
		if (sess->running_exec != NULL)
		{
			EMIT_SIGNAL (XP_TE_ALREADYPROCESS, sess, NULL, NULL, NULL, NULL, 0);
			return TRUE;
		}
		if (!strcmp (word[2], "-o"))
		{
			if (!*word[3])
				return FALSE;
			else
			{
				cmd = word_eol[3];
				tochannel = TRUE;
			}
		}
#ifdef __EMX__						  /* if os/2 */
		if (pipe (fds) < 0)
		{
			PrintText (sess, "Pipe create error\n");
			return FALSE;
		}
		setmode (fds[0], O_BINARY);
		setmode (fds[1], O_BINARY);
#else
		if (socketpair (PF_UNIX, SOCK_STREAM, 0, fds) == -1)
		{
			PrintText (sess, "socketpair(2) failed\n");
			return FALSE;
		}
#endif
		s = (struct nbexec *) malloc (sizeof (struct nbexec));
		memset(s, 0, sizeof(*s));
		s->myfd = fds[0];
		s->tochannel = tochannel;
		s->sess = sess;

		pid = fork ();
		if (pid == 0)
		{
			/* This is the child's context */
			close (0);
			close (1);
			close (2);
			/* Close parent's end of pipe */
			close(s->myfd);
			/* Copy the child end of the pipe to stdout and stderr */
			dup2 (fds[1], 1);
			dup2 (fds[1], 2);
			/* Also copy it to stdin so we can write to it */
			dup2 (fds[1], 0);
			/* Now we call /bin/sh to run our cmd ; made it more friendly -DC1 */
			execl ("/bin/sh", "sh", "-c", cmd, 0);
			/* not reached unless error */
			/*printf("exec error\n");*/
			fflush (stdout);
			fflush (stdin);
			_exit (0);
		}
		if (pid == -1)
		{
			/* Parent context, fork() failed */

			PrintText (sess, "Error in fork(2)\n");
			close(fds[0]);
			close(fds[1]);
		} else
		{
			/* Parent path */
			close(fds[1]);
			s->childpid = pid;
			s->iotag = fe_input_add (s->myfd, 1, 0, 1, exec_data, s);
			sess->running_exec = s;
			return TRUE;
		}
	}
	return FALSE;
}

#endif

int
cmd_quit (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	if (*word_eol[2])
		sess->quitreason = word_eol[2];
	disconnect_server (sess, TRUE, -1);
	sess->quitreason = NULL;
	return 2;
}

int
cmd_gate (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	char *server = word[2];
	if (*server)
	{
		char *port = word[3];
#ifdef USE_OPENSSL
		sess->server->use_ssl = FALSE;
#endif
		if (*port)
			connect_server (sess, server, atoi (port), TRUE);
		else
			connect_server (sess, server, 23, TRUE);
		return TRUE;
	}
	return FALSE;
}

static int
cmd_help (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	int i = 0, longfmt = 0;
	char *helpcmd = "";

	if (tbuf)
		helpcmd = word[2];
	if (*helpcmd && strcmp (helpcmd, "-l") == 0)
		longfmt = 1;

	if (*helpcmd && !longfmt)
	{
		help (sess, helpcmd, FALSE);
	} else
	{
		struct popup *pop;
		GSList *list = command_list;
		char *buf = malloc (4096);
		int t = 1, j;
		strcpy (buf, _("\nCommands Available:\n\n  "));
		if (longfmt)
		{
			while (1)
			{
				if (!xc_cmds[i].name)
					break;
				if (!xc_cmds[i].help || *xc_cmds[i].help == '\0')
					snprintf (buf, 4096, "   \0034%s\003 :\n", xc_cmds[i].name);
				else
					snprintf (buf, 4096, "   \0034%s\003 : %s", xc_cmds[i].name,
								 _(xc_cmds[i].help));
				PrintText (sess, buf);
				i++;
			}
			buf[0] = 0;
		} else
		{
			while (1)
			{
				if (!xc_cmds[i].name)
					break;
				strcat (buf, xc_cmds[i].name);
				t++;
				if (t == 6)
				{
					t = 1;
					strcat (buf, "\n  ");
				} else
					for (j = 0; j < (10 - strlen (xc_cmds[i].name)); j++)
						strcat (buf, " ");
				i++;
			}
		}
		strcat (buf,
				  _("\n\nType /HELP <command> for more information, or /HELP -l\n\n"));
		strcat (buf, _("User defined commands:\n\n  "));
		t = 1;
		while (list)
		{
			pop = (struct popup *) list->data;
			strcat (buf, pop->name);
			t++;
			if (t == 6)
			{
				t = 1;
				strcat (buf, "\n");
				PrintText (sess, buf);
				strcpy (buf, "  ");
			} else
			{
				if (strlen (pop->name) < 10)
				{
					for (j = 0; j < (10 - strlen (pop->name)); j++)
						strcat (buf, " ");
				}
			}
			list = list->next;
		}
		strcat (buf, "\n");
		PrintText (sess, buf);
		free (buf);
	}
	return TRUE;
}

int
cmd_ignore (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	int i;
	int priv = 0;
	int noti = 0;
	int chan = 0;
	int ctcp = 0;
	int invi = 0;
	int unignore = 0;
	int quiet = 0;
	int no_save = 0;

	if (!*word[2])
	{
		ignore_showlist (sess);
		return TRUE;
	}
	if (!*word[3])
		return FALSE;

	i = 3;
	while (1)
	{
		if (!*word[i])
		{
			if (!priv && !noti && !chan && !ctcp && !invi && !unignore)
				return FALSE;
			i =
				ignore_add (word[2], priv, noti, chan, ctcp, invi, unignore,
								no_save);
			if (!quiet)
			{
				if (i == 1)
					EMIT_SIGNAL (XP_TE_IGNOREADD, sess, word[2], NULL, NULL, NULL,
									 0);
				if (i == 2)			  /* old ignore changed */
					EMIT_SIGNAL (XP_TE_IGNORECHANGE, sess, word[2], NULL, NULL,
									 NULL, 0);
			}
			return TRUE;
		}
		if (!strcasecmp (word[i], "UNIGNORE"))
			unignore = 1;
		else if (!strcasecmp (word[i], "ALL"))
			priv = noti = chan = ctcp = invi = 1;
		else if (!strcasecmp (word[i], "PRIV"))
			priv = 1;
		else if (!strcasecmp (word[i], "NOTI"))
			noti = 1;
		else if (!strcasecmp (word[i], "CHAN"))
			chan = 1;
		else if (!strcasecmp (word[i], "CTCP"))
			ctcp = 1;
		else if (!strcasecmp (word[i], "INVI"))
			invi = 1;
		else if (!strcasecmp (word[i], "QUIET"))
			quiet = 1;
		else if (!strcasecmp (word[i], "NOSAVE"))
			no_save = 1;
		else
		{
			sprintf (tbuf, _("Unknown arg '%s' ignored."), word[i]);
			PrintText (sess, tbuf);
		}
		i++;
	}
}

int
cmd_invite (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	if (!*word[2])
		return FALSE;
	if (*word[3])
		sprintf (tbuf, "INVITE %s %s\r\n", word[2], word[3]);
	else
		sprintf (tbuf, "INVITE %s %s\r\n", word[2], sess->channel);
	tcp_send (sess->server, tbuf);
	return TRUE;
}

int
cmd_join (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	char *chan = word[2];
	if (*chan)
	{
		char *po, *pass = word[3];
		if (*pass)
			sprintf (tbuf, "JOIN %s %s\r\n", chan, pass);
		else
			sprintf (tbuf, "JOIN %s\r\n", chan);
		tcp_send (sess->server, tbuf);
		if (sess->channel[0] == 0 && !prefs.persist_chans)
		{
			po = strchr (chan, ',');
			if (po)
				*po = 0;
			safe_strcpy (sess->waitchannel, chan, CHANLEN);
		}
		return TRUE;
	}
	return FALSE;
}

int
cmd_kick (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	char *nick = word[2];
	char *reason = word_eol[3];
	if (*nick)
	{
		if (*reason)
			sprintf (tbuf, "KICK %s %s :%s\r\n", sess->channel, nick, reason);
		else
			sprintf (tbuf, "KICK %s %s\r\n", sess->channel, nick);
		tcp_send (sess->server, tbuf);
		return TRUE;
	}
	return FALSE;
}

int
cmd_kickban (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	char *nick = word[2];
	char *reason = word_eol[3];
	struct User *user;

	if (*nick)
	{
		/* if the reason is a 1 digit number, treat it as a bantype */

		user = find_name (sess, nick);

		if (isdigit (reason[0]) && reason[1] == 0)
		{
			ban (sess, tbuf, nick, reason, (user && user->op));
			reason[0] = 0;
		} else
			ban (sess, tbuf, nick, "", (user && user->op));

		if (*reason)
			sprintf (tbuf, "KICK %s %s :%s\r\n", sess->channel, nick, reason);
		else
			sprintf (tbuf, "KICK %s %s\r\n", sess->channel, nick);
		tcp_send (sess->server, tbuf);

		return TRUE;
	}
	return FALSE;
}

int
cmd_lagcheck (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	lag_check ();
	return TRUE;
}

static void
lastlog (session *sess, char *search)
{
	session *lastlog_sess;

	if (!is_session (sess))
		return;

	lastlog_sess = find_dialog (sess->server, "(lastlog)");
	if (!lastlog_sess)
	{
		lastlog_sess = new_ircwindow (sess->server, "(lastlog)", SESS_DIALOG);
		lastlog_sess->lastlog_sess = sess;
	}

	fe_text_clear (lastlog_sess);

	fe_lastlog (sess, lastlog_sess, search);
}

int
cmd_lastlog (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	if (*word_eol[2])
	{
		lastlog (sess, word_eol[2]);
		return TRUE;
	}

	return FALSE;
}

int
cmd_list (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	if (*word_eol[2])
	{
		sprintf (tbuf, "LIST %s\r\n", word_eol[2]);
		tcp_send (sess->server, tbuf);
	} else
	{
		if (sess->server->is_newtype)
			tcp_send (sess->server, "LIST >0,<10000\r\n");
		else
			tcp_send (sess->server, "LIST\r\n");
	}

	return TRUE;
}

int
cmd_load (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	FILE *fp;
	char *nl;
	char *file;
#ifdef USE_PERL
	int i;
#endif

	if (strcmp (word[2], "-e") == 0)
	{
		file = expand_homedir (word[3]);
		fp = fopen (file, "r");
		free (file);
		if (fp)
		{
			tbuf[1024] = 0;
			while (fgets (tbuf, 1024, fp))
			{
				nl = strchr (tbuf, '\n');
				if (nl)
					*nl = 0;
				handle_command (tbuf, sess, FALSE, FALSE);
			}
			fclose (fp);
		}
		return TRUE;
	}

#ifdef USE_PERL
	file = expand_homedir (word[2]);
	i = perl_load_file (file);
	free (file);
	switch (i)
	{
	case 0:
		return TRUE;
	case 1:
		PrintText (sess, _("Error compiling script\n"));
		return FALSE;
	case 2:
		PrintText (sess, _("Error Loading file\n"));
		return FALSE;
	}
	return FALSE;
#else
	PrintText (sess, _("Perl scripting not available in this compilation.\n"));
	return TRUE;
#endif
}

#ifdef USE_PLUGIN
int
cmd_loaddll (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	char *file;
	int i;

	file = expand_homedir (word[2]);

	i = module_load (file, sess);

	free (file);

	if (i == 0)
		return TRUE;
	return FALSE;
}
#endif

int
cmd_me (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	char *act = word_eol[2];

	if (!(*act))
		return FALSE;

	sprintf (tbuf, "\001ACTION %s\001\r", act);
	/* first try through DCC CHAT */
	if (dcc_write_chat (sess->channel, tbuf))
	{
		/* print it to screen */
		channel_action (sess, tbuf, sess->channel, sess->server->nick,
								act, TRUE);
	} else
	{
		/* DCC CHAT failed, try through server */
		if (sess->server->connected)
		{
			sprintf (tbuf, "PRIVMSG %s :\001ACTION %s\001\r\n", sess->channel,
						act);
			tcp_send (sess->server, tbuf);
			/* print it to screen */
			channel_action (sess, tbuf, sess->channel, sess->server->nick,
									act, TRUE);
		} else
		{
			notc_msg (sess);
		}
	}

	return TRUE;
}

int
cmd_msg (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	char *nick = word[2];
	char *msg = word_eol[3];
	struct session *newsess;

	if (*nick)
	{
		if (*msg)
		{
			if (strcmp (nick, ".") == 0)
			{							  /* /msg the last nick /msg'ed */
				if (sess->lastnick[0])
					nick = sess->lastnick;
			} else
				strcpy (sess->lastnick, nick);	/* prime the last nick memory */

			if (*nick == '=')
			{
				nick++;
				if (!dcc_write_chat (nick, msg))
				{
					EMIT_SIGNAL (XP_TE_NODCC, sess, NULL, NULL, NULL, NULL, 0);
					return TRUE;
				}
			} else
			{
				if (!sess->server->connected)
				{
					notc_msg (sess);
					return TRUE;
				}
				sprintf (tbuf, "PRIVMSG %s :%s\r\n", nick, msg);
				tcp_send (sess->server, tbuf);
			}
			newsess = find_session_from_channel (nick, sess->server);
			if (newsess)
				channel_msg (newsess->server, tbuf, newsess->channel,
						newsess->server->nick, msg, TRUE);
			else
				EMIT_SIGNAL (XP_TE_MSGSEND, sess, nick, msg, NULL, NULL, 0);

			return TRUE;
		}
	}
	return FALSE;
}

int
cmd_names (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	if (*word[2])
		sprintf (tbuf, "NAMES %s\r\n", word[2]);
	else
		sprintf (tbuf, "NAMES %s\r\n", sess->channel);
	tcp_send (sess->server, tbuf);
	return TRUE;
}

int
cmd_nctcp (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	if (*word[2] && *word_eol[3])
	{
		sprintf (tbuf, "NOTICE %s :\001%s\001\r\n", word[2], word_eol[3]);
		tcp_send (sess->server, tbuf);
		return TRUE;
	}
	return FALSE;
}

int
cmd_newserver (struct session *sess, char *tbuf, char *word[],
					char *word_eol[])
{
	sess = new_ircwindow (NULL, NULL, SESS_SERVER);
	cmd_server (sess, tbuf, word, word_eol);
	return TRUE;
}

int
cmd_nick (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	char *nick = word[2];
	if (*nick)
	{
		if (sess->server->connected)
		{
			sprintf (tbuf, "NICK %s\r\n", nick);
			tcp_send (sess->server, tbuf);
		} else
			user_new_nick (sess->server, sess->server->nick, nick, TRUE);
		return TRUE;
	}
	return FALSE;
}

int
cmd_notice (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	if (*word[2] && *word_eol[3])
	{
		sprintf (tbuf, "NOTICE %s :%s\r\n", word[2], word_eol[3]);
		tcp_send (sess->server, tbuf);
		EMIT_SIGNAL (XP_TE_NOTICESEND, sess, word[2], word_eol[3], NULL, NULL,
						 0);
		return TRUE;
	}
	return FALSE;
}

int
cmd_notify (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	int i = 1;

	if (*word[2])
	{
		while (1)
		{
			i++;
			if (!*word[i])
				break;
			if (notify_deluser (word[i]))
			{
				EMIT_SIGNAL (XP_TE_DELNOTIFY, sess, word[i], NULL, NULL, NULL, 0);
				return TRUE;
			}
			notify_adduser (word[i]);
			EMIT_SIGNAL (XP_TE_ADDNOTIFY, sess, word[i], NULL, NULL, NULL, 0);
		}
	} else
		notify_showlist (sess);
	return TRUE;
}

int
cmd_op (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	int i = 2;

	while (1)
	{
		if (!*word[i])
		{
			if (i == 2)
				return FALSE;
			send_channel_modes (sess, tbuf, word, 2, i, '+', 'o');
			return TRUE;
		}
		i++;
	}
}

int
cmd_part (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	char *chan = word[2];
	char *reason = word_eol[3];
	if (!*chan)
		chan = sess->channel;
	if ((*chan) && is_channel (sess->server, chan))
	{
		server_sendpart (sess->server, chan, reason);
		return TRUE;
	}
	return FALSE;
}

int
cmd_ping (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	unsigned long tim;
	char *to = word[2];

	tim = make_ping_time ();

	if (*to)
		sprintf (tbuf, "PRIVMSG %s :\001PING %lu\001\r\n", to, tim);
	else
		sprintf (tbuf, "PING %lu\r\n", tim);
		/*sprintf (tbuf, "PING %lu :%s\r\n", tim, sess->server->servername);*/
	tcp_send (sess->server, tbuf);

	return TRUE;
}

int
cmd_query (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	char *nick = word[2];
	if (*nick)
	{
		if (!find_dialog (sess->server, nick))
			new_ircwindow (sess->server, nick, SESS_DIALOG);
		return TRUE;
	}
	return FALSE;
}

int
cmd_quote (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	int len;
	char *raw = word_eol[2];
	if (*raw)
	{
		len = strlen (raw);
		if (len < 4094)
		{
			tcp_send_len (sess->server, tbuf, sprintf (tbuf, "%s\r\n", raw));
		} else
		{
			tcp_send_len (sess->server, raw, len);
			tcp_send_len (sess->server, "\r\n", 2);
		}
		return TRUE;
	}
	return FALSE;
}

int
cmd_reconnect (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	int tmp = prefs.recon_delay;
	GSList *list;
	server *serv;

	prefs.recon_delay = 0;

	if (!strcasecmp (word[2], "ALL"))
	{
		list = serv_list;
		while (list)
		{
			serv = list->data;
			if (serv->connected)
				auto_reconnect (serv, TRUE, -1);
			list = list->next;
		}
	}	
	/* If it isn't "ALL" and there is something 
	there it *should* be a server they are trying to connect to*/
	else if (*word[2])
	{
		int offset = 0;
#ifdef USE_OPENSSL
		int use_ssl = FALSE;

		if (strcmp (word[2], "-ssl") == 0)
		{
			use_ssl = TRUE;
			offset++;	/* args move up by 1 word */
		}
		sess->server->use_ssl = use_ssl;
		sess->server->accept_invalid_cert = TRUE;
#endif

		if (*word[4+offset])
			safe_strcpy (sess->server->password, word[4+offset], sizeof (sess->server->password));
		if (*word[3+offset])
			sess->server->port = atoi (word[3+offset]);
		safe_strcpy (sess->server->hostname, word[2+offset], sizeof (sess->server->hostname));
		auto_reconnect (sess->server, TRUE, -1);
	}
	else
	{
		auto_reconnect (sess->server, TRUE, -1);
	}	
	prefs.recon_delay = tmp;

	return TRUE;
}

#ifdef USE_PLUGIN
int
cmd_rmdll (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	char *dllname = word[2];

	if (module_unload (dllname, sess) == 0)
		return TRUE;
	return FALSE;
}
#endif

int
cmd_say (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	char *speech = word_eol[2];
	if (*speech)
	{
		channel_msg (sess->server, tbuf, sess->channel, sess->server->nick,
						 speech, TRUE);
		sprintf (tbuf, "PRIVMSG %s :%s\r\n", sess->channel, speech);
		tcp_send (sess->server, tbuf);
		return TRUE;
	}
	return FALSE;
}

int
cmd_settab (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	if (*word_eol[2])
	{
		strcpy (tbuf, sess->channel);
		safe_strcpy (sess->channel, word_eol[2], CHANLEN);
		fe_set_channel (sess);
		strcpy (sess->channel, tbuf);
	}

	return TRUE;
}

int
cmd_server (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	int offset = 0;
	char *server;
	char *port;
	char *pass;
	char *co;
#ifdef USE_OPENSSL
	int use_ssl = FALSE;

	if (strcmp (word[2], "-ssl") == 0)
	{
		use_ssl = TRUE;
		offset++;	/* args move up by 1 word */
	}
#endif

	server = word[2 + offset];
	port = word[3 + offset];
	pass = word[4 + offset];

	if (!(*server))
		return FALSE;

#ifdef USE_OPENSSL
	if (strncasecmp ("ircs://", server, 7) == 0)
	{
		use_ssl = TRUE;
		server += 7;
		goto urlserv;
	}
#endif

	/* dont clear it for /servchan */
	if (strncasecmp (word[1], "SERVCHAN ", 9))
		sess->willjoinchannel[0] = 0;

	if (strncasecmp ("irc://", server, 6) == 0)
	{
		server += 6;
#ifdef USE_OPENSSL
urlserv:
#endif
		/* check for port */
		co = strchr (server, ':');
		if (co)
		{
			port = co + 1;
			*co = 0;
			pass = word[3 + offset];
		} else
			co = server;
		/* check for channel - mirc style */
		co = strchr (co + 1, '/');
		if (co)
		{
			*co = 0;
			co++;
			if (*co == '#')
			{
				safe_strcpy (sess->willjoinchannel, co, CHANLEN);
			} else
			{
				sess->willjoinchannel[0] = '#';
				strncpy (sess->willjoinchannel + 1, co, CHANLEN);
				sess->willjoinchannel[CHANLEN-1] = 0;
			}
		}
	}

	if (*pass)
		strcpy (sess->server->password, pass);
#ifdef USE_OPENSSL
	sess->server->use_ssl = use_ssl;
	sess->server->accept_invalid_cert = TRUE;
#endif

	if (*port)
	{
		connect_server (sess, server, atoi (port), FALSE);
	} else {
#ifdef USE_OPENSSL
		if (use_ssl)
			connect_server (sess, server, 994, FALSE);
		else
#endif
			connect_server (sess, server, 6667, FALSE);
	}
	return TRUE;
}

int
cmd_servchan (struct session *sess, char *tbuf, char *word[],
				  char *word_eol[])
{
	int offset = 0;

#ifdef USE_OPENSSL
	if (strcmp (word[2], "-ssl") == 0)
		offset++;
#endif

	if (*word[4 + offset])
	{
		strcpy (sess->willjoinchannel, word[4 + offset]);
		return cmd_server (sess, tbuf, word, word_eol);
	}

	return FALSE;
}

struct timercommand
{
	session *sess;
	char *cmd;
};

static int
timer_timeout (struct timercommand *tr)
{
	if (is_session (tr->sess))
		handle_command (tr->cmd, tr->sess, 0, 0);
	free (tr->cmd);
	free (tr);
	return 0;
}

int
cmd_timer (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	struct timercommand *tr;

	if (!*word_eol[3])
		return FALSE;

	tr = malloc (sizeof (struct timercommand));
	tr->sess = sess;
	tr->cmd = strdup (word_eol[3]);

	fe_timeout_add (atoi (word[2]) * 1000, timer_timeout, tr);

	return TRUE;
}

int
cmd_topic (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	char *topic = word_eol[2];
	if (*topic)
		sprintf (tbuf, "TOPIC %s :%s\r\n", sess->channel, topic);
	else
		sprintf (tbuf, "TOPIC %s\r\n", sess->channel);
	tcp_send (sess->server, tbuf);
	return TRUE;
}

int
cmd_unignore (struct session *sess, char *tbuf, char *word[],
				  char *word_eol[])
{
	char *mask = word[2];
	char *arg = word[3];
	if (*mask)
	{
		if (ignore_del (mask, NULL))
		{
			if (strcasecmp (arg, "QUIET"))
				EMIT_SIGNAL (XP_TE_IGNOREREMOVE, sess, mask, NULL, NULL, NULL, 0);
		}
		return TRUE;
	}
	return FALSE;
}

int
cmd_userlist (struct session *sess, char *tbuf, char *word[],
				  char *word_eol[])
{
	struct User *user;
	GSList *list;
	int lt;

	list = sess->userlist;
	while (list)
	{
		user = list->data;
		lt = time (0) - user->lasttalk;
		if (!user->lasttalk)
			lt = 0;
		sprintf (tbuf,
					"\00306%s\t\00314[\00310%-38s\00314] \017ov\0033=\017%d%d lt\0033=\017%d\n",
					user->nick, user->hostname, user->op, user->voice, lt);
		PrintText (sess, tbuf);
		list = list->next;
	}

	return TRUE;
}

int
cmd_wallchop (struct session *sess, char *tbuf, char *word[],
				  char *word_eol[])
{
	int i = 0;
	struct User *user;
	GSList *list;

	if (*word_eol[2])
	{
		strcpy (tbuf, "NOTICE ");
		list = sess->userlist;
		while (list)
		{
			user = (struct User *) list->data;
			if (user->op)
			{
				if (i)
					strcat (tbuf, ",");
				strcat (tbuf, user->nick);
				i++;
			}
			if ((i == 5 || !list->next) && i)
			{
				i = 0;
				sprintf (tbuf + strlen (tbuf),
							" :[@%s] %s\r\n", sess->channel, word_eol[2]);
				tcp_send (sess->server, tbuf);
				strcpy (tbuf, "NOTICE ");
			}
			list = list->next;
		}
		return TRUE;
	}
	return FALSE;
}

int
cmd_wallchan (struct session *sess, char *tbuf, char *word[],
				  char *word_eol[])
{
	GSList *list;

	if (*word_eol[2])
	{
		list = sess_list;
		while (list)
		{
			sess = list->data;
			if (sess->type == SESS_CHANNEL)
			{
				channel_msg (sess->server, tbuf, sess->channel, sess->server->nick,
								 word_eol[2], TRUE);
				tcp_send_len (sess->server, tbuf,
								  sprintf (tbuf, "PRIVMSG %s :%s\r\n", sess->channel,
											  word_eol[2]));
			}
			list = list->next;
		}
		return TRUE;
	}
	return FALSE;
}

int
cmd_hop (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	int i = 2;

	while (1)
	{
		if (!*word[i])
		{
			if (i == 2)
				return FALSE;
			send_channel_modes (sess, tbuf, word, 2, i, '+', 'h');
			return TRUE;
		}
		i++;
	}
}

int
cmd_voice (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	int i = 2;

	while (1)
	{
		if (!*word[i])
		{
			if (i == 2)
				return FALSE;
			send_channel_modes (sess, tbuf, word, 2, i, '+', 'v');
			return TRUE;
		}
		i++;
	}
}

int
cmd_flushq (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	char buf[128];

	sprintf (buf, "Flushing server send queue, %d bytes.\n", sess->server->sendq_len);
	PrintText (sess, buf);
	flush_server_queue (sess->server);
	return TRUE;
}

/* inserts %a, %c, %d etc into buffer. Also handles &x %x for word/word_eol. *
 *   returns 1 on success                                                    *
 *   returns 0 on bad-args-for-user-command                                  *
 * - word/word_eol args might be NULL                                        *
 * - this beast is used for UserCommands, UserlistButtons and CTCP replies   */

int
auto_insert (char *dest, char *src, char *word[], char *word_eol[],
				 char *a, char *c, char *d, char *h, char *n, char *s)
{
	int num;
	char buf[4];
	time_t now;
	struct tm *tm_ptr;

	while (src[0])
	{
		if (src[0] == '%' || src[0] == '&')
		{
			if (isdigit (src[1]))
			{
				if (isdigit (src[2]) && isdigit (src[3]))
				{
					buf[0] = src[1];
					buf[1] = src[2];
					buf[2] = src[3];
					buf[3] = 0;
					dest[0] = atoi (buf);
					dest++;
					src += 3;
				} else
				{
					if (word)
					{
						src++;
						num = src[0] - '0';	/* ascii to decimal */
						if (*word[num] == 0)
							return 0;
						if (src[-1] == '%')
							strcpy (dest, word[num]);
						else
							strcpy (dest, word_eol[num]);
						dest += strlen (dest);
					}
				}
			} else
			{
				if (src[0] == '&')
					goto lamecode;
				src++;
				switch (src[0])
				{
				case '%':
					dest[0] = '%';
					dest[1] = 0;
					break;
				case 'a':
					strcpy (dest, a);
					break;
				case 'c':
					strcpy (dest, c);
					break;
				case 'd':
					strcpy (dest, d);
					break;
				case 'h':
					strcpy (dest, h);
					break;
				case 'm':
					strcpy (dest, get_cpu_str ());
					break;
				case 'n':
					strcpy (dest, n);
					break;
				case 's':
					strcpy (dest, s);
					break;
				case 't':
					now = time (0);
					memcpy (dest, ctime (&now), 19);
					dest[19] = 0;
					break;
				case 'v':
					strcpy (dest, VERSION);
					break;
				case 'y':
					now = time (0);
					tm_ptr = localtime (&now);
					sprintf (dest, "%4d%02d%02d", 1900 + tm_ptr->tm_year,
								1 + tm_ptr->tm_mon, tm_ptr->tm_mday);
					break;
				default:
					src--;
					goto lamecode;
				}
				dest += strlen (dest);
			}
		} else
		{
		 lamecode:
			dest[0] = src[0];
			dest++;
		}
		src++;
	}

	dest[0] = 0;

	return 1;
}

void
check_special_chars (char *cmd, int do_ascii) /* check for %X */
{
	int occur = 0;
	int len = strlen (cmd);
	char *buf;
	int i = 0, j = 0;
	char tbuf[4];

	if (!len)
		return;

	buf = malloc (len + 1);

	if (buf)
	{
		while (cmd[j])
		{
			switch (cmd[j])
			{
			case '%':
				occur++;
				if (	do_ascii &&
						j + 3 < len &&
						(isdigit (cmd[j + 1]) && isdigit (cmd[j + 2]) &&
						isdigit (cmd[j + 3])))
				{
					tbuf[0] = cmd[j + 1];
					tbuf[1] = cmd[j + 2];
					tbuf[2] = cmd[j + 3];
					tbuf[3] = 0;
					buf[i] = atoi (tbuf);
					j += 3;
				} else
				{
					switch (cmd[j + 1])
					{
					case 'R':
						buf[i] = '\026';
						break;
					case 'U':
						buf[i] = '\037';
						break;
					case 'B':
						buf[i] = '\002';
						break;
					case 'C':
						buf[i] = '\003';
						break;
					case 'O':
						buf[i] = '\017';
						break;
					case '%':
						buf[i] = '%';
						break;
					default:
						buf[i] = '%';
						j--;
						break;
					}
					j++;
					break;
			default:
					buf[i] = cmd[j];
				}
			}
			j++;
			i++;
		}
		buf[i] = 0;
		if (occur)
			strcpy (cmd, buf);
		free (buf);
	}
}

static void
perform_nick_completion (struct session *sess, char *cmd, char *tbuf)
{
	long len;
	char *space = strchr (cmd, ' ');
	if (space && space != cmd)
	{
		if (((space[-1] == ':') || (space[-1] == prefs.nick_suffix[0])) && (space - 1 != cmd))
		{
			len = (long) space - (long) cmd - 1;
			if (len < 64)
			{
				struct User *user;
				struct User *best = NULL;
				int bestlen = INT_MAX, lenu;
				char nick[64];
				GSList *list;

				memcpy (nick, cmd, len);
				nick[len] = 0;

				list = sess->userlist;
				while (list)
				{
					user = (struct User *) list->data;
					if (!strncasecmp (user->nick, nick, len))
					{
						lenu = strlen (user->nick);
						if (lenu == len)
						{
							sprintf (tbuf, "%s%s", user->nick, space - 1);
							return;
						} else if (lenu < bestlen)
						{
							bestlen = lenu;
							best = user;
						}
					}
					list = list->next;
				}
				if (best)
				{
					sprintf (tbuf, "%s%s", best->nick, space - 1);
					return;
				}
			}
		}
	}

	strcpy (tbuf, cmd);
}

static void
user_command (session * sess, char *tbuf, char *cmd, char *word[],
				  char *word_eol[])
{
	if (!auto_insert (tbuf, cmd, word, word_eol, "", sess->channel, "", "",
							sess->server->nick, ""))
	{
		PrintText (sess, _("Bad arguments for user command.\n"));
		return;
	}

	handle_command (tbuf, sess, FALSE, FALSE);
}

int
handle_command (char *cmd, session *sess, int history, int nocommand)
{
	struct popup *pop;
	int user_cmd = FALSE, i;
	GSList *list = command_list;
	unsigned char pdibuf[2048];
	unsigned char newcmd[2048];
	unsigned char tbuf[4096];
	char *word[PDIWORDS];
	char *word_eol[PDIWORDS];
	struct DCC *dcc;
	static int command_level = 0;

	if (!sess || !*cmd)
		return TRUE;

	if (command_level > 99)
	{
		fe_message (_("Too many recursive usercommands, aborting."), FALSE);
		return TRUE;
	}
	command_level++;
	/* anything below MUST DEC command_level before returning */

	if (history)
		history_add (&sess->history, cmd);

#ifdef USE_PERL
	if (perl_command (cmd, sess))
		goto xit;
#endif

	/* Check if the user is pasting output from a terminal. Don't confuse
	 * a unix directory for a command
	 */
	if (cmd[0] == '/') {
		const char *unix_dirs [] = {
			"/bin/", "/boot/", "/dev/",
			"/etc/", "/home/", "/lib/",
			"/lost+found/", "/mnt/", "/opt/",
			"/proc/", "/root/", "/sbin/",
			"/tmp/", "/usr/", "/var/",
			"/gnome/", NULL};
		for (i = 0; unix_dirs[i] != NULL; i++)
			if (strncmp (cmd, unix_dirs[i], strlen (unix_dirs[i]))==0) {
				nocommand = TRUE;
				break;
			}
	}

	if (!nocommand && cmd[0] == prefs.cmdchar[0] && cmd[1] == prefs.cmdchar[0])
	{
		nocommand = TRUE;
		cmd++;
	}

	if (cmd[0] == prefs.cmdchar[0] && !nocommand)
	{
		cmd++;

		process_data_init (pdibuf, cmd, word, word_eol, TRUE);

		if (EMIT_SIGNAL (XP_USERCOMMAND, sess, pdibuf, word, word_eol, NULL, 0)
			 == 1)
			goto xit;

#ifdef USE_PLUGIN
		if (module_command (pdibuf, sess, tbuf, word, word_eol) == 0)
			goto xit;
#endif
#ifdef USE_PYTHON
		if (pys_cmd_handle (pdibuf, sess, word))
			goto xit;
#endif

		/* first see if it's a userCommand */
		while (list)
		{
			pop = (struct popup *) list->data;
			if (!strcasecmp (pop->name, pdibuf))
			{
				user_command (sess, tbuf, pop->cmd, word, word_eol);
				user_cmd = TRUE;
			}
			list = list->next;
		}

		if (user_cmd)
			goto xit;

		if (prefs.perc_color)
			check_special_chars (cmd, prefs.perc_ascii);

		/* now check internal commands */
		i = 0;
		while (1)
		{
			if (!xc_cmds[i].name)
				break;
			if (!strcasecmp (pdibuf, xc_cmds[i].name))
			{
				if (xc_cmds[i].needserver && !sess->server->connected)
				{
					notc_msg (sess);
					goto xit;
				}
				if (xc_cmds[i].needchannel && !sess->channel[0])
				{
					notj_msg (sess);
					goto xit;
				}
				switch (xc_cmds[i].callback (sess, tbuf, word, word_eol))
				{
				case FALSE:
					help (sess, xc_cmds[i].name, TRUE);
					break;
				case 2:
					command_level--;
					return FALSE;
				}
				goto xit;
			}
			i++;
		}
		if (!sess->server->connected)
		{
			PrintText (sess,_( "Unknown Command. Try /help\n"));
			goto xit;
		}
		sprintf (tbuf, "%s\r\n", cmd);
		tcp_send (sess->server, tbuf);
		goto xit;
	}

	if (strcmp (sess->channel, "(lastlog)") == 0)
	{
		lastlog (sess->lastlog_sess, cmd);
		goto xit;
	}

	if (prefs.perc_color)
		check_special_chars (cmd, prefs.perc_ascii);

	if (!sess->channel[0] || sess->type == SESS_SERVER || sess->type == SESS_NOTICES || sess->type == SESS_SNOTICES)
	{
		notj_msg (sess);
		goto xit;
	}

	if (prefs.nickcompletion)
		perform_nick_completion (sess, cmd, newcmd);
	else
		strncpy (newcmd, cmd, sizeof (newcmd));

	cmd = newcmd;

	if (sess->type == SESS_DIALOG)
	{
		/* try it via dcc, if possible */
		dcc = dcc_write_chat (sess->channel, cmd);
		if (dcc)
		{
			channel_msg (sess->server, tbuf, sess->channel,
							 sess->server->nick, cmd, TRUE);
			set_topic (sess, net_ip (dcc->addr));
			goto xit;
		}
	}

	if (sess->server->connected)
	{
		unsigned int max;
		unsigned char t = 0;

		/* maximum allowed message text */
		max = 456 - strlen (sess->channel);

		if (strlen (cmd) > max)
		{
			t = cmd[max];
			cmd[max] = 0;			  /* insert a NULL terminator to shorten it */
		}

		channel_msg (sess->server, tbuf, sess->channel,
						 sess->server->nick, cmd, TRUE);
		sprintf (tbuf, "PRIVMSG %s :%s\r\n", sess->channel, cmd);
		tcp_send (sess->server, tbuf);

		if (t)
		{
			cmd[max] = t;
			handle_command (cmd + max, sess, history, nocommand);
		}

	} else
	{
		notc_msg (sess);
	}

xit:
	command_level--;
	return TRUE;
}

void
handle_multiline (struct session *sess, char *cmd, int history, int nocommand)
{
	char *cr;

	cr = strchr (cmd, '\n');
	if (cr)
	{
		while (1)
		{
			if (cr)
				*cr = 0;
			if (!handle_command (cmd, sess, history, nocommand))
				return;
			if (!cr)
				break;
			cmd = cr + 1;
			if (*cmd == 0)
				break;
			cr = strchr (cmd, '\n');
		}
	} else
	{
		if (!handle_command (cmd, sess, history, nocommand))
			return;
	}

}
