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
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "fe-gtk.h"
#include "../common/xchat.h"
#include "../common/xchatc.h"
#include "../common/cfgfiles.h"
#include "../common/outbound.h"
#include "../common/ignore.h"
#include "../common/fe.h"
#include "../common/util.h"
#include "../common/plugin.h"
#include "../common/perlc.h"
#include "xtext.h"
#include "about.h"
#include "banlist.h"
#include "chanlist.h"
#include "editlist.h"
#include "fkeys.h"
#include "gtkutil.h"
#include "maingui.h"
#include "notifygui.h"
#include "rawlog.h"
#include "palette.h"
#include "search.h"
#include "settings.h"
#include "textgui.h"
#include "usermenu.h"
#include "urlgrab.h"
#include "menu.h"

#ifdef USE_ZVT
#include <zvt/zvtterm.h>
#endif

extern void module_glist (struct session *sess);

static GSList *submenu_list;


void
goto_url (char *url)
{
#ifdef USE_GNOME
	gnome_url_show (url);
#else
	char tbuf[512];
#ifdef WIN32
	snprintf (tbuf, sizeof (tbuf), "start %s", url);
#else
	snprintf (tbuf, sizeof (tbuf), "netscape -remote 'openURL(%s)'", url);
#endif
	xchat_exec (tbuf);
#endif
}

/* execute a userlistbutton/popupmenu command */

static void
nick_command (session * sess, char *cmd)
{
	if (*cmd == '!')
		xchat_exec (cmd + 1);
	else
		handle_command (cmd, sess, FALSE, FALSE);
}

/* fill in the %a %s %n etc and execute the command */

void
nick_command_parse (session *sess, char *cmd, char *nick, char *allnick)
{
	char *buf;
	char *host = _("Host unknown");
	struct User *user;

	if (sess->type == SESS_DIALOG)
	{
		buf = gtk_entry_get_text (GTK_ENTRY (sess->gui->topicgad));
		buf = strchr (buf, '@');
		if (buf)
			host = buf + 1;
	} else
	{
		user = find_name (sess, nick);
		if (user && user->hostname)
			host = strchr (user->hostname, '@') + 1;
	}

	/* this can't overflow, since popup->cmd is only 256 */
	buf = malloc (strlen (cmd) + strlen (nick) + strlen (allnick) + 512);

	auto_insert (buf, cmd, 0, 0, allnick, sess->channel, "", host,
						sess->server->nick, nick);

	nick_command (sess, buf);

	free (buf);
}

/* userlist button has been clicked */

void
userlist_button_cb (GtkWidget * button, char *cmd)
{
	int nicks, using_allnicks = FALSE;
	char *nick = NULL, *allnicks;
	struct User *user;
	GList *sel_list;
	session *sess;

	/* this is set in maingui.c userlist_button() */
	sess = gtk_object_get_user_data (GTK_OBJECT (button));

	if (strstr (cmd, "%a"))
		using_allnicks = TRUE;

	/* count number of selected rows */
	sel_list = (GTK_CLIST (sess->gui->namelistgad))->selection;
	nicks = g_list_length (sel_list);

	if (nicks == 0)
	{
		nick_command_parse (sess, cmd, "", "");
		return;
	}

	/* create "allnicks" string */
	allnicks = malloc (65 * nicks);
	*allnicks = 0;

	nicks = 0;
	while (sel_list)
	{
		user = gtk_clist_get_row_data (GTK_CLIST (sess->gui->namelistgad),
												 (gint) sel_list->data);
		if (!nick)
			nick = user->nick;
		if (nicks > 0)
			strcat (allnicks, " ");
		strcat (allnicks, user->nick);
		sel_list = sel_list->next;
		nicks++;

		/* if not using "%a", execute the command once for each nickname */
		if (!using_allnicks)
			nick_command_parse (sess, cmd, user->nick, "");

	}

	if (using_allnicks)
	{
		if (!nick)
			nick = "";
		nick_command_parse (sess, cmd, nick, allnicks);
	}

	free (allnicks);
}

/* a popup-menu-item has been selected */

static void
popup_menu_cb (GtkWidget * item, char *cmd)
{
	char *nick;

	/* the userdata is set in menu_quick_item() */
	nick = gtk_object_get_user_data (GTK_OBJECT (item));

	if (!menu_sess)	/* for url grabber window */
		nick_command_parse (sess_list->data, cmd, nick, nick);
	else
		nick_command_parse (menu_sess, cmd, nick, nick);
}

static void
menu_toggle_item (char *label, GtkWidget *menu, void *callback, void *userdata,
						int state)
{
	GtkWidget *item;

	item = gtk_check_menu_item_new_with_label (label);
	/* Always show checkbox, not just when it is moused over. */
	gtk_check_menu_item_set_show_toggle ((GtkCheckMenuItem*)item, TRUE);
	gtk_check_menu_item_set_state ((GtkCheckMenuItem*)item, state);
	gtk_menu_append (GTK_MENU (menu), item);
	gtk_signal_connect (GTK_OBJECT (item), "activate",
								GTK_SIGNAL_FUNC (callback), userdata);
	gtk_widget_show (item);
}

static GtkWidget *
menu_quick_item (char *cmd, char *label, GtkWidget * menu, int flags,
					  gpointer userdata)
{
	GtkWidget *item;
	if (!label)
		item = gtk_menu_item_new ();
	else
		item = gtk_menu_item_new_with_label (label);
	gtk_menu_append (GTK_MENU (menu), item);
	gtk_object_set_user_data (GTK_OBJECT (item), userdata);
	if (cmd)
		gtk_signal_connect (GTK_OBJECT (item), "activate",
								  GTK_SIGNAL_FUNC (popup_menu_cb), cmd);
	if (flags & (1 << 0))
		gtk_widget_set_sensitive (GTK_WIDGET (item), FALSE);
	gtk_widget_show (item);

	return item;
}

void
menu_quick_item_with_callback (void *callback, char *label, GtkWidget * menu,
										 void *arg)
{
	GtkWidget *item;

	item = gtk_menu_item_new_with_label (label);
	gtk_menu_append (GTK_MENU (menu), item);
	gtk_signal_connect (GTK_OBJECT (item), "activate",
							  GTK_SIGNAL_FUNC (callback), arg);
	gtk_widget_show (item);
}

static GtkWidget *
menu_quick_sub (char *name, GtkWidget * menu)
{
	GtkWidget *sub_menu;
	GtkWidget *sub_item;

	if (!name)
		return menu;

	/* Code to add a submenu */
	sub_menu = gtk_menu_new ();
	sub_item = gtk_menu_item_new_with_label (name);
	gtk_menu_append (GTK_MENU (menu), sub_item);
	gtk_widget_show (sub_item);
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (sub_item), sub_menu);

	/* We create a new element in the list */
	submenu_list = g_slist_prepend (submenu_list, sub_menu);
	return (sub_menu);
}

static GtkWidget *
menu_quick_endsub ()
{
	/* Just delete the first element in the linked list pointed to by first */
	if (submenu_list)
		submenu_list = g_slist_remove (submenu_list, submenu_list->data);

	if (submenu_list)
		return (submenu_list->data);
	else
		return NULL;
}

static void
toggle_cb (GtkWidget *item, char *pref_name)
{
	char buf[256];

	if (GTK_CHECK_MENU_ITEM (item)->active)
		snprintf (buf, sizeof (buf), "/set %s 1", pref_name);
	else
		snprintf (buf, sizeof (buf), "/set %s 0", pref_name);

	handle_command (buf, menu_sess, FALSE, FALSE);
}

/* append items to "menu" using the (struct popup*) list provided */

void
menu_create (GtkWidget *menu, GSList *list, char *target)
{
	struct popup *pop;
	GtkWidget *tempmenu = menu;

	submenu_list = g_slist_prepend (0, menu);
	while (list)
	{
		pop = (struct popup *) list->data;
		if (!strncasecmp (pop->name, "SUB", 3))
			tempmenu = menu_quick_sub (pop->cmd, tempmenu);
		else if (!strncasecmp (pop->name, "TOGGLE", 6))
		{
			menu_toggle_item (pop->name + 7, tempmenu, toggle_cb, pop->cmd,
									cfg_get_bool (pop->cmd));
		}
		else if (!strncasecmp (pop->name, "ENDSUB", 6))
		{
			if (tempmenu != menu)
				tempmenu = menu_quick_endsub ();
			/* If we get here and tempmenu equals menu that means we havent got any submenus to exit from */
		} else if (!strncasecmp (pop->name, "SEP", 3))
			menu_quick_item (0, 0, tempmenu, 1, 0);
		else
			menu_quick_item (pop->cmd, pop->name, tempmenu, 0, target);

		list = list->next;

	}

	/* Let's clean up the linked list from mem */
	while (submenu_list)
		submenu_list = g_slist_remove (submenu_list, submenu_list->data);
}

static void
menu_destroy (GtkObject *object, gpointer unused)
{
	gtk_widget_destroy (GTK_WIDGET (object));
}

static void
menu_popup (GtkWidget *menu, GdkEventButton *event)
{
	gtk_signal_connect (GTK_OBJECT (menu), "selection-done",
							  GTK_SIGNAL_FUNC (menu_destroy), NULL);
	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,
						 event->button, event->time);
}

static char *str_copy = 0;		/* for all pop-up menus */

void
menu_nickmenu (session *sess, GdkEventButton *event, char *nick)
{
	char buf[256];
	struct User *user;
	GtkWidget *wid, *submenu, *menu = gtk_menu_new ();

	if (str_copy)
		free (str_copy);
	str_copy = strdup (nick);

	submenu_list = 0;	/* first time though, might not be 0 */

	user = find_name_global (menu_sess->server, nick);
	if (user)
	{
		submenu = menu_quick_sub (str_copy, menu);

		sprintf (buf, _("User: %s"),
					user->hostname ? user->hostname : _("Unknown"));
		wid = menu_quick_item (0, buf, submenu, 0, 0);

		sprintf (buf, _("Country: %s"),
					user->hostname ? country(user->hostname) : _("Unknown"));
		wid = menu_quick_item (0, buf, submenu, 0, 0);

		sprintf (buf, _("Realname: %s"),
					user->realname ? user->realname : _("Unknown"));
		wid = menu_quick_item (0, buf, submenu, 0, 0);

		sprintf (buf, _("Server: %s"),
					user->servername ? user->servername : _("Unknown"));
		wid = menu_quick_item (0, buf, submenu, 0, 0);

		sprintf (buf, _("Last Msg: %s"),
					user->lasttalk ? ctime (&(user->lasttalk)) : _("Unknown"));
		if (user->lasttalk)
			buf[strlen (buf) - 1] = 0;
		wid = menu_quick_item (0, buf, submenu, 0, 0);

		gtk_label_set_justify (GTK_LABEL (GTK_BIN (wid)->child), GTK_JUSTIFY_LEFT);
		menu_quick_endsub ();
		menu_quick_item (0, 0, menu, 1, 0);
	}

	menu_create (menu, popup_list, str_copy);
	menu_popup (menu, event);
}

void
menu_showhide (void)
{
	session *sess;
	GSList *list;
	GtkWidget *menu;
#ifdef USE_GNOME
	GtkWidget *win;
	int resize_done = FALSE;	/* resize main_window only once */
#endif

	list = sess_list;
	while (list)
	{
		sess = list->data;

		if (sess->is_tab)
		{
			menu = main_menu_bar;
		} else
		{
			if (!sess->gui->menu)
				goto cont;
			menu = sess->gui->menu->parent;	/* really the handlebox */
		}

		if (prefs.hidemenu)
		{
			gtk_widget_hide (menu);
#ifdef USE_GNOME
			/* needed to hide the menubar properly under gnome. */
			if (!resize_done || !sess->is_tab)
			{
				win = gtk_widget_get_toplevel (menu);
				gdk_window_resize (win->window, win->allocation.width-1, win->allocation.height);
				gdk_window_resize (win->window, win->allocation.width, win->allocation.height);
				if (sess->is_tab)
					resize_done = TRUE;
			}
#endif
		} else
		{
			gtk_widget_show (menu);
		}
cont:
		list = list->next;
	}
}

static void
menu_middle_cb (GtkWidget *item, int n)
{
	switch (n)
	{
	case 0:
		if (prefs.hidemenu)
		{
			prefs.hidemenu = 0;
			menu_showhide ();
		} else
		{
			prefs.hidemenu = 1;
			menu_showhide ();
		}
		break;
	case 1:
		maingui_showhide_topic (menu_sess);
		break;
	case 2:
		userlist_hide (0, menu_sess);
		break;
	}
}

void
menu_middlemenu (session *sess, GdkEventButton *event)
{
	GtkWidget *menu;

	menu = createmenus (0, sess, 0);

	menu_toggle_item (_("Menu Bar"), menu, (void*)menu_middle_cb, 0, !prefs.hidemenu);
	menu_toggle_item (_("Topic Bar"), menu, (void*)menu_middle_cb, (void*)1, prefs.topicbar);
	menu_toggle_item (_("User List"), menu, (void*)menu_middle_cb, (void*)2, !prefs.hideuserlist);

	menu_popup (menu, event);
}

void
menu_urlmenu (GdkEventButton *event, char *url)
{
	GtkWidget *menu;
	int i;
	char *tmp;

	if (str_copy)
		free (str_copy);

	/* replace all commas with %2c */
	str_copy = malloc ((strlen (url) * 3) + 1);
	i = 0;
	while (*url)
	{
		if (*url == ',')
		{
			str_copy[i++] = '%';
			str_copy[i++] = '2';
			str_copy[i] = 'c';
#ifdef WIN32
		} else if (*url == '\\')
		{
			str_copy[i++] = '\\';
			str_copy[i] = '\\';
#endif
		} else
			str_copy[i] = *url;
		i++;
		url++;
	}
	str_copy[i] = 0;

	menu = gtk_menu_new ();
	if (strlen (str_copy) > 51)
	{
		tmp = strdup (str_copy);
		tmp[47] = tmp[48] = tmp[49] = '.';
		tmp[50] = 0;
		menu_quick_item (0, tmp, menu, 1, 0);
		free (tmp);
	} else
	{
		menu_quick_item (0, str_copy, menu, 1, 0);
	}
	menu_quick_item (0, 0, menu, 1, 0);

	menu_create (menu, urlhandler_list, str_copy);
	menu_popup (menu, event);
}

static void
menu_chan_cycle (GtkWidget * menu, char *chan)
{
	char tbuf[256];

	if (menu_sess)
	{
		snprintf (tbuf, sizeof tbuf, "PART %s\r\nJOIN %s\r\n", chan, chan);
		tcp_send (menu_sess->server, tbuf);
	}
}

static void
menu_chan_part (GtkWidget * menu, char *chan)
{
	char tbuf[256];

	if (menu_sess)
	{
		snprintf (tbuf, sizeof tbuf, "PART %s\r\n", chan);
		tcp_send (menu_sess->server, tbuf);
	}
}

static void
menu_chan_join (GtkWidget * menu, char *chan)
{
	char tbuf[256];

	if (menu_sess)
	{
		snprintf (tbuf, sizeof tbuf, "JOIN %s\r\n", chan);
		tcp_send (menu_sess->server, tbuf);
	}
}

void
menu_chanmenu (struct session *sess, GdkEventButton * event, char *chan)
{
	GtkWidget *menu;
	GSList *list = sess_list;
	int is_joined = FALSE;
	struct session *s;

	while (list)
	{
		s = (struct session *) list->data;
		if (s->server == sess->server)
		{
			if (!strcasecmp (chan, s->channel))
			{
				is_joined = TRUE;
				break;
			}
		}
		list = list->next;
	}

	if (str_copy)
		free (str_copy);
	str_copy = strdup (chan);

	menu = gtk_menu_new ();

	menu_quick_item (0, chan, menu, 1, str_copy);
	menu_quick_item (0, 0, menu, 1, str_copy);

	if (!is_joined)
		menu_quick_item_with_callback (menu_chan_join, _("Join Channel"), menu,
												 str_copy);
	else
	{
		menu_quick_item_with_callback (menu_chan_part, _("Part Channel"), menu,
												 str_copy);
		menu_quick_item_with_callback (menu_chan_cycle, _("Cycle Channel"), menu,
												 str_copy);
	}

	menu_popup (menu, event);
}

static void
menu_open_server_list (GtkWidget *wid, gpointer none)
{
	fe_open_serverlist (menu_sess, FALSE, FALSE);
}

static void
menu_settings (GtkWidget * wid, gpointer none)
{
	settings_opengui (menu_sess);
}

#ifdef USE_ZVT

static int
shell_exit (ZvtTerm * term, struct session *sess)
{
	gtk_widget_destroy (sess->gui->window);
	return 0;
}

static void
menu_shell_title (ZvtTerm * term, int type, char *newtitle, session * sess)
{
	switch (type)
	{
	case VTTITLE_WINDOW:
	case VTTITLE_WINDOWICON:
		gtk_entry_set_text (GTK_ENTRY (sess->gui->topicgad), newtitle);
	}
}

#ifdef USE_XLIB
#include <gdk/gdkx.h>
#endif

/*                  30 31 32 33 34 35 36  37  30  31 32 33 34  35  36  37 */
static
	int color_conv[] =
	{ 1, 4, 3, 5, 2, 6, 10, 15, 14, 7, 9, 8, 12, 13, 11, 0, 18, 19 };

static gushort zvt_red[18];
static gushort zvt_grn[18];
static gushort zvt_blu[18];

void
menu_newshell_set_palette (session *sess)
{
	int i;

	for (i = 17;; i--)
	{
		zvt_red[i] = colors[color_conv[i]].red;
		zvt_grn[i] = colors[color_conv[i]].green;
		zvt_blu[i] = colors[color_conv[i]].blue;
		if (!i)
			break;
	}
	zvt_term_set_color_scheme ((ZvtTerm *) sess->gui->textgad,
										zvt_red, zvt_grn, zvt_blu);
}

static void
menu_newshell_tab (GtkWidget * wid, struct session *sess)
{
	int oldb = prefs.chanmodebuttons;
	int oldp = prefs.paned_userlist;
	int oldu = prefs.use_server_tab;
	char *shell, *name;
	char buf[16];
	ZvtTerm *zvt;

	prefs.chanmodebuttons = prefs.paned_userlist = prefs.use_server_tab = 0;
	sess = new_ircwindow (NULL, NULL, SESS_SHELL);
	prefs.chanmodebuttons = oldb;
	prefs.paned_userlist = oldp;
	prefs.use_server_tab = oldu;

	fe_set_title (sess);

#ifdef USE_GNOME
	shell = gnome_util_user_shell ();
#else
	shell = strdup (getenv ("SHELL"));
#endif
	name = strrchr (shell, '/');
	if (!name)
		name = shell;
	else
		name++;
	strcpy (sess->channel, name);
	fe_set_channel (sess);

	gtk_widget_destroy (sess->gui->textgad);
	gtk_widget_destroy (sess->gui->vscrollbar);

	sess->gui->textgad = zvt_term_new ();
	zvt = (ZvtTerm *) sess->gui->textgad;

	zvt_term_set_blink (zvt, FALSE);
	if (prefs.font_shell[0] == 0)
		zvt_term_set_fonts (zvt, font_normal, NULL);
	else
		zvt_term_set_font_name (zvt, prefs.font_shell);
	zvt_term_set_scrollback (zvt, 720);
	zvt_term_set_scroll_on_keystroke (zvt, TRUE);
	zvt_term_set_background (zvt, prefs.background, prefs.transparent, 0);
	gtk_container_add (GTK_CONTAINER (sess->gui->leftpane), GTK_WIDGET (zvt));
	gtk_widget_show (GTK_WIDGET (zvt));
	menu_newshell_set_palette (sess);

	sess->gui->vscrollbar = gtk_vscrollbar_new (zvt->adjustment);
	gtk_box_pack_start (GTK_BOX (sess->gui->leftpane), sess->gui->vscrollbar,
							  0, 0, 1);
	show_and_unfocus (sess->gui->vscrollbar);

	GTK_WIDGET_UNSET_FLAGS (sess->gui->topicgad, GTK_CAN_FOCUS);

	if (sess->gui->userlistbox)
	{
		gtk_widget_destroy (sess->gui->userlistbox);
		sess->gui->userlistbox = 0;
	}

	gtk_signal_connect (GTK_OBJECT (sess->gui->textgad), "child_died",
							  GTK_SIGNAL_FUNC (shell_exit), sess);
	gtk_signal_connect (GTK_OBJECT (sess->gui->textgad), "title_changed",
							  GTK_SIGNAL_FUNC (menu_shell_title), sess);
	gtk_widget_destroy (sess->gui->inputgad->parent);

	sess->gui->inputgad = 0;
	sess->gui->toolbox = 0;
	gtk_widget_grab_focus (sess->gui->textgad);

	switch (zvt_term_forkpty ((ZvtTerm *) sess->gui->textgad, 0))
	{
	case -1:
		break;

	case 0:
#ifdef USE_XLIB
		sprintf (buf, "%d",
					(int) GDK_WINDOW_XWINDOW (GTK_WIDGET (sess->gui->textgad)->
													  window));
#endif
#ifdef HAVE_SETENV				  /* solaris is lame */
		setenv ("WINDOWID", buf, 1);
		setenv ("COLORTERM", "zterm", 0);
#endif
		execl (shell, name, NULL);
		_exit (127);
	}

	if (main_window)
		gdk_window_set_back_pixmap (main_book->window, 0, 0);

#ifdef USE_GNOME
	g_free (shell);				/* FIXME: is this a race condition ???? */
#else
	free (shell);
#endif
}
#endif

static void
menu_newserver_window (GtkWidget * wid, gpointer none)
{
	int old = prefs.tabchannels;

	prefs.tabchannels = 0;
	new_ircwindow (NULL, NULL, SESS_SERVER);
	prefs.tabchannels = old;
}

static void
menu_newchannel_window (GtkWidget * wid, gpointer none)
{
	int old = prefs.tabchannels;

	prefs.tabchannels = 0;
	new_ircwindow (menu_sess->server, NULL, SESS_CHANNEL);
	prefs.tabchannels = old;
}

static void
menu_newserver_tab (GtkWidget * wid, gpointer none)
{
	int old = prefs.tabchannels;

	prefs.tabchannels = 1;
	new_ircwindow (NULL, NULL, SESS_SERVER);
	prefs.tabchannels = old;
}

static void
menu_newchannel_tab (GtkWidget * wid, gpointer none)
{
	int old = prefs.tabchannels;

	prefs.tabchannels = 1;
	new_ircwindow (menu_sess->server, NULL, SESS_CHANNEL);
	prefs.tabchannels = old;
}

static void
menu_rawlog (GtkWidget * wid, gpointer none)
{
	open_rawlog (menu_sess->server);
}

static void
menu_autodccsend (GtkWidget * wid, gpointer none)
{
	prefs.autodccsend = !prefs.autodccsend;
#ifndef WIN32
	if (prefs.autodccsend)
	{
		if (!strcasecmp (g_get_home_dir (), prefs.dccdir))
		{
			gtkutil_simpledialog (_("*WARNING*\n"
										 "Auto accepting DCC to your home directory\n"
										 "can be dangerous and is exploitable. Eg:\n"
										 "Someone could send you a .bash_profile"));
		}
	}
#endif
}

static void
menu_close (GtkWidget * wid, gpointer none)
{
	gtk_widget_destroy (menu_sess->gui->window);
}

static void
menu_search ()
{
	search_open (menu_sess);
}

static void
menu_flushbuffer (GtkWidget * wid, gpointer none)
{
	fe_text_clear (menu_sess);
}

static void
savebuffer_req_done (session *sess, void *arg2, char *file)
{
	int fh, newlen;
	char *buf;
	textentry *ent;

	if (!file)
		return;

	fh = open (file, O_TRUNC | O_WRONLY | O_CREAT, 0600);
	if (fh != -1)
	{
		ent = GTK_XTEXT(sess->gui->textgad)->text_first;
		while (ent)
		{
			buf = gtk_xtext_strip_color (ent->str, ent->str_len, NULL,
							&newlen, GTK_XTEXT(sess->gui->textgad)->fonttype, NULL);
			write (fh, buf, strlen (buf));
			write (fh, "\n", 1);
			free (buf);
			ent = ent->next;
		}
		close (fh);
	}
	free (file);
}

static void
menu_savebuffer (GtkWidget * wid, gpointer none)
{
	gtkutil_file_req (_("Select an output filename"), savebuffer_req_done,
							menu_sess, 0, TRUE);
}

static void
menu_wallops (GtkWidget * wid, gpointer none)
{
	char tbuf[128];
	prefs.wallops = !prefs.wallops;
	if (menu_sess->server->connected)
	{
		if (prefs.wallops)
			sprintf (tbuf, "MODE %s +w\r\n", menu_sess->server->nick);
		else
			sprintf (tbuf, "MODE %s -w\r\n", menu_sess->server->nick);
		tcp_send (menu_sess->server, tbuf);
	}
}

static void
menu_servernotice (GtkWidget * wid, gpointer none)
{
	char tbuf[128];
	prefs.servernotice = !prefs.servernotice;
	if (menu_sess->server->connected)
	{
		if (prefs.servernotice)
			sprintf (tbuf, "MODE %s +s\r\n", menu_sess->server->nick);
		else
			sprintf (tbuf, "MODE %s -s\r\n", menu_sess->server->nick);
		tcp_send (menu_sess->server, tbuf);
	}
}

static void
menu_away (GtkWidget * wid, gpointer none)
{
	handle_command ("/away", menu_sess, FALSE, FALSE);
}

static void
menu_invisible (GtkWidget * wid, gpointer none)
{
	char tbuf[128];
	prefs.invisible = !prefs.invisible;
	if (menu_sess->server->connected)
	{
		if (prefs.invisible)
			sprintf (tbuf, "MODE %s +i\r\n", menu_sess->server->nick);
		else
			sprintf (tbuf, "MODE %s -i\r\n", menu_sess->server->nick);
		tcp_send (menu_sess->server, tbuf);
	}
}

static void
menu_savedefault (GtkWidget * wid, gpointer none)
{
	palette_save ();
	if (save_config ())
		gtkutil_simpledialog (_("Settings saved."));
}

static void
menu_chanlist (GtkWidget * wid, gpointer none)
{
	chanlist_opengui (menu_sess->server);
}

static void
menu_banlist (GtkWidget * wid, gpointer none)
{
	banlist_opengui (menu_sess);
}


#ifdef USE_PERL

static void
menu_loadperl_callback (struct session *sess, void *data2, char *file)
{
	if (file)
	{
		char *buf = malloc (strlen (file) + 7);

		sprintf (buf, "/LOAD %s", file);
		free (file);
		handle_command (buf, sess, FALSE, FALSE);
		free (buf);
	}
}

static void
menu_loadperl (void)
{
	gtkutil_file_req (_("Select a Perl script to load"), menu_loadperl_callback,
							menu_sess, 0, FALSE);
}

static void
menu_unloadall (void)
{
	cmd_unloadall (menu_sess, 0, 0, 0);
}

static void
menu_perllist (void)
{
	handle_command ("/scpinfo", menu_sess, FALSE, FALSE);
}

#else

#define menu_perllist 0
#define menu_unloadall 0
#define menu_loadperl 0

#endif


#ifdef USE_PLUGIN

static void
menu_loadplugin_callback (struct session *sess, void *data2, char *file)
{
	if (file)
	{
		char *buf = malloc (strlen (file) + 10);

		sprintf (buf, "/LOADDLL %s", file);
		free (file);
		handle_command (buf, sess, FALSE, FALSE);
		free (buf);
	}
}

static void
menu_loadplugin (void)
{
	gtkutil_file_req (_("Select a Plugin to load"), menu_loadplugin_callback,
							menu_sess, 0, FALSE);
}

static void
menu_pluginlist (void)
{
	module_glist (menu_sess);
}

static void
menu_unloadallplugins (void)
{
	module_unload (0, menu_sess);
}

#else

#define menu_unloadallplugins 0
#define menu_pluginlist 0
#define menu_loadplugin 0

#endif


#ifdef USE_PYTHON

static void
menu_loadpython_callback (struct session *sess, void *data2, char *file)
{
	if (file)
	{
		char *buf = malloc (strlen (file) + 8);

		sprintf (buf, "/pload %s", file);
		free (file);
		handle_command (buf, sess, FALSE, FALSE);
		free (buf);
	}
}

static void
menu_loadpython (void)
{
	gtkutil_file_req (_("Select a Python script to load"),
							menu_loadpython_callback, menu_sess, 0, FALSE);
}

static void
menu_pythonlist (void)
{
	handle_command ("/plist", menu_sess, FALSE, FALSE);
}

#else

#define menu_pythonlist 0
#define menu_loadpython 0

#endif


#define usercommands_help  _("User Commands - Special codes:\n\n"\
                           "%c  =  current channel\n"\
									"%m  =  machine info\n"\
                           "%n  =  your nick\n"\
									"%t  =  time/date\n"\
                           "%v  =  x-chat version ("VERSION")\n"\
                           "%2  =  word 2\n"\
                           "%3  =  word 3\n"\
                           "&2  =  word 2 to the end of line\n"\
                           "&3  =  word 3 to the end of line\n\n"\
                           "eg:\n"\
                           "/cmd john hello\n\n"\
                           "%2 would be \042john\042\n"\
                           "&2 would be \042john hello\042.")

#define ulpopup_help       _("Userlist Popup - Special codes:\n\n"\
                           "%c  =  current channel\n"\
                           "%h  =  selected nick's hostname\n"\
									"%m  =  machine info\n"\
                           "%n  =  your nick\n"\
                           "%s  =  selected nick\n"\
									"%t  =  time/date\n")

#define ulbutton_help       _("Userlist Buttons - Special codes:\n\n"\
                           "%a  =  all selected nicks\n"\
                           "%c  =  current channel\n"\
                           "%h  =  selected nick's hostname\n"\
									"%m  =  machine info\n"\
                           "%n  =  your nick\n"\
                           "%s  =  selected nick\n"\
									"%t  =  time/date\n")

#define dlgbutton_help      _("Dialog Buttons - Special codes:\n\n"\
                           "%a  =  all selected nicks\n"\
                           "%c  =  current channel\n"\
                           "%h  =  selected nick's hostname\n"\
									"%m  =  machine info\n"\
                           "%n  =  your nick\n"\
                           "%s  =  selected nick\n"\
									"%t  =  time/date\n")

#define ctcp_help          _("CTCP Replies - Special codes:\n\n"\
                           "%d  =  data (the whole ctcp)\n"\
									"%m  =  machine info\n"\
                           "%s  =  nick who sent the ctcp\n"\
                           "%t  =  time/date\n"\
                           "%2  =  word 2\n"\
                           "%3  =  word 3\n"\
                           "&2  =  word 2 to the end of line\n"\
                           "&3  =  word 3 to the end of line\n\n")

#define url_help           _("URL Handlers - Special codes:\n\n"\
                           "%s  =  the URL string\n\n"\
                           "Putting a ! infront of the command\n"\
                           "indicates it should be sent to a\n"\
                           "shell instead of X-Chat")

static void
menu_usercommands (void)
{
	editlist_gui_open (command_list, _("X-Chat: User Defined Commands"),
							 "commands", "commands.conf", usercommands_help);
}

static void
menu_ulpopup (void)
{
	editlist_gui_open (popup_list, _("X-Chat: Userlist Popup menu"), "popup",
							 "popup.conf", ulpopup_help);
}

static void
menu_rpopup (void)
{
	editlist_gui_open (replace_list, _("X-Chat: Replace"), "replace",
							 "replace.conf", 0);
}

static void
menu_usermenu (void)
{
	editlist_gui_open (usermenu_list, _("X-Chat: User menu"), "usermenu",
							 "usermenu.conf", 0);
}

static void
menu_urlhandlers (void)
{
	editlist_gui_open (urlhandler_list, _("X-Chat: URL Handlers"), "urlhandlers",
							 "urlhandlers.conf", url_help);
}

static void
menu_evtpopup (void)
{
	pevent_dialog_show ();
}

static void
menu_keypopup (void)
{
	key_dialog_show ();
}

static void
menu_ulbuttons (void)
{
	editlist_gui_open (button_list, _("X-Chat: Userlist buttons"), "buttons",
							 "buttons.conf", ulbutton_help);
}

static void
menu_dlgbuttons (void)
{
	editlist_gui_open (dlgbutton_list, _("X-Chat: Dialog buttons"), "dlgbuttons",
							 "dlgbuttons.conf", dlgbutton_help);
}

static void
menu_ctcpguiopen (void)
{
	editlist_gui_open (ctcp_list, _("X-Chat: CTCP Replies"), "ctcpreply",
							 "ctcpreply.conf", ctcp_help);
}

static void
menu_reload (void)
{
	char *buf = malloc (strlen (default_file ()) + 12);
	load_config ();
	sprintf (buf, "%s reloaded.", default_file ());
	fe_message (buf, FALSE);
	free (buf);
}

static void
menu_autorejoin (GtkWidget *wid, gpointer none)
{
	prefs.autorejoin = !prefs.autorejoin;
}

static void
menu_autoreconnect (GtkWidget *wid, gpointer none)
{
	prefs.autoreconnect = !prefs.autoreconnect;
}

static void
menu_autoreconnectonfail (GtkWidget *wid, gpointer none)
{
	prefs.autoreconnectonfail = !prefs.autoreconnectonfail;
}

static void
menu_autodialog (GtkWidget *wid, gpointer none)
{
	if (GTK_CHECK_MENU_ITEM (wid)->active)
		prefs.autodialog = 1;
	else
		prefs.autodialog = 0;
}

static void
menu_autodccchat (GtkWidget *wid, gpointer none)
{
	prefs.autodccchat = !prefs.autodccchat;
}

static void
menu_saveexit (GtkWidget *wid, gpointer none)
{
	prefs.autosave = !prefs.autosave;
}

static void
menu_docs (GtkWidget *wid, gpointer none)
{
	goto_url ("http://xchat.org/docs.html");
}

static void
menu_webpage (GtkWidget *wid, gpointer none)
{
	goto_url ("http://xchat.org");
}

static void
menu_dcc_recv_win (GtkWidget *wid, gpointer none)
{
	fe_dcc_open_recv_win (FALSE);
}

static void
menu_dcc_send_win (GtkWidget *wid, gpointer none)
{
	fe_dcc_open_send_win (FALSE);
}

static void
menu_dcc_chat_win (GtkWidget *wid, gpointer none)
{
	fe_dcc_open_chat_win (FALSE);
}

#ifdef USE_GNOME

#include "../pixmaps/win.xpm"

static GnomeUIInfo xchatmenu[] = {
	{
	 GNOME_APP_UI_ITEM,
	 N_("Server List.."), 0,
	 menu_open_server_list, 0, 0,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BOOK_RED,
	 0, 0, 0},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM,
	 N_("New Server Tab.."), 0,
	 menu_newserver_tab, 0, 0,
	 GNOME_APP_PIXMAP_DATA, win_xpm,
	 0, 0, 0},
	{
	 GNOME_APP_UI_ITEM,
	 N_("New Server Window.."), 0,
	 menu_newserver_window, 0, 0,
	 GNOME_APP_PIXMAP_DATA, win_xpm,
	 0, 0, 0},
	{
	 GNOME_APP_UI_ITEM,
	 N_("New Channel Tab.."), 0,
	 menu_newchannel_tab, 0, 0,
	 GNOME_APP_PIXMAP_DATA, win_xpm,
	 0, 0, 0},
	{
	 GNOME_APP_UI_ITEM,
	 N_("New Channel Window.."), 0,
	 menu_newchannel_window, 0, 0,
	 GNOME_APP_PIXMAP_DATA, win_xpm,
	 0, 0, 0},
	GNOMEUIINFO_SEPARATOR,
#ifdef USE_ZVT
	{
	 GNOME_APP_UI_ITEM,
	 N_("New Shell Tab.."), 0,
	 menu_newshell_tab, 0, 0,
	 GNOME_APP_PIXMAP_DATA, win_xpm,
	 0, 0, 0},
#else
	{
	 GNOME_APP_UI_ITEM,
	 N_("New Shell Tab.."), 0,
	 0, 0, 0,
	 GNOME_APP_PIXMAP_DATA, win_xpm,
	 0, 0, 0},
#endif
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM,
	 N_("Close"), 0,
	 menu_close, 0, 0,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_CLOSE,
	 0, 0, 0},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM,
	 N_("Quit"), 0,
	 xchat_exit, 0, 0,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_QUIT,
	 'Q', GDK_MOD1_MASK, 0},
	GNOMEUIINFO_END
};

static GnomeUIInfo windowsmenu[] = {
	{
	 GNOME_APP_UI_ITEM,
	 N_("Channel List Window.."), 0,
	 menu_chanlist, 0, 0, GNOME_APP_PIXMAP_DATA, win_xpm,
	 },
	{
	 GNOME_APP_UI_ITEM,
	 N_("File Send Window.."), 0,
	 menu_dcc_send_win, 0, 0, GNOME_APP_PIXMAP_DATA, win_xpm,
	 },
	{
	 GNOME_APP_UI_ITEM,
	 N_("File Receive Window.."), 0,
	 menu_dcc_recv_win, 0, 0, GNOME_APP_PIXMAP_DATA, win_xpm,
	 },
	{
	 GNOME_APP_UI_ITEM,
	 N_("DCC Chat Window.."), 0,
	 menu_dcc_chat_win, 0, 0, GNOME_APP_PIXMAP_DATA, win_xpm,
	 },
	{
	 GNOME_APP_UI_ITEM,
	 N_("Raw Log Window.."), 0,
	 menu_rawlog, 0, 0, GNOME_APP_PIXMAP_DATA, win_xpm,
	 },
	{
	 GNOME_APP_UI_ITEM,
	 N_("URL Grabber Window.."), 0,
	 url_opengui, 0, 0, GNOME_APP_PIXMAP_DATA, win_xpm,
	 },
	{
	 GNOME_APP_UI_ITEM,
	 N_("Notify List Window.."), 0,
	 (menucallback) notify_opengui, 0, 0, GNOME_APP_PIXMAP_DATA, win_xpm,
	 },
	{
	 GNOME_APP_UI_ITEM,
	 N_("Ban List Window.."), 0,
	 (menucallback) menu_banlist, 0, 0, GNOME_APP_PIXMAP_DATA, win_xpm,
	},
	{
	 GNOME_APP_UI_ITEM,
	 N_("Ignore Window.."), 0,
	 ignore_gui_open, 0, 0, GNOME_APP_PIXMAP_DATA, win_xpm,
	 },
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM,
	 N_("Flush Buffer"), 0,
	 menu_flushbuffer, 0, 0,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_CLEAR,
	 'L', GDK_CONTROL_MASK, 0},
	{
	 GNOME_APP_UI_ITEM,
	 N_("Search Buffer.."), 0,
	 menu_search, 0, 0,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SEARCH,
	 0, 0, 0},
	{
	 GNOME_APP_UI_ITEM,
	 N_("Save Buffer.."), 0,
	 menu_savebuffer, 0, 0, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SAVE, 0,
	 0, 0},
	GNOMEUIINFO_END
};

static GnomeUIInfo usermodesmenu[] = {
	{
	 GNOME_APP_UI_TOGGLEITEM,
	 N_("Invisible"), 0,
	 menu_invisible, 0, 0, 0, 0, 0, 0, 0},
	{
	 GNOME_APP_UI_TOGGLEITEM,
	 N_("Receive Wallops"), 0,
	 menu_wallops, 0, 0, GNOME_APP_PIXMAP_NONE, 0, 0, 0, 0},
	{
	 GNOME_APP_UI_TOGGLEITEM,
	 N_("Receive Server Notices"), 0,
	 menu_servernotice, 0, 0, 0, 0, 0, 0, 0},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_TOGGLEITEM,
	 N_("Marked Away"), 0,
	 menu_away, 0, 0, GNOME_APP_PIXMAP_NONE, 0, 'A', GDK_MOD1_MASK, 0},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_TOGGLEITEM,
	 N_("Auto Rejoin on Kick"), 0,
	 menu_autorejoin, 0, 0, 0, 0, 0, 0, 0},
	{
	 GNOME_APP_UI_TOGGLEITEM,
	 N_("Auto ReConnect to Server"), 0,
	 menu_autoreconnect, 0, 0, 0, 0, 0, 0, 0},
	{
	 GNOME_APP_UI_TOGGLEITEM,
	 N_("Never-give-up ReConnect"), 0,
	 menu_autoreconnectonfail, 0, 0, 0, 0, 0, 0, 0},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_TOGGLEITEM,
	 N_("Auto Open Dialog Windows"), 0,
	 menu_autodialog, 0, 0, 0, 0, 0, 0, 0},
	{
	 GNOME_APP_UI_TOGGLEITEM,
	 N_("Auto Accept DCC Chat"), 0,
	 menu_autodccchat, 0, 0, 0, 0, 0, 0, 0},
	{
	 GNOME_APP_UI_TOGGLEITEM,
	 N_("Auto Accept DCC Send"), 0,
	 menu_autodccsend, 0, 0, 0, 0, 0, 0, 0},
	GNOMEUIINFO_END
};

static GnomeUIInfo settingsmenu[] = {
	{
	 GNOME_APP_UI_ITEM,
	 N_("Setup.."), 0,
	 menu_settings, 0, 0,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_PREF,
	 0, 0, 0},
	{
	 GNOME_APP_UI_ITEM,
	 N_("Palette.."), 0,
	 palette_edit, 0, 0,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_COLORSELECTOR,
	 0, 0, 0},
	{
	 GNOME_APP_UI_ITEM,
	 N_("User Commands.."), 0,
	 menu_usercommands, 0, 0,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_EXEC,
	 0, 0, 0},
	{
	 GNOME_APP_UI_ITEM,
	 N_("CTCP Replies.."), 0,
	 menu_ctcpguiopen, 0, 0,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_REFRESH,
	 0, 0, 0},
	{
	 GNOME_APP_UI_ITEM,
	 N_("Userlist Buttons.."), 0,
	 menu_ulbuttons, 0, 0,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_STOP,
	 0, 0, 0},
	{
	 GNOME_APP_UI_ITEM,
	 N_("Userlist Popup.."), 0,
	 menu_ulpopup, 0, 0,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_INDEX,
	 0, 0, 0},
	{
	 GNOME_APP_UI_ITEM,
	 N_("Dialog Buttons.."), 0,
	 menu_dlgbuttons, 0, 0,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_STOP,
	 0, 0, 0},
	{
	 GNOME_APP_UI_ITEM,
	 N_("Replace Popup.."), 0,
	 menu_rpopup, 0, 0,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ALIGN_LEFT,
	 0, 0, 0},
	{
	 GNOME_APP_UI_ITEM,
	 N_("URL Handlers.."), 0,
	 menu_urlhandlers, 0, 0,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_HOME,
	 0, 0, 0},
	{
	 GNOME_APP_UI_ITEM,
	 N_("Edit Event Texts.."), 0,
	 menu_evtpopup, 0, 0,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ALIGN_JUSTIFY,
	 0, 0, 0},
	{
	 GNOME_APP_UI_ITEM,
	 N_("Edit Key Bindings.."), 0,
	 menu_keypopup, 0, 0,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_FONT,
	 0, 0, 0},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM,
	 N_("Reload Settings"), 0,
	 menu_reload, 0, 0,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_REVERT,
	 0, 0, 0},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM,
	 N_("Save Settings now"), 0,
	 menu_savedefault, 0, 0,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SAVE,
	 0, 0, 0},
	{
	 GNOME_APP_UI_TOGGLEITEM,
	 N_("Save Settings on exit"), 0,
	 menu_saveexit, 0, 0,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SAVE,
	 0, 0, 0},
	GNOMEUIINFO_END
};

static GnomeUIInfo helpmenu[] = {
	{
	 GNOME_APP_UI_ITEM,
	 N_("X-Chat Homepage.."), 0,
	 menu_webpage, 0, 0,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_HOME,
	 0, 0, 0},
	{
	 GNOME_APP_UI_ITEM,
	 N_("Online Docs.."), 0,
	 menu_docs, 0, 0,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BOOK_OPEN,
	 0, 0, 0},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM,
	 N_("About X-Chat.."), 0,
	 menu_about, 0, 0,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ABOUT,
	 0, 0, 0},
	GNOMEUIINFO_END
};

static GnomeUIInfo loadmenu[] = {
	{
	 GNOME_APP_UI_ITEM,
	 N_("Perl Script.."), 0,
	 menu_loadperl, 0, 0, 0, 0, 0, 0, 0},
	{
	 GNOME_APP_UI_ITEM,
	 N_("Python Script.."), 0,
	 menu_loadpython, 0, 0, 0, 0, 0, 0, 0},
	{
	 GNOME_APP_UI_ITEM,
	 N_("Plugin.."), 0,
	 menu_loadplugin, 0, 0, 0, 0, 0, 0, 0},
	GNOMEUIINFO_END
};

static GnomeUIInfo infomenu[] = {
	{
	 GNOME_APP_UI_ITEM,
	 N_("Perl List"), 0,
	 menu_perllist, 0, 0, 0, 0, 0, 0, 0},
	{
	 GNOME_APP_UI_ITEM,
	 N_("Python List"), 0,
	 menu_pythonlist, 0, 0, 0, 0, 0, 0, 0},
	{
	 GNOME_APP_UI_ITEM,
	 N_("Plugin List"), 0,
	 menu_pluginlist, 0, 0, 0, 0, 0, 0, 0},
	GNOMEUIINFO_END
};

static GnomeUIInfo killmenu[] = {
	{
	 GNOME_APP_UI_ITEM,
	 N_("All Perl Scripts"), 0,
	 menu_unloadall, 0, 0, 0, 0, 0, 0, 0},
	{
	 GNOME_APP_UI_ITEM,
	 N_("All Python Scripts"), 0,
	 0, 0, 0, 0, 0, 0, 0, 0},
	{
	 GNOME_APP_UI_ITEM,
	 N_("All Plugins"), 0,
	 menu_unloadallplugins, 0, 0, 0, 0, 0, 0, 0},
	GNOMEUIINFO_END
};

static GnomeUIInfo scriptsmenu[] = {
	GNOMEUIINFO_SUBTREE_STOCK (N_("Load"), loadmenu, GNOME_STOCK_MENU_SAVE),
	GNOMEUIINFO_SUBTREE_STOCK (N_("Info"), infomenu, GNOME_STOCK_MENU_BOOK_GREEN),
	GNOMEUIINFO_SUBTREE_STOCK (N_("Kill"), killmenu, GNOME_STOCK_MENU_CLOSE),
	GNOMEUIINFO_END
};

static GnomeUIInfo usermenu[] = {
	{
	 GNOME_APP_UI_ITEM,
	 N_("Edit User Menu.."), 0,
	 menu_usermenu, 0, 0,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_PREF,
	 0, 0, 0},
	GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_END
};

static GnomeUIInfo mainmenu[] = {
	GNOMEUIINFO_SUBTREE (N_("_X-Chat"), xchatmenu),
	GNOMEUIINFO_SUBTREE (N_("_Windows"), windowsmenu),
	GNOMEUIINFO_SUBTREE (N_("User _Modes"), usermodesmenu),
	GNOMEUIINFO_SUBTREE (N_("_Settings"), settingsmenu),
	GNOMEUIINFO_SUBTREE (N_("S_cripts & Plugins"), scriptsmenu),
	GNOMEUIINFO_SUBTREE (N_("Use_r Menu"), usermenu),
	GNOMEUIINFO_SUBTREE (N_("_Help"), helpmenu),
	GNOMEUIINFO_END
};

GtkWidget *
createmenus (void *app, struct session *sess, int bar)
{
	GtkWidget *menu;

	if (!menu_sess)
		menu_sess = sess;

	if (bar)
	{
		gnome_app_create_menus (GNOME_APP (app), mainmenu);
		menu = GNOME_APP (app)->menubar;
		gtk_menu_item_right_justify (GTK_MENU_ITEM (mainmenu[6].widget));
	} else
	{
		menu = gnome_popup_menu_new (mainmenu);
	}

	/* is it legal to poke in the initial values?? */
	((GtkCheckMenuItem *) (usermodesmenu[0].widget))->active = prefs.invisible;
	((GtkCheckMenuItem *) (usermodesmenu[1].widget))->active = prefs.wallops;
	((GtkCheckMenuItem *) (usermodesmenu[2].widget))->active = prefs.servernotice;
	((GtkCheckMenuItem *) (usermodesmenu[4].widget))->active = sess->server->is_away;
	((GtkCheckMenuItem *) (usermodesmenu[6].widget))->active = prefs.autorejoin;
	((GtkCheckMenuItem *) (usermodesmenu[7].widget))->active = prefs.autoreconnect;
	((GtkCheckMenuItem *) (usermodesmenu[8].widget))->active = prefs.autoreconnectonfail;
	((GtkCheckMenuItem *) (usermodesmenu[10].widget))->active = prefs.autodialog;
	((GtkCheckMenuItem *) (usermodesmenu[11].widget))->active = prefs.autodccchat;
	((GtkCheckMenuItem *) (usermodesmenu[12].widget))->active = prefs.autodccsend;
	((GtkCheckMenuItem *) (settingsmenu[15].widget))->active = prefs.autosave;

#ifndef USE_PERL
	gtk_widget_set_sensitive (loadmenu[0].widget, FALSE);
	gtk_widget_set_sensitive (infomenu[0].widget, FALSE);
	gtk_widget_set_sensitive (killmenu[0].widget, FALSE);
#endif
#ifndef USE_PYTHON
	gtk_widget_set_sensitive (loadmenu[1].widget, FALSE);
	gtk_widget_set_sensitive (infomenu[1].widget, FALSE);
#endif
	gtk_widget_set_sensitive (killmenu[1].widget, FALSE);	/* unimplemented */
#ifndef USE_PLUGIN
	gtk_widget_set_sensitive (loadmenu[2].widget, FALSE);
	gtk_widget_set_sensitive (infomenu[2].widget, FALSE);
	gtk_widget_set_sensitive (killmenu[2].widget, FALSE);
#endif
#ifndef USE_ZVT
	gtk_widget_set_sensitive (xchatmenu[7].widget, FALSE);
#endif

	if (bar)	/* popup menus can't alter sess->gui, only a menubar! */
	{
		sess->gui->usermenu = GTK_MENU_ITEM (mainmenu[5].widget)->submenu;
		sess->gui->awaymenuitem = usermodesmenu[4].widget;
	}

	usermenu_create (GTK_MENU_ITEM (mainmenu[5].widget)->submenu);

	return menu;
}

#else

static struct mymenu mymenu[] = {
	{M_NEWMENU, N_("X-Chat"), 0, 0, 1},
	{M_MENU, N_("Server List.."), (menucallback) menu_open_server_list, 0, 1},
	{M_SEP, 0, 0, 0, 0},
	{M_MENU, N_("New Server Tab.."), (menucallback) menu_newserver_tab, 0, 1},
	
	{M_MENU, N_("New Server Window.."), (menucallback) menu_newserver_window, 0, 1},
	{M_MENU, N_("New Channel Tab.."), (menucallback) menu_newchannel_tab, 0, 1},
	{M_MENU, N_("New Channel Window.."), (menucallback) menu_newchannel_window, 0, 1},
	{M_SEP, 0, 0, 0, 0},
#ifdef USE_ZVT
	{M_MENU, N_("New Shell Tab.."), (menucallback) menu_newshell_tab, 0, 1},
	{M_SEP, 0, 0, 0, 0},
#define menuoffset 0
#else
#define menuoffset 2
#endif
	{M_MENU, N_("Close"), (menucallback) menu_close, 0, 1},
	{M_SEP, 0, 0, 0, 0},
	{M_MENU, N_("Quit"), (menucallback) xchat_exit, 0, 1},	/* 12 */

	{M_NEWMENU, N_("Windows"), 0, 0, 1},
	{M_MENU, N_("Channel List Window.."), (menucallback) menu_chanlist, 0, 1},
	{M_MENU, N_("File Send Window.."), (menucallback) menu_dcc_send_win, 0, 1},
	{M_MENU, N_("File Receive Window.."), (menucallback) menu_dcc_recv_win, 0, 1},
	{M_MENU, N_("DCC Chat Window.."), (menucallback) menu_dcc_chat_win, 0, 1},
	{M_MENU, N_("Raw Log Window.."), (menucallback) menu_rawlog, 0, 1},
	{M_MENU, N_("URL Grabber Window.."), (menucallback) url_opengui, 0, 1},
	{M_MENU, N_("Notify List Window.."), (menucallback) notify_opengui, 0, 1},
	{M_MENU, N_("Ignore Window.."), (menucallback) ignore_gui_open, 0, 1},
	{M_MENU, N_("Ban List Window.."), (menucallback) menu_banlist, 0, 1},
	{M_SEP, 0, 0, 0, 0},
	{M_MENU, N_("Flush Buffer"), (menucallback) menu_flushbuffer, 0, 1},
	{M_MENU, N_("Search Buffer.."), (menucallback) menu_search, 0, 1},
	{M_MENU, N_("Save Buffer.."), (menucallback) menu_savebuffer, 0, 1},	/* 26 */

	{M_NEWMENU, N_("User Modes"), 0, 0, 1},
	{M_MENUTOG, N_("Invisible"), (menucallback) menu_invisible, 1, 1},
	{M_MENUTOG, N_("Receive Wallops"), (menucallback) menu_wallops, 1, 1},
	{M_MENUTOG, N_("Receive Server Notices"), (menucallback) menu_servernotice, 1, 1},
	{M_SEP, 0, 0, 0, 0},
	{M_MENUTOG, N_("Marked Away"), (menucallback) menu_away, 0, 1},
	{M_SEP, 0, 0, 0, 0},														/* 33 */
	{M_MENUTOG, N_("Auto ReJoin on Kick"), (menucallback) menu_autorejoin, 0, 1},
	{M_MENUTOG, N_("Auto ReConnect to Server"), (menucallback) menu_autoreconnect, 0, 1},
	{M_MENUTOG, N_("Never-give-up ReConnect"), (menucallback) menu_autoreconnectonfail, 0, 1},
	{M_SEP, 0, 0, 0, 0},														/* 37 */
	{M_MENUTOG, N_("Auto Open Dialog Windows"), (menucallback) menu_autodialog, 0, 1},
	{M_MENUTOG, N_("Auto Accept DCC Chat"), (menucallback) menu_autodccchat, 0, 1},
	{M_MENUTOG, N_("Auto Accept DCC Send"), (menucallback) menu_autodccsend, 0, 1},

	{M_NEWMENU, N_("Settings"), 0, 0, 1},	/* 41 */
	{M_MENU, N_("Setup.."), (menucallback) menu_settings, 0, 1},
	{M_MENU, N_("Palette.."), (menucallback) palette_edit, 0, 1},
	{M_MENU, N_("User Commands.."), (menucallback) menu_usercommands, 0, 1},
	{M_MENU, N_("CTCP Replies.."), (menucallback) menu_ctcpguiopen, 0, 1},
	{M_MENU, N_("Userlist Buttons.."), (menucallback) menu_ulbuttons, 0, 1},
	{M_MENU, N_("Userlist Popup.."), (menucallback) menu_ulpopup, 0, 1},
	{M_MENU, N_("Dialog Buttons.."), (menucallback) menu_dlgbuttons, 0, 1},
	{M_MENU, N_("Replace.."), (menucallback) menu_rpopup, 0, 1},
	{M_MENU, N_("URL Handlers.."), (menucallback) menu_urlhandlers, 0, 1},
	{M_MENU, N_("Edit Event Texts.."), (menucallback) menu_evtpopup, 0, 1},	/* 51 */
	{M_MENU, N_("Edit Key Bindings.."), (menucallback) menu_keypopup, 0, 1},
	{M_SEP, 0, 0, 0, 0},
	{M_MENU, N_("Reload Settings"), (menucallback) menu_reload, 0, 1},
	{M_SEP, 0, 0, 0, 0},
	{M_MENU, N_("Save Settings now"), (menucallback) menu_savedefault, 0, 1},
	{M_MENUTOG, N_("Save Settings on exit"), (menucallback) menu_saveexit, 1, 1},
	{M_NEWMENU, N_("Scripts & Plugins"), 0, 0, 1},	/* 58 */
#ifdef USE_PERL
	{M_MENU, N_("Load Perl Script.."), (menucallback) menu_loadperl, 0, 1},
	{M_MENU, N_("Unload All Scripts"), (menucallback) menu_unloadall, 0, 1},
	{M_MENU, N_("Perl List"), (menucallback) menu_perllist, 0, 1},
#else
	{M_MENU, N_("Load Perl Script.."), 0, 0, 0},
	{M_MENU, N_("Unload All Scripts"), 0, 0, 0},
	{M_MENU, N_("Perl List"), 0, 0, 0},
#endif
	{M_SEP, 0, 0, 0, 0},
#ifdef USE_PLUGIN
	{M_MENU, N_("Load Plugin (*.so).."), (menucallback) menu_loadplugin, 0, 1},
	{M_MENU, N_("Unload All Plugins"), (menucallback) menu_unloadallplugins, 0, 1},
	{M_MENU, N_("Plugin List"), (menucallback) menu_pluginlist, 0, 1},
#else
	{M_MENU, N_("Load Plugin (*.so).."), 0, 0, 0},
	{M_MENU, N_("Unload All Plugins"), 0, 0, 0},
	{M_MENU, N_("Plugin List"), 0, 0, 0},
#endif
	{M_SEP, 0, 0, 0, 0},
#ifdef USE_PYTHON
	{M_MENU, N_("Load Python Script.."), (menucallback) menu_loadpython, 0, 1},
	{M_MENU, N_("Python List"), (menucallback) menu_pythonlist, 0, 1},
#else
	{M_MENU, N_("Load Python Script.."), 0, 0, 0},
	{M_MENU, N_("Python List"), 0, 0, 0},
#endif

	{M_NEWMENU, N_("User Menu"), (menucallback) - 1, 0, 1},
	{M_MENU, N_("Edit User Menu"), (menucallback) menu_usermenu, 0, 1},
	{M_SEP, 0, 0, 0, 0},

	{M_NEWMENURIGHT, N_("Help"), 0, 0, 1},
	{M_MENU, N_("X-Chat Homepage.."), (menucallback) menu_webpage, 0, 1},
	{M_MENU, N_("Online Docs.."), (menucallback) menu_docs, 0, 1},
	{M_SEP, 0, 0, 0, 0},
	{M_MENU, N_("About X-Chat.."), (menucallback) menu_about, 0, 1},

	{M_END, 0, 0, 0, 0},
};


GtkWidget *
createmenus (void *app, struct session *sess, int bar)
{
	int i = 0;
	GtkWidget *item;
	GtkWidget *menu = 0;
	GtkWidget *menu_item = 0;
	GtkWidget *menu_bar;
	GtkWidget *usermenu = 0;

	if (bar)
		menu_bar = gtk_menu_bar_new ();
	else
		menu_bar = gtk_menu_new ();

	if (!menu_sess)
		menu_sess = sess;

	mymenu[28-menuoffset].state = prefs.invisible;
	mymenu[29-menuoffset].state = prefs.wallops;
	mymenu[30-menuoffset].state = prefs.servernotice;
	mymenu[32-menuoffset].state = sess->server->is_away;
	mymenu[34-menuoffset].state = prefs.autorejoin;
	mymenu[35-menuoffset].state = prefs.autoreconnect;
	mymenu[36-menuoffset].state = prefs.autoreconnectonfail;
	mymenu[38-menuoffset].state = prefs.autodialog;
	mymenu[39-menuoffset].state = prefs.autodccchat;
	mymenu[40-menuoffset].state = prefs.autodccsend;
	mymenu[57-menuoffset].state = prefs.autosave;

	while (1)
	{
		switch (mymenu[i].type)
		{
		case M_NEWMENURIGHT:
		case M_NEWMENU:
			if (menu)
				gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), menu);
			menu = gtk_menu_new ();
			if (mymenu[i].callback == (void *) -1)
				usermenu = menu;
			menu_item = gtk_menu_item_new_with_label (mymenu[i].text);
			if (mymenu[i].type == M_NEWMENURIGHT)
				gtk_menu_item_right_justify ((GtkMenuItem *) menu_item);
			if (bar)
				gtk_menu_bar_append (GTK_MENU_BAR (menu_bar), menu_item);
			else
				gtk_menu_append (GTK_MENU (menu_bar), menu_item);
			gtk_widget_show (menu_item);
			break;
		case M_MENU:
			item = gtk_menu_item_new_with_label (mymenu[i].text);
			if (mymenu[i].callback)
				gtk_signal_connect_object (GTK_OBJECT (item), "activate",
													GTK_SIGNAL_FUNC (mymenu[i].callback),
													(gpointer)sess);
			gtk_menu_append (GTK_MENU (menu), item);
			gtk_widget_show (item);
			gtk_widget_set_sensitive (item, mymenu[i].activate);
			break;
		case M_MENUTOG:
			item = gtk_check_menu_item_new_with_label (mymenu[i].text);
			/* Always show checkbox, not just when it is moused over. */
			gtk_check_menu_item_set_show_toggle ((GtkCheckMenuItem*)item, TRUE);

			gtk_check_menu_item_set_state (GTK_CHECK_MENU_ITEM (item),
													 mymenu[i].state);
			if (mymenu[i].callback)
				gtk_signal_connect (GTK_OBJECT (item), "toggled",
										  GTK_SIGNAL_FUNC (mymenu[i].callback),
										  (gpointer)sess);
			gtk_menu_append (GTK_MENU (menu), item);
			gtk_widget_show (item);
			gtk_widget_set_sensitive (item, mymenu[i].activate);
			if (bar && i == 32 - menuoffset)
				sess->gui->awaymenuitem = item;
			break;
		case M_SEP:
			item = gtk_menu_item_new ();
			gtk_widget_set_sensitive (item, FALSE);
			gtk_menu_append (GTK_MENU (menu), item);
			gtk_widget_show (item);
			break;
		case M_END:
			if (menu)
				gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), menu);
			if (usermenu)
				usermenu_create (usermenu);
			if (bar)
				sess->gui->usermenu = usermenu;
			return (menu_bar);
		}
		i++;
	}
}
#endif
