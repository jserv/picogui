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
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>
#include "../common/xchat.h"
#include "../common/xchatc.h"
#include "../common/outbound.h"
#include "../common/util.h"
#include "../common/fe.h"

#include <picogui.h>
#include "fe-picogui.h"

static GSList *tmr_list;		  /* timer list */
static int tmr_list_count;
static GSList *se_list;			  /* socket event list */
static int se_list_count;
static pghandle pgEmptyString, pgPlusHTML;
static short int output_type=PG_WIDGET_TERMINAL;

static int
fieldActivate(struct pgEvent *evt)
{
	char *cmd;
	pghandle handle;

	handle=pgGetWidget(evt->from, PG_WP_TEXT);
	if(handle&&(cmd=strdup(pgGetString(handle))))
	{
		pgSetWidget(evt->from, PG_WP_TEXT, pgEmptyString, 0);
		/* Must do handle_command last because it may be /close */
		handle_command (cmd, evt->extra, TRUE, FALSE);
		free(cmd);
	}
	return 0;
}
static int
evtActionsToggle(struct pgEvent *evt)
{
	pgSetWidget(((struct session_gui*)evt->extra)->buttonbox,
		    PG_WP_SIZE, pgGetWidget(evt->from,PG_WP_ON) ? -1 : 0,
		    0);
	return 0;
}
static int
evtCloseMessage(struct pgEvent *evt)
{
	pghandle title=pgGetWidget(evt->from, PG_WP_TEXT);
	pgDelete(evt->from);
	pgDelete(title);
	/* for labels, but not textboxen */
	if(evt->extra)
		pgDelete((pghandle)evt->extra);
	return 1;
}
static int
evtCloseWindow(struct pgEvent *evt)
{
	fe_close_window(evt->extra);
	/* 1 prevents us from exiting */
	return 1;
}

static int
evtPassFocus(struct pgEvent *evt)
{
	int i;
	union pg_client_trigger trig;

	pgFocus((pghandle)evt->extra);

	/* hope they don't send things with mods.. */
	for(i=0;i<evt->e.data.size;i++) {
		trig.content.type = PG_TRIGGER_CHAR;
		trig.content.u.kbd.key = evt->e.data.pointer[i];
		pgInFilterSend(&trig);
	}
	return 1;
}

static int done_intro = 0;

void
fe_new_window (struct session *sess)
{
	char buf[512];
	pghandle scroll, rightbox, boxbar;

	sess->gui = malloc(sizeof(struct session_gui));
	memset(sess->gui, 0, sizeof(struct session_gui));

	/* App */
	sess->gui->app = pgRegisterApp(PG_APP_NORMAL, "X-Chat ["VERSION"]", 0);
	fe_set_title(sess);
	pgBind(0, PG_WE_CLOSE, evtCloseWindow, sess);

	sess->gui->input = pgNewWidget(PG_WIDGET_FIELD, 0, 0);
	pgSetWidget(0, PG_WP_SIDE, PG_S_BOTTOM, 0);
	pgBind(0, PG_WE_ACTIVATE, fieldActivate, sess);

	/* Chat area */
	if(sess->type!=SESS_DIALOG)
	{
		rightbox=pgNewWidget(PG_WIDGET_BOX, 0, 0);
		pgSetWidget(PGDEFAULT,
			    PG_WP_SIDE, PG_S_RIGHT,
			    PG_WP_MARGIN, 0,
			    0);

		/* Add a panelbar to resize the right box with */
		boxbar = pgNewWidget(PG_WIDGET_PANELBAR, PG_DERIVE_INSIDE, rightbox);
		pgSetWidget(PGDEFAULT,
			    PG_WP_SIDE, PG_S_LEFT,
			    PG_WP_BIND, rightbox,
			    0);
		sess->gui->userlistinfo = pgNewWidget(PG_WIDGET_LABEL,PG_DERIVE_INSIDE, 0);
		pgSetWidget(PGDEFAULT,
			    PG_WP_DIRECTION, PG_DIR_VERTICAL,
			    PG_WP_SIDE, PG_S_ALL,
			    PG_WP_THOBJ, PGTH_O_PANELBAR,
			    PG_WP_TEXT, pgNewString("User List (Drag to resize)"),
			    0);
		pgNewWidget(PG_WIDGET_BUTTON, PG_DERIVE_BEFORE, 0);
		pgSetWidget(PGDEFAULT,
			    PG_WP_DIRECTION, PG_DIR_VERTICAL,
			    PG_WP_SIDE, PG_S_BOTTOM,
			    PG_WP_TEXT, pgNewString("Actions"),
			    PG_WP_EXTDEVENTS, PG_EXEV_TOGGLE,
			    0);
		pgBind(PGDEFAULT, PG_WE_ACTIVATE, evtActionsToggle, sess->gui);

		/* scroll bug: the scroll doesn't recalculate correctly
		 * when resized. be sure to have the top of the nicklist
		 * visible when enlarging */
		sess->gui->userscroll=pgNewWidget(PG_WIDGET_SCROLL, PG_DERIVE_AFTER, boxbar);
		sess->gui->userlist=pgNewWidget(PG_WIDGET_BOX, 0, 0);
		pgSetWidget(0, PG_WP_SIDE, PG_S_ALL, 0);
		pgSetWidget (sess->gui->userscroll, PG_WP_BIND,
				sess->gui->userlist, 0);
		sess->gui->buttonbox = pgNewWidget(PG_WIDGET_BOX, PG_DERIVE_AFTER, boxbar);
		pgSetWidget(PGDEFAULT, 
			    PG_WP_SIDE, PG_S_BOTTOM,
			    PG_WP_SIZE, 0,
			    0);
		fe_buttons_update(sess);
		pgNewWidget(PG_WIDGET_BOX, 0, rightbox);
	}
	else
		pgNewWidget(PG_WIDGET_BOX, 0, 0);

	pgSetWidget(0, 
		    PG_WP_SIDE, PG_S_ALL,
		    PG_WP_MARGIN, 0,
		    0);
	scroll=pgNewWidget(PG_WIDGET_SCROLL, PG_DERIVE_INSIDE, 0);
	pgSetWidget(0, PG_WP_SIDE, PG_S_RIGHT, 0);
	/* can be set from command line */
	sess->gui->output_type = output_type;
	sess->gui->output = pgNewWidget(output_type, 0, 0);
	pgSetWidget(scroll, PG_WP_BIND, sess->gui->output, 0);
	switch(output_type) {
		case PG_WIDGET_TEXTBOX:
			/* set the textbox to autoscroll, append HTML */
			pgSetWidget(0, PG_WP_AUTOSCROLL, 1,
				PG_WP_SIDE, PG_S_ALL, PG_WP_TEXTFORMAT,
				pgPlusHTML, 0);
			break;
		case PG_WIDGET_TERMINAL:
			/* make the terminal widget pass focus to the input */
			pgBind(0, PG_WE_DATA, evtPassFocus,
					(void*)sess->gui->input);
			pgSetWidget(0, PG_WP_SIDE, PG_S_ALL,
					PG_WP_AUTOSCROLL, 1,
					PG_WP_THOBJ, ircterm, 0);
			if(prefs.max_lines)
				pgSetWidget(0, PG_WP_LINES, prefs.max_lines, 0);
			/* hide cursor */
			pgWriteData(0, pgFromMemory("\e[?25l", 6));
			break;
	}

	if (!sess->server->front_session)
	{
		pgFocus(sess->gui->input);
		sess->server->front_session = sess;
	}
	if (!current_tab)
		current_tab = sess;

	if (done_intro)
		return;
	done_intro = 1;

	snprintf (buf, sizeof (buf),	
				"\n"
				" \017xchat \00310"VERSION"\n"
				" \017Running on \00310%s \017glib \00310%d.%d.%d\n"
				" \017This binary compiled \00310"__DATE__"\017\n",
				get_cpu_str(), 
				glib_major_version, glib_minor_version, glib_micro_version);
	fe_print_text (sess, buf);

	strcpy (buf, "\n\nCompiled in Features\0032:\017 ");
#ifdef USE_PERL
	strcat (buf, "Perl ");
#endif
#ifdef USE_PYTHON
	strcat (buf, "Python ");
#endif
#ifdef USE_PLUGIN
	strcat (buf, "Plugin ");
#endif
#ifdef ENABLE_NLS
	strcat (buf, "NLS ");
#endif
#ifdef USE_TRANS
	strcat (buf, "Trans ");
#endif
#ifdef USE_HEBREW
	strcat (buf, "Hebrew ");
#endif
#ifdef USE_OPENSSL
	strcat (buf, "OpenSSL ");
#endif
#ifdef SOCKS
	strcat (buf, "Socks5 ");
#endif
#ifdef HAVE_ICONV
	strcat (buf, "JCode ");
#endif
#ifdef USE_IPV6
	strcat (buf, "IPv6 ");
#endif
	strcat (buf, "\n\n");
	fe_print_text (sess, buf);
}

void
fe_timeout_remove (int tag)
{
	timerevent *te;
	GSList *list;

	list = tmr_list;
	while (list)
	{
		te = (timerevent *) list->data;
		if (te->tag == tag)
		{
			tmr_list = g_slist_remove (tmr_list, te);
			free (te);
			return;
		}
		list = list->next;
	}
}

int
fe_timeout_add (int interval, void *callback, void *userdata)
{
	struct timeval now;
	timerevent *te = malloc (sizeof (timerevent));

	tmr_list_count++;  /* this overflows at 2.2Billion, who cares!! */

	te->tag = tmr_list_count;
	te->interval = interval;
	te->callback = callback;
	te->userdata = userdata;

	gettimeofday (&now, NULL);
	te->next_call = now.tv_sec * 1000 + (now.tv_usec / 1000) + te->interval;

	tmr_list = g_slist_prepend (tmr_list, te);

	return te->tag;
}

void
fe_input_remove (int tag)
{
	socketevent *se;
	GSList *list;

	list = se_list;
	while (list)
	{
		se = (socketevent *) list->data;
		if (se->tag == tag)
		{
			se_list = g_slist_remove (se_list, se);
			free (se);
			return;
		}
		list = list->next;
	}
}

int
fe_input_add (int sok, int rread, int wwrite, int eexcept, void *func,
				  void *data)
{
	socketevent *se = malloc (sizeof (socketevent));

	se_list_count++;  /* this overflows at 2.2Billion, who cares!! */

	se->tag = se_list_count;
	se->sok = sok;
	se->rread = rread;
	se->wwrite = wwrite;
	se->eexcept = eexcept;
	se->callback = func;
	se->userdata = data;
	se_list = g_slist_prepend (se_list, se);

	return se->tag;
}

int
fe_args (int argc, char *argv[])
{
	pgInit(argc, argv);
	pgEmptyString=pgNewString("");
	pgPlusHTML=pgNewString("+HTML");
	if (argc > 1)
	{
		if (!strcasecmp (argv[1], "--version") || !strcasecmp (argv[1], "-v"))
		{
			puts (VERSION);
			return 0;
		}
		if (!strcasecmp (argv[1], "--textbox"))
			output_type=PG_WIDGET_TEXTBOX;
		if (!strcasecmp (argv[1], "--terminal"))
			output_type=PG_WIDGET_TERMINAL;
	}
	return 1;
}

void
fe_init (void)
{
	se_list = 0;
	se_list_count = 0;
	tmr_list = 0;
	tmr_list_count = 0;
	prefs.autosave = 0;
	prefs.use_server_tab = 0;
	prefs.autodialog = 0;
	prefs.lagometer = 0;
	prefs.skipserverlist = 1;
	palette_load();
}

static int
selectwrapper (int nfds, fd_set *readfds, fd_set *writefds,
		fd_set *exceptfds, struct timeval *timeout);
static void
afterselect(int result, fd_set *rfds);

void
fe_main (void)
{
#ifdef ENABLE_NLS
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);
#endif
	pgCustomizeSelect(selectwrapper, afterselect);
	pgEventLoop();
}

static fd_set *fe_pg_rfds, *fe_pg_wfds, *fe_pg_efds;

static
int selectwrapper (int n, fd_set *readfds, fd_set *writefds,
		fd_set *exceptfds, struct timeval *timeout)
{
	static fd_set rfds, wfds, efds;
	struct timeval now;
	socketevent *se;
	timerevent *te;
	GSList *list;
	guint64 shortest, delay;

	if(readfds)
		fe_pg_rfds=readfds;
	else
	{
		fe_pg_rfds=&rfds;
		FD_ZERO (&rfds);
	}
	if(writefds)
		fe_pg_wfds=writefds;
	else
	{
		fe_pg_wfds=&wfds;
		FD_ZERO (&wfds);
	}
	if(exceptfds)
		fe_pg_efds=exceptfds;
	else
	{
		fe_pg_efds=&efds;
		FD_ZERO (&efds);
	}

	list = se_list;
	while (list)
	{
		se = (socketevent *) list->data;
		if (se->rread)
		{
			FD_SET (se->sok, fe_pg_rfds);
			if(n<=se->sok)
				n=se->sok+1;
		}
		if (se->wwrite)
		{
			FD_SET (se->sok, fe_pg_wfds);
			if(n<=se->sok)
				n=se->sok+1;
		}
		if (se->eexcept)
		{
			FD_SET (se->sok, fe_pg_efds);
			if(n<=se->sok)
				n=se->sok+1;
		}
		list = list->next;
	}

	/* find the shortest timeout event */
	if(tmr_list)
	{
		gettimeofday (&now, NULL);

		shortest = now.tv_sec * 1000 + now.tv_usec / 1000 +
			23*60*60*1000;	/* we wait a max of 23 hours */
		list = tmr_list;
		while (list)
		{
			te = (timerevent *) list->data;
			if (te->next_call < shortest)
				shortest = te->next_call;
			list = list->next;
		}
		delay = shortest - ((now.tv_sec * 1000) + (now.tv_usec / 1000));
		if(delay<0)
			delay=0;
		now.tv_sec = delay / 1000;
		now.tv_usec = (delay % 1000) * 1000;
		if(timeout)
		{
			if(now.tv_sec<timeout->tv_sec ||
					(now.tv_sec==timeout->tv_sec &&
					now.tv_usec<timeout->tv_usec))
				*timeout=now;
		}
		else
			timeout=&now;
	}

	return select (n, fe_pg_rfds, fe_pg_wfds, fe_pg_efds, timeout);
}

static void
afterselect(int result, fd_set *rfds)
{
	GSList *list;
	socketevent *se;
	timerevent *te;
	struct timeval now;
	static int last=0;

	/* these should already be the same */
	fe_pg_rfds=rfds;

	/* set all checked flags to false */
	list = se_list;
	while (list)
	{
		se = (socketevent *) list->data;
		se->checked = 0;
		list = list->next;
	}

	/* check all the socket callbacks */
	list = se_list;
	while (list)
	{
		se = (socketevent *) list->data;
		se->checked = 1;
		if (se->rread && FD_ISSET (se->sok, fe_pg_rfds))
		{
			se->callback (NULL, 0, se->userdata);
		} else if (se->wwrite && FD_ISSET (se->sok, fe_pg_wfds))
		{
			se->callback (NULL, 0, se->userdata);
		} else if (se->eexcept && FD_ISSET (se->sok, fe_pg_efds))
		{
			se->callback (NULL, 0, se->userdata);
		}
		list = se_list;
		if (list)
		{
			se = (socketevent *) list->data;
			while (se->checked)
			{
				list = list->next;
				if (!list)
					break;
				se = (socketevent *) list->data;
			}
		}
	}

	/* now check our list of timeout events, some might need to be called! */
	gettimeofday (&now, NULL);
	list = tmr_list;
	while (list)
	{
		te = (timerevent *) list->data;
		list = list->next;
		if (now.tv_sec < last)
			te->next_call -= 24*60*60*1000;
		if (now.tv_sec * 1000 + (now.tv_usec / 1000) >= te->next_call)
		{
			/* if the callback returns 0, it must be removed */
			if (te->callback (te->userdata) == 0)
			{
				fe_timeout_remove (te->tag);
			} else
			{
				te->next_call = now.tv_sec * 1000 + (now.tv_usec / 1000) + te->interval;
			}
		}
	}
	last=now.tv_sec;
}

void
fe_exit (void)
{
	pgExitEventLoop();
}

void
fe_new_server (struct server *serv)
{
	serv->gui = malloc (sizeof(struct server_gui));
}

void
fe_message (char *msg, int wait)
{
	if(wait)
		pgMessageDialog("X-Chat", msg, 0);
	else
	{
		pghandle text;

		pgRegisterApp(PG_APP_NORMAL, "X-Chat message", 0);
		pgBind(0, PG_WE_CLOSE, evtCloseMessage, NULL);
		pgNewWidget(PG_WIDGET_TEXTBOX, 0, 0);
		pgSetWidget(0, PG_WP_TEXT, text=pgNewString(msg), 0);
		pgDelete(text);
	}
}

void
fe_close_window (struct session *sess)
{
	int i;

	if(sess->gui->userlistinfo)
		sess->gui->userlistinfo=pgGetWidget(sess->gui->userlistinfo,PG_WP_TEXT);
	if(sess->gui->topic)
		sess->gui->topic=pgGetWidget(sess->gui->app, PG_WP_TEXT);
	for(i=0;i<sess->gui->buttons;i++)
		sess->gui->userbutton[i].h=
			pgGetWidget(sess->gui->userbutton[i].h, PG_WP_TEXT);
	for(i=0;i<sess->gui->users;i++)
		sess->gui->uhmap[i].handle=
			pgGetWidget(sess->gui->uhmap[i].handle, PG_WP_TEXT);
	pgDelete(sess->gui->app);
	for(i=0;i<sess->gui->buttons;i++)
		pgDelete(sess->gui->userbutton[i].h);
	if(sess->gui->userbutton)
		free(sess->gui->userbutton);
	for(i=0;i<sess->gui->users;i++)
		pgDelete(sess->gui->uhmap[i].handle);
	if(sess->gui->uhmap)
		free(sess->gui->uhmap);
	if(sess->gui->topic)
		pgDelete(sess->gui->topic);
	if(sess->gui->userlistinfo)
		pgDelete(sess->gui->userlistinfo);
	kill_session_callback (sess);
}

void
fe_beep (void)
{
	pgDriverMessage(PGDM_SOUNDFX, PG_SND_BEEP);
}

void
fe_add_rawlog (struct server *serv, char *text, int outbound)
{
}

static int
setTopic(struct pgEvent *evt)
{
	struct session *sess=evt->extra;
	pghandle topichandle;
	char *topic, *cmd;

	topichandle=pgGetWidget(evt->from, PG_WP_TEXT);
	if(topichandle)
	{
		topic=pgGetString(topichandle);
		cmd=malloc(strlen(topic)+8);
		if(cmd)
		{
			sprintf(cmd, "/topic %s", topic);
			handle_command (cmd, sess, FALSE, FALSE);
			free(cmd);
		}
	}
	return 0;
}

void
fe_set_topic (struct session *sess, char *topic)
{
	pghandle str;

	if(!sess->gui->topic)
	{
		sess->gui->topic = pgNewWidget(PG_WIDGET_FIELD, PG_DERIVE_BEFORE,
				sess->gui->input);
		pgSetWidget(0, PG_WP_SIDE, PG_S_TOP, 0);
		pgBind(0, PG_WE_ACTIVATE, setTopic, sess);
	}
	str=pgNewString(topic);
	pgSetWidget(sess->gui->topic, PG_WP_TEXT, str, 0);
	pgDelete(str);
}
void
fe_cleanup (void)
{
	pgDelete(pgPlusHTML);
	pgDelete(pgEmptyString);
}
void
fe_set_hilight (struct session *sess)
{
}
void
fe_update_mode_buttons (struct session *sess, char mode, char sign)
{
}
void
fe_update_channel_key (struct session *sess)
{
}
void
fe_update_channel_limit (struct session *sess)
{
}
int
fe_is_chanwindow (struct server *serv)
{
	return 0;
}

void
fe_add_chan_list (struct server *serv, char *chan, char *users, char *topic)
{
}
void
fe_chan_list_end (struct server *serv)
{
}
int
fe_is_banwindow (struct session *sess)
{
	return 0;
}
void
fe_add_ban_list (struct session *sess, char *chan, char *users, char *topic)
{
}               
void
fe_ban_list_end (struct session *sess)
{
}
void
fe_notify_update (char *name)
{
}
void
fe_text_clear (struct session *sess)
{
}
void
fe_progressbar_start (struct session *sess)
{
}
void
fe_progressbar_end (struct session *sess)
{
}

void
fe_userlist_insert (struct session *sess, struct User *newuser, int row)
{
	int i;
	pghandle item;

	sess->gui->uhmap=realloc(sess->gui->uhmap,
			++sess->gui->users*sizeof(struct uhmapping));
	if(row==-1)
		row=sess->gui->users-1;
	else
		for(i=sess->gui->users-1;i>row;i--)
			sess->gui->uhmap[i]=sess->gui->uhmap[i-1];
	item=pgNewWidget(PG_WIDGET_LISTITEM, row?PG_DERIVE_AFTER:
			PG_DERIVE_INSIDE, row?sess->gui->uhmap[row-1].handle:
			sess->gui->userlist);
	pgSetWidget(0, PG_WP_EXTDEVENTS, pgGetWidget(0, PG_WP_EXTDEVENTS) &
			~PG_EXEV_EXCLUSIVE, 0);
	pgReplaceTextFmt(0, "%c%s", newuser->prefix, newuser->nick);
	sess->gui->uhmap[row].user=newuser;
	sess->gui->uhmap[row].handle=item;
}
void
fe_userlist_remove (struct session *sess, struct User *user)
{
	int i;

	for(i=0;i<sess->gui->users;i++)
		if(sess->gui->uhmap[i].user==user)
		{
			pgDelete(pgGetWidget(sess->gui->uhmap[i].handle,
						PG_WP_TEXT));
			pgDelete(sess->gui->uhmap[i].handle);
			--sess->gui->users;
			while(i<sess->gui->users)
			{
				sess->gui->uhmap[i]=sess->gui->uhmap[i+1];
				i++;
			}
			sess->gui->uhmap=realloc(sess->gui->uhmap,
				sess->gui->users*sizeof(struct uhmapping));
			break;
		}
}
void
fe_userlist_move (struct session *sess, struct User *user, int new_row)
{
	struct uhmapping tmp;
	int i;

	if(new_row==-1)
		new_row=sess->gui->users-1;
	for(i=0;i<sess->gui->users;i++)
		if(sess->gui->uhmap[i].user==user)
		{
			tmp=sess->gui->uhmap[i];
			break;
		}
	if(i==sess->gui->users)
		return;		/* should never happen */
	while(i<new_row)
	{
		sess->gui->uhmap[i]=sess->gui->uhmap[i+1];
		i++;
	}
	while(i>new_row)
	{
		sess->gui->uhmap[i]=sess->gui->uhmap[i-1];
		i--;
	}
	sess->gui->uhmap[new_row]=tmp;
	pgReplaceTextFmt(tmp.handle, "%c%s", tmp.user->prefix, tmp.user->nick);
	pgAttachWidget(new_row?sess->gui->uhmap[new_row-1].handle:
			sess->gui->userlist, new_row?PG_DERIVE_AFTER:
			PG_DERIVE_INSIDE, tmp.handle);
}
void
fe_userlist_numbers (struct session *sess)
{
	pgReplaceTextFmt(sess->gui->userlistinfo, "User List - @%d +%d %d",
			sess->ops, sess->voices, sess->total);
}
void
fe_userlist_clear (struct session *sess)
{
	int i=sess->gui->users;
	pghandle h;

	if(!sess->gui->uhmap)
		return;
	while(i--)
	{
		h=pgGetWidget(sess->gui->uhmap[i].handle, PG_WP_TEXT);
		if(h)
			pgDelete(h);
		pgDelete(sess->gui->uhmap[i].handle);
	}
	sess->gui->users=0;
	free(sess->gui->uhmap);
	sess->gui->uhmap=NULL;
}
void
fe_dcc_update_recv_win (void)
{
}
void
fe_dcc_update_send_win (void)
{
}
void
fe_dcc_update_chat_win (void)
{
}
void
fe_dcc_update_send (struct DCC *dcc)
{
}
void
fe_dcc_update_recv (struct DCC *dcc)
{
}
void
fe_clear_channel (struct session *sess)
{
}
void
fe_session_callback (struct session *sess)
{
}
void
fe_server_callback (struct server *serv)
{
}
void
fe_checkurl (char *text)
{
}
void
fe_pluginlist_update (void)
{
}

static int
evtUserButton (struct pgEvent *evt)
{
	struct session *sess=evt->extra;
	char *cmdtemplate=NULL, *p, *cmd;
	int i, size, didit=0;

	for(i=0;i<sess->gui->buttons;i++)
		if(sess->gui->userbutton[i].h==evt->from)
		{
			/* I know about the warning, but otherwise strcpy()
			 * gives that warning a _lot_ below */
			cmdtemplate=sess->gui->userbutton[i].cmd;
			break;
		}
	if(!cmdtemplate)
		return 0;
	size=strlen(cmdtemplate);
	if(p=strstr(cmdtemplate, "%s"))
	{	/* one nick at a time */
		cmd=malloc(size+NICKLEN);	/* wasting at least 2 bytes */
		memcpy(cmd, cmdtemplate, size=p-cmdtemplate);
		p+=2;
		for(i=0;i<sess->gui->users;i++)
		{
			if(pgGetWidget(sess->gui->uhmap[i].handle, PG_WP_ON))
			{
				strcpy(cmd+size,sess->gui->uhmap[i].user->nick);
				strcat(cmd+size, p);
				handle_command (cmd, sess, FALSE, FALSE);
				didit=1;
			}
		}
		if(!didit)
		{
			strcpy(cmd+size, p);
			handle_command (cmd, sess, FALSE, FALSE);
		}
		free(cmd);
	}
	else if(p=strstr(cmdtemplate, "%a"))
	{	/* multinick command */
		/* wasting at least sess->gui->users+1 bytes */
		cmd=malloc(size+sess->gui->users*NICKLEN);
		memcpy(cmd, cmdtemplate, size=p-cmdtemplate);
		p+=2;
		for(i=0;i<sess->gui->users;i++)
		{
			if(pgGetWidget(sess->gui->uhmap[i].handle, PG_WP_ON))
			{
				if(didit)
					cmd[size++]=' ';
				strcpy(cmd+size,sess->gui->uhmap[i].user->nick);
				size+=strlen(cmd+size);
				didit=1;
			}
		}
		strcpy(cmd+size, p);
		handle_command (cmd, sess, FALSE, FALSE);
		free(cmd);
	}
	else
		handle_command (cmdtemplate, sess, FALSE, FALSE);
	return 1;
}
void
fe_buttons_update (struct session *sess)
{
	GSList *list;
	struct popup *pop;
	pghandle h;

	if(sess->gui->userbutton)
	{
		while(--sess->gui->buttons>=0)
		{
			h=sess->gui->userbutton[sess->gui->buttons].h;
			pgDelete(pgGetWidget(h, PG_WP_TEXT));
			pgDelete(h);
		}
		free(sess->gui->userbutton);
		sess->gui->userbutton=NULL;
	}
	sess->gui->buttons=0;
	list=button_list;
	while(list)
	{
		pop = (struct popup *) list->data;
		if(pop->cmd[0])
		{
			sess->gui->userbutton=realloc(sess->gui->userbutton,
					sizeof(struct userbut) *
					++sess->gui->buttons);
			sess->gui->userbutton[sess->gui->buttons-1].cmd=
				pop->cmd;
			sess->gui->userbutton[sess->gui->buttons-1].h=
				pgNewWidget(PG_WIDGET_BUTTON, 
					    sess->gui->buttons-1?0:
					    PG_DERIVE_INSIDE,
					    sess->gui->buttons-1?0:
					    sess->gui->buttonbox);
			pgSetWidget(0, PG_WP_TEXT, pgNewString(pop->name),
					PG_WP_SIDE, PG_S_BOTTOM, 0);
			pgBind(0, PG_WE_ACTIVATE, evtUserButton, sess);
		}
		list=list->next;
	}
}
void
fe_dlgbuttons_update (struct session *sess)
{
}
void
fe_dcc_send_filereq (struct session *sess, char *nick)
{
}
void
fe_set_channel (struct session *sess)
{
}
void
fe_set_title (struct session *sess)
{
	int type=sess->type;

	if(sess->server->connected == FALSE && sess->type != SESS_DIALOG)
		type = SESS_SHELL;	/* force default */

	switch (type)
	{
		case SESS_DIALOG:
			pgReplaceTextFmt(sess->gui->app,
					"%s @ %s : X-Chat",
					sess->channel, sess->server->servername);
			break;
		case SESS_SERVER:
			pgReplaceTextFmt(sess->gui->app, "%s @ %s : X-Chat",
					sess->server->nick, sess->server->servername);
			break;
		case SESS_CHANNEL:
			pgReplaceTextFmt(sess->gui->app,
					"%s @ %s : X-Chat",
					 sess->server->nick,
					 sess->channel);
			break;
		case SESS_NOTICES:
		case SESS_SNOTICES:
			pgReplaceTextFmt(sess->gui->app,
					"%s @ %s (notices) : X-Chat",
					sess->server->nick, sess->server->servername);
			break;
		default:
			pgReplaceText(sess->gui->app, "X-Chat ["VERSION"]");
	}
}
void
fe_set_nonchannel (struct session *sess, int state)
{
}
void
fe_set_nick (struct server *serv, char *newnick)
{
	strcpy (serv->nick, newnick);
}
void
fe_change_nick (struct server *serv, char *nick, char *newnick)
{
	struct session *sess = find_dialog (serv, nick);
	if (sess)
	{
		safe_strcpy (sess->channel, newnick, CHANLEN);
		fe_set_title (sess);
	}
}
void
fe_ignore_update (int level)
{
}
void
fe_dcc_open_recv_win (int passive)
{
}
void
fe_dcc_open_send_win (int passive)
{
}
void
fe_dcc_open_chat_win (int passive)
{
}
int
fe_is_confmode (struct session *sess)
{
	return 0;
}
int
fe_is_beep (struct session *sess)
{
	return 0;
}

void
fe_userlist_hide (session * sess)
{
}
void
fe_lastlog (session * sess, session * lastlog_sess, char *sstr)
{
}
void
fe_set_lag (server * serv, int lag)
{
}
void
fe_set_throttle (server * serv)
{
}
void
fe_set_away (server *serv)
{
}
int
fe_open_serverlist (session *sess, int auto_connect, int dont_show)
{
	return FALSE;
}
