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

#include <stdlib.h>
#include <string.h>
#include "../../config.h"
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <ctype.h>

#define WANTSOCKET
#define WANTARPA
#include "fe-gtk.h"
#include "../common/inet.h"
#include "../common/xchat.h"
#include "../common/fe.h"
#include "../common/xchatc.h"
#include "../common/util.h"
#include "../common/outbound.h"
#include <gdk/gdkkeysyms.h>
#include "ascii.h"
#include "banlist.h"
#include "dialog.h"
#include "gtkutil.h"
#include "menu.h"
#include "palette.h"
#include "panel.h"
#include "pixmaps.h"
#include "xtext.h"
#include "wins.h"
#include "fkeys.h"
#include "userlistgui.h"
#include "maingui.h"

#ifdef USE_PANEL
#include <applet-widget.h>
#endif

GtkWidget *main_window = 0;
GtkWidget *main_book;
GtkWidget *main_menu_bar;
static GtkWidget *main_menu;
static GtkWidget *main_menu_away_item = 0;

GtkStyle *redtab_style = 0;
GtkStyle *bluetab_style;
GtkStyle *inputgad_style;

static char chan_flags[] = { 't', 'n', 's', 'i', 'p', 'm', 'l', 'k' };

static void userlist_button (GtkWidget * box, char *label, char *cmd,
									  struct session *sess, int a, int b, int c,
									  int d);
static void my_gtk_togglebutton_state (GtkWidget * wid, int state);
static void tree_update ();
static void tree_default_style (struct session *sess);
static void channelmode_button_cb (GtkWidget *but, char *flag);
static void maingui_code (GtkWidget * button, char *code);


void
maingui_showhide_topic (session *sess)
{
	if (GTK_WIDGET_VISIBLE (sess->gui->tbox))
	{
		gtk_widget_hide (sess->gui->tbox);
		prefs.topicbar = 0;
	} else
	{
		gtk_widget_show (sess->gui->tbox);
		prefs.topicbar = 1;
	}
}

GtkWidget *
maingui_window (char *name, char *title, int force_toplevel,
						int link_buttons,
						void *close_callback, void *userdata,
						int width, int height, GtkWidget **child_ret)
{
	GtkWidget *win;

	if (prefs.windows_as_tabs && main_window && !force_toplevel)
	{
		win = wins_new (name, title, close_callback, userdata, NULL, NULL,
								TRUE, TRUE, child_ret);
		gtk_widget_show (win);
		if (prefs.newtabstofront)
			wins_bring_tofront (win);
	} else
	{
		win = wins_new (name, title, close_callback, userdata, NULL, NULL,
								FALSE, TRUE, child_ret);
		gtk_widget_set_usize (win, width, height);
	}

	if (link_buttons)
		wins_create_linkbuttons (win, wins_get_vbox (win));

	return win;
}

void
fe_set_away (server *serv)
{
	GSList *list = sess_list;
	session *sess;

	while (list)
	{
		sess = list->data;
		if (sess->server == serv)
		{
			if (!sess->gui->awaymenuitem)
				sess->gui->awaymenuitem = main_menu_away_item;
			if (sess->gui->awaymenuitem)
				((GtkCheckMenuItem*)sess->gui->awaymenuitem)->active = serv->is_away;
		}
		list = list->next;
	}
}

void
fe_set_nonchannel (struct session *sess, int state)
{
	int i;

	if (sess->gui->flag_wid[0])
	{
		for (i = 0; i < NUM_FLAG_WIDS; i++)
		{
			gtk_widget_set_sensitive (sess->gui->flag_wid[i], state);
		}
		gtk_widget_set_sensitive (sess->gui->limit_entry, state);
		gtk_widget_set_sensitive (sess->gui->key_entry, state);
	}
	if (prefs.inputgad_superfocus)	/* sanity */
		gtk_entry_set_editable ((GtkEntry *) sess->gui->topicgad, FALSE);
	else
		gtk_entry_set_editable ((GtkEntry *) sess->gui->topicgad, state);
}

static void
maingui_createbuttons (session * sess)
{
	struct popup *pop;
	GSList *list = button_list;
	int a = 0, b = 0;

	sess->gui->button_box = gtk_table_new (5, 2, FALSE);
	gtk_box_pack_end (GTK_BOX (sess->gui->nl_box), sess->gui->button_box,
							FALSE, FALSE, 1);
	gtk_widget_show (sess->gui->button_box);

	while (list)
	{
		pop = (struct popup *) list->data;
		if (pop->cmd[0])
		{
			userlist_button (sess->gui->button_box, pop->name, pop->cmd, sess, a,
								  a + 1, b, b + 1);
			a++;
			if (a == 2)
			{
				a = 0;
				b++;
			}
		}
		list = list->next;
	}
}

void
fe_buttons_update (struct session *sess)
{
	if (sess->gui->button_box)
	{
		gtk_widget_destroy (sess->gui->button_box);
		sess->gui->button_box = 0;
	}
	if (prefs.userlistbuttons)
		maingui_createbuttons (sess);
}

void
fe_set_title (struct session *sess)
{
	char tbuf[256];
	int type;

	type = sess->type;

	if (sess->server->connected == FALSE && sess->type != SESS_DIALOG)
		type = SESS_SHELL;	/* force default */

	switch (type)
	{
	case SESS_DIALOG:
		snprintf (tbuf, sizeof (tbuf), "X-Chat ["VERSION"]: Dialog with %s @ %s",
					 sess->channel, sess->server->servername);
		break;
	case SESS_SERVER:
		snprintf (tbuf, sizeof (tbuf), "X-Chat ["VERSION"]: %s @ %s",
					 sess->server->nick, sess->server->servername);
		break;
	case SESS_CHANNEL:
		snprintf (tbuf, sizeof (tbuf),
					 "X-Chat ["VERSION"]: %s @ %s / %s (%s)",
					 sess->server->nick, sess->server->servername,
					 sess->channel, sess->current_modes);
		break;
	case SESS_NOTICES:
	case SESS_SNOTICES:
		snprintf (tbuf, sizeof (tbuf), "X-Chat ["VERSION"]: %s @ %s (notices)",
					 sess->server->nick, sess->server->servername);
		break;
	default:
		wins_set_title (sess->gui->window, "X-Chat ["VERSION"]");
		return;
	}

	wins_set_title (sess->gui->window, tbuf);
}

void
fe_set_channel (struct session *sess)
{
	wins_set_name (sess->gui->window, sess->channel);
	/* toplevel dialogs dont have a changad */
	if (sess->gui->changad != NULL)
		gtk_label_set_text (GTK_LABEL (sess->gui->changad), sess->channel);
#ifdef USE_PANEL
	if (sess->gui->panel_button)
		gtk_label_set_text (GTK_LABEL
								  (GTK_BIN (sess->gui->panel_button)->child),
								  sess->channel);
#endif
	if (prefs.treeview)
		tree_update ();
}

void
fe_clear_channel (struct session *sess)
{
	char tbuf[CHANLEN+4];
	int i;

	if (sess->waitchannel[0])
		sprintf(tbuf, "(%s)", sess->waitchannel);
	else
		strcpy (tbuf, _("<none>"));
	gtk_entry_set_text (GTK_ENTRY (sess->gui->topicgad), "");
	gtk_label_set_text (GTK_LABEL (sess->gui->changad), tbuf);
	add_tip (sess->gui->topicgad, _("The channel topic"));
#ifdef USE_PANEL
	if (sess->gui->panel_button)
		gtk_label_set_text (GTK_LABEL
								  (GTK_BIN (sess->gui->panel_button)->child),
								  tbuf);
#endif
	gtk_widget_hide (sess->gui->namelistinfo);

	if (sess->gui->flag_wid[0])
	{
		for (i = 0; i < NUM_FLAG_WIDS - 1; i++)
			my_gtk_togglebutton_state (sess->gui->flag_wid[i], FALSE);
		gtk_entry_set_text ((GtkEntry*)sess->gui->limit_entry, "");
		gtk_entry_set_text ((GtkEntry*)sess->gui->key_entry, "");
	}

	if (sess->gui->op_xpm)
	{
		gtk_widget_destroy (sess->gui->op_xpm);
		sess->gui->op_xpm = 0;
	}

	if (prefs.treeview)
		tree_update ();
}

void
fe_set_nick (struct server *serv, char *newnick)
{
	GSList *list = sess_list;
	struct session *sess;

	strcpy (serv->nick, newnick);
	if (prefs.nickgad)
	{
		while (list)
		{
			sess = (struct session *) list->data;
			if (sess->server == serv && sess->type != SESS_DIALOG)
				gtk_label_set_text (GTK_LABEL (sess->gui->nickgad), newnick);
			list = list->next;
		}
	}
}

static void
handle_topicgad (GtkWidget * igad, struct session *sess)
{
	char *topic = gtk_entry_get_text (GTK_ENTRY (igad));
	if (sess->channel[0] && sess->server->connected)
	{
		char *fmt = "TOPIC %s :%s\r\n";
		char *tbuf;
		int len = strlen (fmt) - 3 + strlen (sess->channel) + strlen (topic);
		tbuf = malloc (len);
		snprintf (tbuf, len, fmt, sess->channel, topic);
		tcp_send (sess->server, tbuf);
		free (tbuf);
	} else
		gtk_entry_set_text (GTK_ENTRY (igad), "");
	/* restore focus to the input widget, where the next input will most
likely be */
	gtk_widget_grab_focus (sess->gui->inputgad);
}

static char *
find_selected_nick (struct session *sess)
{
	int row;
	struct User *user;

	row = gtkutil_clist_selection (sess->gui->namelistgad);
	if (row == -1)
		return 0;

	user = gtk_clist_get_row_data (GTK_CLIST (sess->gui->namelistgad), row);
	if (!user)
		return 0;
	return user->nick;
}

static void
ul_button_rel (GtkWidget * widget, GdkEventButton * even,
					struct session *sess)
{
	char *nick;
	int row, col;
	char buf[67];

	if (!even)
		return;
	if (even->button == 3)
	{
		if (gtk_clist_get_selection_info
			 (GTK_CLIST (widget), even->x, even->y, &row, &col) < 0)
			return;
		gtk_clist_unselect_all (GTK_CLIST (widget));
		gtk_clist_select_row (GTK_CLIST (widget), row, 0);
		nick = find_selected_nick (sess);
		if (nick)
		{
			menu_nickmenu (sess, even, nick);
		}
		return;
	}
	if (even->button == 2)
	{
		if (gtk_clist_get_selection_info
			 (GTK_CLIST (widget), even->x, even->y, &row, &col) != -1)
		{
			gtk_clist_select_row (GTK_CLIST (widget), row, col);
			nick = find_selected_nick (sess);
			if (nick)
			{
				snprintf (buf, sizeof (buf), "%s: ", nick);
				gtk_entry_set_text (GTK_ENTRY (sess->gui->inputgad), buf);
				gtk_widget_grab_focus (sess->gui->inputgad);
			}
		}
	}
}

void
focus_in (GtkWindow * win, GtkWidget * wid, session *sess)
{
	if (!sess)
	{
		if (current_tab)
		{
			if (current_tab->type != SESS_SHELL)
				gtk_widget_grab_focus (current_tab->gui->inputgad);
			else
				gtk_widget_grab_focus (current_tab->gui->textgad);
			menu_sess = current_tab;
			if (!prefs.use_server_tab)
				current_tab->server->front_session = current_tab;
		}
	} else
	{
		if (!prefs.use_server_tab)
			sess->server->front_session = sess;
		if (sess->type != SESS_SHELL)
			gtk_widget_grab_focus (sess->gui->inputgad);
		else
			gtk_widget_grab_focus (sess->gui->textgad);
		menu_sess = sess;
#ifdef USE_PANEL
		if (sess->gui->panel_button)
			gtk_widget_set_rc_style (GTK_BIN (sess->gui->panel_button)->child);
#endif
		if (prefs.treeview)
			tree_default_style (sess);
	}
}

static int
check_is_number (char *t)
{
	while (*t)
	{
		if (*t < '0' || *t > '9')
			return FALSE;
		t++;
	}
	return TRUE;
}

static void
change_channel_flag (GtkWidget * wid, struct session *sess, char flag)
{
	char outbuf[512];

	if (sess->server->connected && sess->channel[0])
	{
		if (GTK_TOGGLE_BUTTON (wid)->active)
			sprintf (outbuf, "MODE %s +%c\r\n", sess->channel, flag);
		else
			sprintf (outbuf, "MODE %s -%c\r\n", sess->channel, flag);
		tcp_send (sess->server, outbuf);
		sprintf (outbuf, "MODE %s\r\n", sess->channel);
		tcp_send (sess->server, outbuf);
		sess->ignore_mode = TRUE;
		sess->ignore_date = TRUE;
	}
}

static void
flagl_hit (GtkWidget * wid, struct session *sess)
{
	char outbuf[512];
	char *limit_str;

	if (GTK_TOGGLE_BUTTON (wid)->active)
	{
		if (sess->server->connected && sess->channel[0])
		{
			limit_str = gtk_entry_get_text (GTK_ENTRY (sess->gui->limit_entry));
			if (check_is_number (limit_str) == FALSE)
			{
				gtkutil_simpledialog (_("User limit must be a number!\n"));
				gtk_entry_set_text (GTK_ENTRY (sess->gui->limit_entry), "");
				gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (wid), FALSE);
				return;
			}
			sprintf (outbuf, "MODE %s +l %d\r\n", sess->channel,
						atoi (limit_str));
			tcp_send (sess->server, outbuf);
			sprintf (outbuf, "MODE %s\r\n", sess->channel);
			tcp_send (sess->server, outbuf);
		}
	} else
		change_channel_flag (wid, sess, 'l');
}

static void
flagk_hit (GtkWidget * wid, struct session *sess)
{
	char outbuf[512];

	if (GTK_TOGGLE_BUTTON (wid)->active)
	{
		if (sess->server->connected && sess->channel[0])
		{
			snprintf (outbuf, 512, "MODE %s +k %s\r\n", sess->channel,
						 gtk_entry_get_text (GTK_ENTRY (sess->gui->key_entry)));
			tcp_send (sess->server, outbuf);
			snprintf (outbuf, 512, "MODE %s\r\n", sess->channel);
			tcp_send (sess->server, outbuf);
		}
	} else
	{
		if (sess->server->connected && sess->channel[0])
		{
			snprintf (outbuf, 512, "MODE %s -k %s\r\n", sess->channel,
						 gtk_entry_get_text (GTK_ENTRY (sess->gui->key_entry)));
			tcp_send (sess->server, outbuf);
			snprintf (outbuf, 512, "MODE %s\r\n", sess->channel);
			tcp_send (sess->server, outbuf);
		}
	}
}

static void
channelmode_button_cb (GtkWidget *but, char *flag)
{
	session *sess;
	char mode;

	sess = gtk_object_get_user_data (GTK_OBJECT (but));
	if (!sess)	/* can be zero if called from my_gtk_togglebutton_state() */
		return;

	mode = tolower (flag[0]);

	switch (mode)
	{
	case 'l':
		flagl_hit (but, sess);
		break;
	case 'k':
		flagk_hit (but, sess);
		break;
	case 'b':
		my_gtk_togglebutton_state (sess->gui->flag_b, FALSE);
		banlist_opengui (sess);
		break;
	default:
		change_channel_flag (but, sess, mode);
	}
}

static void
key_entry (GtkWidget * igad, struct session *sess)
{
	if (sess->server->connected && sess->channel[0])
	{
		char outbuf[512];
		sprintf (outbuf, "MODE %s +k %s\r\n", sess->channel,
					gtk_entry_get_text (GTK_ENTRY (igad)));
		tcp_send (sess->server, outbuf);
		sprintf (outbuf, "MODE %s\r\n", sess->channel);
		tcp_send (sess->server, outbuf);
	}
}

static void
limit_entry (GtkWidget * igad, struct session *sess)
{
	char outbuf[512];
	if (sess->server->connected && sess->channel[0])
	{
		if (check_is_number (gtk_entry_get_text (GTK_ENTRY (igad))) == FALSE)
		{
			gtk_entry_set_text (GTK_ENTRY (igad), "");
			gtkutil_simpledialog (_("User limit must be a number!\n"));
			gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (sess->gui->flag_l),
												  FALSE);
			return;
		}
		sprintf (outbuf, "MODE %s +l %d\r\n", sess->channel,
					atoi (gtk_entry_get_text (GTK_ENTRY (igad))));
		tcp_send (sess->server, outbuf);
		sprintf (outbuf, "MODE %s\r\n", sess->channel);
		tcp_send (sess->server, outbuf);
	}
}

static GtkWidget *
add_flag_wid (char *tip, GtkWidget *box, char *face, session *sess)
{
	GtkWidget *wid;

	wid = gtk_toggle_button_new_with_label (face);
	gtk_widget_set_usize (wid, 18, -1);
	add_tip (wid, tip);
	gtk_box_pack_end (GTK_BOX (box), wid, 0, 0, 0);
	gtk_object_set_user_data (GTK_OBJECT (wid), sess);
	gtk_signal_connect (GTK_OBJECT (wid), "toggled",
							  GTK_SIGNAL_FUNC (channelmode_button_cb), face);
	show_and_unfocus (wid);

	return wid;
}

static void
userlist_button (GtkWidget * box, char *label, char *cmd,
					  struct session *sess, int a, int b, int c, int d)
{
	GtkWidget *wid = gtk_button_new_with_label (label);
	gtk_object_set_user_data (GTK_OBJECT (wid), sess);
	gtk_signal_connect (GTK_OBJECT (wid), "clicked",
							  GTK_SIGNAL_FUNC (userlist_button_cb), cmd);
	gtk_table_attach_defaults (GTK_TABLE (box), wid, a, b, c, d);
	show_and_unfocus (wid);
}


#define WORD_URL     1
#define WORD_NICK    2
#define WORD_CHANNEL 3
#define WORD_HOST    4
#define WORD_EMAIL   5
#define WORD_DIALOG  -1

/* check if a word is clickable */

int
maingui_word_check (GtkWidget * xtext, char *word)
{
	session *sess;
	char *at, *dot;
	int i, dots;
	int len = strlen (word);

	if ((word[0] == '@' || word[0] == '+') && word[1] == '#')
		return WORD_CHANNEL;

	if (word[0] == '#' && word[1] != '#' && word[1] != 0)
		return WORD_CHANNEL;

	if (!strncasecmp (word, "irc://", 6))
		return WORD_URL;

	if (!strncasecmp (word, "irc.", 4))
		return WORD_URL;

	if (!strncasecmp (word, "ftp.", 4))
		return WORD_URL;

	if (!strncasecmp (word, "ftp:", 4))
		return WORD_URL;

	if (!strncasecmp (word, "www.", 4))
		return WORD_URL;

	if (!strncasecmp (word, "http:", 5))
		return WORD_URL;

	if (!strncasecmp (word, "https:", 6))
		return WORD_URL;

	sess = gtk_object_get_user_data (GTK_OBJECT (xtext));

	if (find_name (sess, word))
		return WORD_NICK;

	at = strchr (word, '@');	  /* check for email addy */
	dot = strrchr (word, '.');
	if (at && dot)
	{
		if ((unsigned long) at < (unsigned long) dot)
		{
			if (strchr (word, '*'))
				return WORD_HOST;
			else
				return WORD_EMAIL;
		}
	}

	/* check if it's an IP number */
	dots = 0;
	for (i = 0; i < len; i++)
	{
		if (word[i] == '.')
			dots++;
	}
	if (dots == 3)
	{
		if (inet_addr (word) != -1)
			return WORD_HOST;
	}

	if (!strncasecmp (word + len - 5, ".html", 5))
		return WORD_HOST;

	if (!strncasecmp (word + len - 4, ".org", 4))
		return WORD_HOST;

	if (!strncasecmp (word + len - 4, ".net", 4))
		return WORD_HOST;

	if (!strncasecmp (word + len - 4, ".com", 4))
		return WORD_HOST;

	if (!strncasecmp (word + len - 4, ".edu", 4))
		return WORD_HOST;

	if (len > 5)
	{
		if (word[len - 3] == '.' &&
			 isalpha (word[len - 2]) && isalpha (word[len - 1]))
			return WORD_HOST;
	}

	if (sess->type == SESS_DIALOG)
		return WORD_DIALOG;

	return 0;
}

/* mouse click inside text area */

void
maingui_word_clicked (GtkWidget *xtext, char *word, GdkEventButton *even,
							 session *sess)
{

	if (even->button == 1)			/* left button */
	{
		if (even->state & GDK_CONTROL_MASK)
		{
			switch (maingui_word_check (xtext, word))
			{
			case WORD_URL:
			case WORD_HOST:
				goto_url (word);
			}
		}
		return;
	}

	if (even->button == 2)
	{
		if (sess->type == SESS_DIALOG)
			menu_middlemenu (sess, even);
		return;
	}

	switch (maingui_word_check (xtext, word))
	{
	case 0:
		menu_middlemenu (sess, even);
		break;
	case WORD_URL:
	case WORD_HOST:
		menu_urlmenu (even, word);
		break;
	case WORD_NICK:
		menu_nickmenu (sess, even, word);
		break;
	case WORD_CHANNEL:
		if (*word == '@' || *word == '+')
			word++;
		menu_chanmenu (sess, even, word);
		break;
	case WORD_EMAIL:
		{
			char *newword = malloc (strlen (word) + 10);
			if (*word == '~')
				word++;
			sprintf (newword, "mailto:%s", word);
			menu_urlmenu (even, newword);
			free (newword);
		}
		break;
	case WORD_DIALOG:
		menu_nickmenu (sess, even, sess->channel);
		break;
	}
}

void
maingui_configure (session *sess)
{
	if (sess == 0)			/* for the main_window */
	{
		sess = menu_sess;
		if (main_window && prefs.mainwindow_save)
		{
			gdk_window_get_root_origin (main_window->window,
										  &prefs.mainwindow_left,
										  &prefs.mainwindow_top);
			gdk_window_get_size (main_window->window,
										&prefs.mainwindow_width,
										&prefs.mainwindow_height);
		}
	}

	if (sess)
	{
#ifdef USE_ZVT
		if (sess->type == SESS_SHELL)
		{
			gtk_widget_queue_draw (sess->gui->textgad);
		} else
		{
#endif
			if (((GtkXText *) sess->gui->textgad)->transparent)
			{
				gtk_widget_queue_draw (sess->gui->textgad);
			}
#ifdef USE_ZVT
		}
#endif
	}
}

static void
maingui_create_textlist (struct session *sess, GtkWidget * leftpane)
{
	sess->gui->textgad =
		gtk_xtext_new (prefs.indent_pixels * prefs.indent_nicks,
							prefs.show_separator);

	gtk_object_set_user_data (GTK_OBJECT (sess->gui->textgad), sess);

	((GtkXText *) sess->gui->textgad)->wordwrap = prefs.wordwrap;
	((GtkXText *) sess->gui->textgad)->max_auto_indent = prefs.max_auto_indent;
	((GtkXText *) sess->gui->textgad)->auto_indent = prefs.auto_indent;
	((GtkXText *) sess->gui->textgad)->thinline = prefs.thin_separator;
	((GtkXText *) sess->gui->textgad)->max_lines = prefs.max_lines;
	((GtkXText *) sess->gui->textgad)->error_function = gtkutil_simpledialog;
	((GtkXText *) sess->gui->textgad)->urlcheck_function = maingui_word_check;

	((GtkXText *) sess->gui->textgad)->tint_red = prefs.tint_red;
	((GtkXText *) sess->gui->textgad)->tint_green = prefs.tint_green;
	((GtkXText *) sess->gui->textgad)->tint_blue = prefs.tint_blue;

	if (prefs.timestamp && prefs.indent_nicks)
		((GtkXText *) sess->gui->textgad)->time_stamp = TRUE;

	gtk_xtext_set_palette (GTK_XTEXT (sess->gui->textgad), colors);
	gtk_xtext_set_font (GTK_XTEXT (sess->gui->textgad), font_normal, 0);
	gtk_xtext_set_background (GTK_XTEXT (sess->gui->textgad),
									  channelwin_pix, prefs.transparent, prefs.tint);

	gtk_widget_set_usize (sess->gui->textgad,
								 prefs.mainwindow_width - 138,
								 prefs.mainwindow_height - 106);
	gtk_container_add (GTK_CONTAINER (leftpane), sess->gui->textgad);
	show_and_unfocus (sess->gui->textgad);

	sess->gui->vscrollbar =
		gtk_vscrollbar_new (GTK_XTEXT (sess->gui->textgad)->adj);
	gtk_box_pack_start (GTK_BOX (leftpane), sess->gui->vscrollbar, FALSE,
							  FALSE, 1);
	show_and_unfocus (sess->gui->vscrollbar);

	if (!sess->is_tab)
		gtk_signal_connect_object (GTK_OBJECT (sess->gui->window),
											"configure_event",
											GTK_SIGNAL_FUNC (maingui_configure),
											GTK_OBJECT (sess));

	gtk_signal_connect (GTK_OBJECT (sess->gui->textgad), "word_click",
							  GTK_SIGNAL_FUNC (maingui_word_clicked), sess);
}

/* called when a new tab gets focus */

static void
gui_new_tab (session * sess)
{
	current_tab = sess;
	menu_sess = sess;

	if (!prefs.use_server_tab)
		sess->server->front_session = sess;

	fe_set_away (sess->server);
	gtk_widget_grab_focus (sess->gui->inputgad);
	if (main_window)
		gtk_widget_set_sensitive (main_menu, TRUE);

#ifdef USE_PANEL
	if (sess->gui->panel_button)
		gtk_widget_set_rc_style (GTK_BIN (sess->gui->panel_button)->child);
#endif

	if (sess->new_data || sess->nick_said)
	{
		sess->nick_said = FALSE;
		sess->new_data = FALSE;
		gtk_widget_set_rc_style (sess->gui->changad);
		if (prefs.treeview)
			tree_default_style (sess);
	}
}

static void
gui_new_tab_callback (GtkWidget * widget, GtkNotebookPage * nbpage,
							 guint page)
{
	struct session *sess;
	GSList *list = sess_list;

	if (xchat_is_quitting || main_window == NULL)
		return;

	wins_update_notebooktitle (nbpage->child);

	while (list)
	{
		sess = (struct session *) list->data;
		if (sess->gui->window == nbpage->child && sess->type != SESS_SHELL)
		{
			gui_new_tab (sess);
			return;
		}
		list = list->next;
	}
	/* we're moved to a tab that isn't a session, the menus would crash! */
	if (main_window)
		gtk_widget_set_sensitive (main_menu, FALSE);
	current_tab = 0;
	menu_sess = 0;
}

static void
gui_main_window_kill (gpointer userdata)
{
	GSList *list;
	session *sess;

	xchat_is_quitting = TRUE;

	/* see if there's any non-tab windows left */
	list = sess_list;
	while (list)
	{
		sess = (session *) list->data;
		if (!sess->is_tab)
		{
			xchat_is_quitting = FALSE;
			break;
		}
		list = list->next;
	}

	main_window = 0;
	current_tab = 0;
}

void
userlist_hide (GtkWidget *igad, session *sess)
{
#ifdef USE_GNOMEE
	if (sess->userlisthidden > 0)
	{
		if (sess->gui->paned)
			gtk_paned_set_position (GTK_PANED (sess->gui->paned),
											sess->userlisthidden);
		else
			gtk_widget_show (sess->gui->userlistbox);
		sess->userlisthidden = 0;
	} else
	{
		if (sess->gui->paned)
		{
			sess->userlisthidden = GTK_PANED (sess->gui->paned)->handle_xpos + 3;
			gtk_paned_set_position (GTK_PANED (sess->gui->paned), 1200);
		} else
		{
			sess->userlisthidden = 1;
			gtk_widget_hide (sess->gui->userlistbox);
		}
	}
#else
	if (sess->userlisthidden)
	{
		if (igad)
			gtk_label_set (GTK_LABEL (GTK_BIN (igad)->child), ">");
		if (sess->gui->paned)
			gtk_paned_set_position (GTK_PANED (sess->gui->paned),
											sess->userlisthidden);
		else
			gtk_widget_show (sess->gui->userlistbox);
		sess->userlisthidden = FALSE;
		prefs.hideuserlist = 0;
	} else
	{
		if (igad)
			gtk_label_set (GTK_LABEL (GTK_BIN (igad)->child), "<");
		if (sess->gui->paned)
		{
			sess->userlisthidden = GTK_PANED (sess->gui->paned)->handle_xpos + 3;
			gtk_paned_set_position (GTK_PANED (sess->gui->paned), 1200);
		} else
		{
			sess->userlisthidden = TRUE;
			gtk_widget_hide (sess->gui->userlistbox);
		}
		prefs.hideuserlist = 1;
	}
#endif
}

static void
maingui_userlist_selected (GtkWidget * clist, gint row, gint column,
									GdkEventButton * even)
{
	struct User *user;

	if (even)
	{
		if (even->type == GDK_2BUTTON_PRESS)
		{
			if (prefs.doubleclickuser[0])
			{
				user = gtk_clist_get_row_data (GTK_CLIST (clist), row);
				nick_command_parse (menu_sess, prefs.doubleclickuser,
										  user->nick, user->nick);
			}
		} else
		{
			if (!(even->state & GDK_SHIFT_MASK))
			{
				gtk_clist_unselect_all (GTK_CLIST (clist));
				gtk_clist_select_row (GTK_CLIST (clist), row, column);
			}
		}
	}
}

/* Treeview code --AGL */

static GList *tree_list = NULL;

#define TREE_SERVER 0
#define TREE_SESSION 1

struct tree_data
{
	int type;
	void *data;
};

static void
tree_row_destroy (struct tree_data *td)
{
	g_free (td);
}

static void
tree_add_sess (GSList ** o_ret, struct server *serv)
{
	GSList *ret = *o_ret;
	GSList *session;
	struct session *sess;

	session = sess_list;
	while (session)
	{
		sess = session->data;
		if (sess->server == serv && sess->type != SESS_SERVER)
			ret = g_slist_prepend (ret, sess);
		session = session->next;
	}

	*o_ret = ret;
	return;
}

static GSList *
tree_build ()
{
	GSList *serv;
	GSList *ret = NULL;
	struct server *server;

	serv = serv_list;
	while (serv)
	{
		server = serv->data;
		ret = g_slist_prepend (ret, server);
		ret = g_slist_prepend (ret, NULL);
		tree_add_sess (&ret, server);
		ret = g_slist_prepend (ret, NULL);
		serv = serv->next;
	}

	ret = g_slist_reverse (ret);
	return ret;
}

static gpointer
tree_new_entry (int type, void *data)
{
	struct tree_data *td;

	td = g_new (struct tree_data, 1);
	td->type = type;
	td->data = data;

	return td;
}

static void
tree_populate (GtkCTree * tree, GSList * l)
{
	int state = 0;
	void *data;
	gchar *text[1];
	GtkCTreeNode *parent = NULL, *leaf;

	gtk_clist_freeze (GTK_CLIST (tree));
	gtk_clist_clear (GTK_CLIST (tree));
	for (; l; l = l->next)
	{
		data = l->data;
		if (data == NULL)
		{
			if (!state)
			{
				state = 1;
				continue;
			} else
			{
				state = 0;
				continue;
			}
		}
		if (!state)
		{
			text[0] = ((struct server *) data)->servername;
			if (text[0][0])
			{
				parent = gtk_ctree_insert_node (tree, NULL, NULL, text, 0, NULL,
														  NULL, NULL, NULL, 0, 1);
				((GtkCListRow *) ((GList *) parent)->data)->data =
					tree_new_entry (TREE_SERVER, data);
				((GtkCListRow *) ((GList *) parent)->data)->destroy =
					(GtkDestroyNotify) tree_row_destroy;
			}
			continue;
		} else
		{
			text[0] = ((struct session *) data)->channel;
			if (text[0][0])
			{
				leaf = gtk_ctree_insert_node (tree, parent, NULL, text, 0, NULL,
														NULL, NULL, NULL, 1, 1);
				((GtkCListRow *) ((GList *) leaf)->data)->data =
					tree_new_entry (TREE_SESSION, data);
				((GtkCListRow *) ((GList *) leaf)->data)->destroy =
					(GtkDestroyNotify) tree_row_destroy;
			}
			continue;
		}
	}
	gtk_clist_thaw (GTK_CLIST (tree));
}

static void
tree_destroy (GtkWidget * tree)
{
	tree_list = g_list_remove (tree_list, tree);
}

static void
tree_select_row (GtkWidget * tree, GList * row)
{
	struct tree_data *td;
	struct session *sess = NULL;
	struct server *serv = NULL;

	td = ((struct tree_data *) ((GtkCListRow *) row->data)->data);
	if (!td)
		return;
	if (td->type == TREE_SESSION)
	{
		sess = td->data;
		if (sess->is_tab)
		{
			if (main_window)		  /* this fixes a little refresh glitch */
			{
				wins_bring_tofront (sess->gui->window);
			}
		} else
		{
			gtk_widget_hide (sess->gui->window);
			gtk_widget_show (sess->gui->window);
		}
	} else if (td->type == TREE_SERVER)
	{
		serv = td->data;
		sess = serv->front_session;
		if (serv->front_session)
		{
			if (serv->front_session->is_tab)
			{
				if (main_window)
				{
					wins_bring_tofront (sess->gui->window);
				}
			} else
			{
				gtk_widget_hide (sess->gui->window);
				gtk_widget_show (sess->gui->window);
			}
		}
	}
}

static GtkWidget *
new_tree_view ()
{
	GtkWidget *tree;
	GSList *data;

	tree = gtk_ctree_new_with_titles (1, 0, NULL);
	gtk_clist_set_selection_mode (GTK_CLIST (tree), GTK_SELECTION_BROWSE);
	gtk_clist_set_column_width (GTK_CLIST (tree), 0, 80);
	gtk_signal_connect (GTK_OBJECT (tree), "destroy", tree_destroy, NULL);
	gtk_signal_connect (GTK_OBJECT (tree), "tree_select_row", tree_select_row,
							  0);
	data = tree_build ();
	tree_populate (GTK_CTREE (tree), data);
	tree_list = g_list_prepend (tree_list, tree);
	g_slist_free (data);

	return tree;
}

void
tree_update ()
{
	GList *tree;
	GSList *data = tree_build ();

	for (tree = tree_list; tree; tree = tree->next)
		tree_populate (GTK_CTREE (tree->data), data);

	g_slist_free (data);
}

static void
tree_set_color (session *sess, int col)
{
	GList *tree;
	GList *rows;
	struct tree_data *td;
	int row = 0;

	for (tree = tree_list; tree; tree = tree->next)
	{
		for (rows = (GTK_CLIST (tree->data)->row_list); rows; rows = rows->next)
		{
			td = ((struct tree_data *) ((GtkCListRow *) rows->data)->data);
			if (td->data == sess ||
				 (sess->type == SESS_SERVER && sess->server == td->data))
			{
				gtk_clist_set_foreground (GTK_CLIST (tree->data), row, &colors[col]);
				return;
			}
			row++;
		}
	}
}

static void
tree_default_style (session *sess)
{
	tree_set_color (sess, 1);
}

void
tree_red_style (session *sess)
{
	tree_set_color (sess, 4);
}

void
tree_blue_style (session *sess)
{
	tree_set_color (sess, 2);
}

void
maingui_set_tab_pos (int pos)
{
	gtk_notebook_set_show_tabs ((GtkNotebook *) main_book, TRUE);
	switch (pos)
	{
	case 0:
		gtk_notebook_set_tab_pos ((GtkNotebook *) main_book, GTK_POS_BOTTOM);
		break;
	case 1:
		gtk_notebook_set_tab_pos ((GtkNotebook *) main_book, GTK_POS_TOP);
		break;
	case 2:
		gtk_notebook_set_tab_pos ((GtkNotebook *) main_book, GTK_POS_LEFT);
		break;
	case 3:
		gtk_notebook_set_tab_pos ((GtkNotebook *) main_book, GTK_POS_RIGHT);
		break;
	case 4:
		gtk_notebook_set_show_tabs ((GtkNotebook *) main_book, FALSE);
		break;
	}
}

void
gui_make_tab_window (session *sess, GtkWidget *win)
{
	GtkWidget *main_box, *main_hbox = NULL, *trees;
	GSList *list;
#ifndef USE_GNOME
	GtkWidget *wid;
#endif

	/* YUK! FIXME */
	if (sess == NULL)
	{
		list = sess_list;
		while (list)
		{
			sess = list->data;
			if (sess->gui->window == win)
				break;
			list = list->next;
		}
	}

	current_tab = 0;

	main_window = wins_new ("X-Chat", "X-Chat", gui_main_window_kill, NULL,
									NULL, NULL, FALSE, FALSE, NULL);
	gtk_widget_set_usize (main_window, prefs.mainwindow_width,
									prefs.mainwindow_height);
	gtk_signal_connect ((GtkObject *) main_window, "focus_in_event",
							  GTK_SIGNAL_FUNC (focus_in), 0);
	gtk_signal_connect_object (GTK_OBJECT (main_window), "configure_event",
										GTK_SIGNAL_FUNC (maingui_configure), 0);

	main_box = wins_get_vbox (main_window);

#ifdef USE_GNOME
	main_menu = createmenus (main_window, sess, 1);
	main_menu_bar = main_menu->parent;
#else
	main_menu = createmenus (0, sess, 1);
	gtk_widget_show (main_menu);

	main_menu_bar = wid = gtk_handle_box_new ();
	gtk_container_add (GTK_CONTAINER (wid), main_menu);
	gtk_box_pack_start (GTK_BOX (main_box), wid, FALSE, TRUE, 0);
	if (!prefs.hidemenu)
		gtk_widget_show (wid);
#endif
	main_menu_away_item = sess->gui->awaymenuitem;
	if (prefs.treeview)
	{
		main_hbox = gtk_hpaned_new ();
		trees = new_tree_view ();
		gtk_widget_show (trees);
		gtk_paned_add1 (GTK_PANED (main_hbox), trees);
	}

	main_book = gtk_notebook_new ();
	gtk_notebook_set_show_border (GTK_NOTEBOOK (main_book), FALSE);
	maingui_set_tab_pos (prefs.tabs_position);
	gtk_notebook_set_scrollable ((GtkNotebook *) main_book, TRUE);
	gtk_signal_connect ((GtkObject *) main_book, "switch_page",
							  GTK_SIGNAL_FUNC (gui_new_tab_callback), 0);
	if (prefs.treeview)
	{
		gtk_container_add (GTK_CONTAINER (main_box), main_hbox);
		gtk_paned_add2 (GTK_PANED (main_hbox), main_book);
		gtk_widget_show (main_hbox);
	} else
		gtk_container_add (GTK_CONTAINER (main_box), main_book);

	if (prefs.inputgad_superfocus)
		show_and_unfocus (main_book);
	else
		gtk_widget_show (main_book);
}

#ifdef USE_GNOME
GtkTargetEntry dnd_targets[] = {
	{"text/uri-list", 0, 1}
};
#endif

static void
maingui_init_styles (GtkStyle *style)
{
	redtab_style = gtk_style_new ();
	gdk_font_unref (redtab_style->font);
	redtab_style->font = style->font;
	redtab_style->fg[0] = colors[4];
	gdk_font_ref (redtab_style->font);

	bluetab_style = gtk_style_new ();
	gdk_font_unref (bluetab_style->font);
	bluetab_style->font = style->font;
	bluetab_style->fg[0] = colors[12];
	gdk_font_ref (bluetab_style->font);
}

static void
maingui_toolbox (GtkWidget * button, GtkWidget * box)
{
	if (GTK_WIDGET_VISIBLE (box))
	{
		gtk_label_set_text (GTK_LABEL (GTK_BIN (button)->child), "<");
		gtk_widget_hide (box);
		add_tip (button, _("Open Toolbox"));
	} else
	{
		gtk_label_set_text (GTK_LABEL (GTK_BIN (button)->child), ">");
		gtk_widget_show (box);
		add_tip (button, _("Close Toolbox"));
	}
}

static void
maingui_code (GtkWidget * button, char *code)
{
	if (menu_sess)
		key_action_insert (menu_sess->gui->inputgad, 0, code, 0, menu_sess);
}

static GtkWidget *
toolbox_button (char *label, char *code, GtkWidget * box, char *tip)
{
	GtkWidget *wid;

	wid = gtk_button_new_with_label (label);
	gtk_box_pack_end (GTK_BOX (box), wid, 0, 0, 0);
	if (code)
		gtk_signal_connect (GTK_OBJECT (wid), "clicked",
								  GTK_SIGNAL_FUNC (maingui_code), code);
	gtk_widget_show (wid);
	if (tip)
		add_tip (wid, tip);

	return wid;
}

static void
maingui_cp (GtkWidget * button, session * sess)
{
	GTK_XTEXT (sess->gui->textgad)->color_paste =
		GTK_TOGGLE_BUTTON (button)->active;
}

static void
toolbox_color (GtkWidget * button, int color_no)
{
	char buf[8];

	sprintf (buf, "%%C%d", color_no);

	if (menu_sess)
		key_action_insert (menu_sess->gui->inputgad, 0, buf, 0, menu_sess);
}

/* create the < and ^ buttons to the right of the inputbox */

void
gui_create_toolbox (session *sess, GtkWidget *box)
{
	GtkStyle *style;
	GtkWidget *wid, *toolbox;
	int i;

	sess->gui->toolbox = toolbox = gtk_hbox_new (0, 0);
	gtk_box_pack_start (GTK_BOX (box), toolbox, 0, 0, 0);

	sess->gui->beepbutton = wid = gtk_toggle_button_new_with_label (_("Beep"));
	gtk_box_pack_end (GTK_BOX (toolbox), wid, 0, 0, 0);
	gtk_widget_show (wid);
	add_tip (wid, _("Beep on messages"));

	sess->gui->confbutton = wid = gtk_toggle_button_new_with_label (_("Conf"));
	gtk_box_pack_end (GTK_BOX (toolbox), wid, 0, 0, 0);
	gtk_widget_show (wid);
	add_tip (wid, _("Conference mode (no join/part msgs)"));

	wid = gtk_toggle_button_new_with_label (_("CP"));
	gtk_signal_connect (GTK_OBJECT (wid), "toggled",
							  GTK_SIGNAL_FUNC (maingui_cp), sess);
	gtk_box_pack_end (GTK_BOX (toolbox), wid, 0, 0, 0);
	gtk_widget_show (wid);
	add_tip (wid, _("Color Paste"));

	wid = toolbox_button (_("Ascii"), 0, toolbox, _("Open ASCII Chart"));
	gtk_signal_connect (GTK_OBJECT (wid), "clicked",
							  GTK_SIGNAL_FUNC (ascii_open), 0);

	for (i = 15; i; i--)
	{
		style = gtk_style_new ();
		style->bg[0] = colors[i];
		wid = toolbox_button ("  ", 0, toolbox, 0);
		gtk_widget_set_style (wid, style);
		gtk_signal_connect (GTK_OBJECT (wid), "clicked",
								  GTK_SIGNAL_FUNC (toolbox_color), (gpointer) i);
		gtk_style_unref (style);
	}

	wid = toolbox_button ("", "%U", toolbox, _("Underline"));
	gtk_label_parse_uline (GTK_LABEL (GTK_BIN (wid)->child), "_U");

	/*wid =*/ toolbox_button ("B", "%B", toolbox, _("Bold"));
	/*style = gtk_style_new ();
	gdk_font_unref (style->font);
   style->font = font_bold;
   gdk_font_ref (font_bold); 
	gtk_widget_set_style (GTK_BIN (wid)->child, style);
	gtk_style_unref (style);*/

	toolbox_button ("A", "%O", toolbox, _("Plain Text"));

	wid = gtk_button_new_with_label ("<");
	gtk_signal_connect (GTK_OBJECT (wid), "clicked",
							  GTK_SIGNAL_FUNC (maingui_toolbox), toolbox);
	gtk_box_pack_start (GTK_BOX (box), wid, 0, 0, 1);
	if (prefs.inputgad_superfocus)
		show_and_unfocus (wid);
	else
		gtk_widget_show (wid);
	add_tip (wid, _("Open Toolbox"));
}

/*static void
ul_keypress_cb (GtkWidget *widget, GdkEventKey *event, session *sess)
{
	struct User *user;
	GSList *list = sess->userlist;
	int row = 0;

	while (list)
	{
		user = list->data;
		if (user->nick[0] == event->keyval)
		{
			gtk_clist_moveto (GTK_CLIST (sess->gui->namelistgad), row, 0, 0.5, 0);
			break;
		}
		list = list->next;
		row++;
	}
}*/

static GtkWidget *
create_infoframe (GdkPixmap *pix, GdkPixmap *mask, GtkWidget *box)
{
	GtkWidget *frame, *label, *hbox, *p;

	frame = gtk_frame_new (0);
	gtk_frame_set_shadow_type ((GtkFrame*)frame, GTK_SHADOW_OUT);
	gtk_container_add (GTK_CONTAINER (box), frame);
	gtk_widget_show (frame);

	hbox = gtk_hbox_new (0, 0);
	gtk_container_add (GTK_CONTAINER (frame), hbox);
	gtk_widget_show (hbox);

	if (pix)
	{
		p = gtk_pixmap_new (pix, mask);
		gtk_container_add (GTK_CONTAINER (hbox), p);
		gtk_widget_show (p);
	}

	label = gtk_label_new (" ");
	gtk_container_add (GTK_CONTAINER (hbox), label);
	gtk_widget_show (label);

	return label;
}

static void
maingui_make_changad (session *sess)
{
	if (sess->type == SESS_CHANNEL || sess->type == SESS_SERVER)
	{
		if (sess->channel[0])
			sess->gui->changad = gtk_label_new (sess->channel);
		else
			sess->gui->changad = gtk_label_new (_("<none>"));
		gtk_box_pack_start (GTK_BOX (sess->gui->tbox), sess->gui->changad,
								  FALSE, FALSE, 5);
		gtk_box_reorder_child (GTK_BOX (sess->gui->tbox), sess->gui->changad, 1);
		gtk_widget_show (sess->gui->changad);
	} else
	{
		sess->gui->changad = NULL;
	}
}

/* a session got delinked or relinked */

void
link_cb (GtkWidget *win, session *sess)
{
#ifndef USE_GNOME
	GtkWidget *wid = NULL;
#endif

	sess->gui->window = win;

	if (sess->is_tab)
	{
		sess->is_tab = FALSE;

		if (sess->type == SESS_CHANNEL)
		{
			/* turned into a toplevel window, so create a menubar */
#ifdef USE_GNOME
			sess->gui->menu = createmenus (GNOME_APP (sess->gui->window), sess, 1);
#else
			wid = gtk_handle_box_new ();
			gtk_box_pack_start (GTK_BOX (sess->gui->vbox), wid, FALSE, FALSE, 0);
			gtk_box_reorder_child (GTK_BOX (sess->gui->vbox), wid, 0);

			sess->gui->menu = createmenus (0, sess, 1);
			gtk_container_add (GTK_CONTAINER (wid), sess->gui->menu);
			gtk_widget_show_all (wid);
#endif
		}
		gtk_signal_connect (GTK_OBJECT (win), "focus_in_event",
								  GTK_SIGNAL_FUNC (focus_in), sess);
		gtk_signal_connect_object (GTK_OBJECT (win), "configure_event",
											GTK_SIGNAL_FUNC (maingui_configure),
											GTK_OBJECT (sess));

		/* toplevel windows can't use the notebook label as changad */
		maingui_make_changad (sess);

		/* Hide menu if it is turned off */
		if (prefs.hidemenu)
#ifdef USE_GNOME
			menu_showhide ();
#else
			if (wid)
				gtk_widget_hide (wid);
#endif

#ifdef USE_ZVT
		if (sess->type == SESS_SHELL)
			menu_newshell_set_palette (sess);
		else
#endif
			gtk_xtext_refresh (GTK_XTEXT (sess->gui->textgad), 1);

	} else
	{
		sess->is_tab = TRUE;

		if (sess->gui->menu)
		{
#ifndef USE_GNOME
			/* this will destroy the HandleBox and its children */
			gtk_widget_destroy (sess->gui->menu->parent);
#else
			/* causes GTK_IS_WIDGET failed warning */
			/*gtk_widget_destroy (sess->gui->menu);*/
#endif
			sess->gui->menu = NULL;
		}

		/* tabs don't have the changad near the topic bar */
		if (sess->gui->changad != NULL)
			gtk_widget_destroy (sess->gui->changad);
		sess->gui->changad = wins_get_notebooklabel (sess->gui->window);

#ifdef USE_GNOME
		/* why does only gnome need this ?? */
		/* dialog windows don't have this box */
		if (sess->gui->userlistbox && sess->type != SESS_DIALOG)
		{
			gtk_widget_realize (sess->gui->userlistbox);
			gdk_window_set_background (sess->gui->userlistbox->window,
												&sess->gui->userlistbox->
												style->bg[GTK_STATE_NORMAL]);
			gdk_window_set_back_pixmap (main_book->window, 0, 0);
		}
		menu_showhide ();
#endif
	}
}

/* called when sess->gui->window is destroyed */

static void
window_destroy_cb (session *sess)
{
	kill_session_callback (sess);	/* xchat.c */

	if (main_window != NULL)
	{
		/* if no tabs left, destroy main window */
		if (gtk_notebook_get_nth_page (GTK_NOTEBOOK (main_book), 0) == NULL)
		{
			gtk_widget_destroy (main_window);
			main_window = NULL;
		}
	}
}

void
create_window (struct session *sess)
{
	GtkWidget *leftpane, *rightpane;
	GtkWidget *vbox, *tbox, *bbox, *nlbox, *wid, *infbox;
	int justopened = FALSE;

	if (sess->type == SESS_DIALOG)
	{
		new_dialog (sess);
		if (prefs.treeview)
			tree_update ();
		return;
	}
	if (!sess->server->front_session)
		sess->server->front_session = sess;

	if (prefs.tabchannels)
	{
		sess->is_tab = TRUE;
		if (!main_window)
		{
			justopened = TRUE;
			gui_make_tab_window (sess, NULL);
		}
		sess->gui->window = wins_new (_("<none>"), "",
									window_destroy_cb, sess, link_cb, sess,
									TRUE, FALSE, NULL);
		if (!current_tab)
			current_tab = sess;
	} else
	{
		sess->gui->window = wins_new (_("<none>"), "",
									window_destroy_cb, sess, link_cb, sess,
									FALSE, FALSE, NULL);
		gtk_widget_set_usize (sess->gui->window, prefs.mainwindow_width,
										prefs.mainwindow_height);
		gtk_signal_connect ((GtkObject *) sess->gui->window, "focus_in_event",
								  GTK_SIGNAL_FUNC (focus_in), sess);
	}

	fe_set_title (sess);
	palette_alloc (sess->gui->window);

	sess->gui->vbox = vbox = wins_get_vbox (sess->gui->window);

	if (!prefs.tabchannels)
	{
#ifdef USE_GNOME
		sess->gui->menu = createmenus (sess->gui->window, sess, 1);
#else
		sess->gui->menu = createmenus (0, sess, 1);
		gtk_widget_show (sess->gui->menu);

		wid = gtk_handle_box_new ();
		gtk_container_add (GTK_CONTAINER (wid), sess->gui->menu);
		gtk_box_pack_start (GTK_BOX (vbox), wid, FALSE, TRUE, 0);
		if (!prefs.hidemenu)
			gtk_widget_show (wid);
#endif
	}

	sess->gui->tbox = tbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), tbox, FALSE, FALSE, 2);
	if (prefs.topicbar)
		gtk_widget_show (tbox);

	wins_create_linkbuttons (sess->gui->window, tbox);

	if (!prefs.tabchannels)
		maingui_make_changad (sess);

	sess->gui->topicgad = gtk_entry_new ();
	gtk_signal_connect (GTK_OBJECT (sess->gui->topicgad), "activate",
							  GTK_SIGNAL_FUNC (handle_topicgad), sess);
	gtk_container_add (GTK_CONTAINER (tbox), sess->gui->topicgad);
	if (prefs.inputgad_superfocus)
		show_and_unfocus (sess->gui->topicgad);
	else
		gtk_widget_show (sess->gui->topicgad);

	add_tip (sess->gui->topicgad, _("The channel topic"));

#ifdef USE_GNOMEE
	wid = gtkutil_button (sess->gui->window, GNOME_STOCK_PIXMAP_FORWARD, 0,
								 userlist_hide, sess, 0);
#else
	if (prefs.hideuserlist)
		wid = gtk_button_new_with_label ("<");
	else
		wid = gtk_button_new_with_label (">");
	gtk_signal_connect (GTK_OBJECT (wid), "clicked",
							  GTK_SIGNAL_FUNC (userlist_hide), (gpointer) sess);
#endif
	gtk_box_pack_end (GTK_BOX (tbox), wid, 0, 0, 0);
	show_and_unfocus (wid);
	add_tip (wid, _("Hide/Show Userlist"));

	if (prefs.chanmodebuttons)
	{
		sess->gui->key_entry = gtk_entry_new_with_max_length (16);
		gtk_widget_set_usize (sess->gui->key_entry, 30, -1);
		gtk_box_pack_end (GTK_BOX (tbox), sess->gui->key_entry, 0, 0, 0);
		gtk_signal_connect (GTK_OBJECT (sess->gui->key_entry), "activate",
								  GTK_SIGNAL_FUNC (key_entry), (gpointer) sess);
		if (prefs.inputgad_superfocus)
			show_and_unfocus (sess->gui->key_entry);
		else
			gtk_widget_show (sess->gui->key_entry);

		sess->gui->flag_k = add_flag_wid (_("Keyword"), tbox, _("K"), sess);

		sess->gui->limit_entry = gtk_entry_new_with_max_length (10);
		gtk_widget_set_usize (sess->gui->limit_entry, 30, -1);
		gtk_box_pack_end (GTK_BOX (tbox), sess->gui->limit_entry, 0, 0, 0);
		gtk_signal_connect (GTK_OBJECT (sess->gui->limit_entry), "activate",
								  GTK_SIGNAL_FUNC (limit_entry), (gpointer) sess);
		if (prefs.inputgad_superfocus)
			show_and_unfocus (sess->gui->limit_entry);
		else
			gtk_widget_show (sess->gui->limit_entry);

		sess->gui->flag_l = add_flag_wid (_("User Limit"), tbox, "L", sess);
		sess->gui->flag_b = add_flag_wid (_("Ban List"), tbox, "B", sess);
		sess->gui->flag_m = add_flag_wid (_("Moderated"), tbox, "M", sess);
		sess->gui->flag_p = add_flag_wid (_("Private"), tbox, "P", sess);
		sess->gui->flag_i = add_flag_wid (_("Invite Only"), tbox, "I", sess);
		sess->gui->flag_s = add_flag_wid (_("Secret"), tbox, "S", sess);
		sess->gui->flag_n = add_flag_wid (_("No outside messages"), tbox, "N", sess);
		sess->gui->flag_t = add_flag_wid (_("Topic Protection"), tbox, "T", sess);
	} else
		sess->gui->flag_wid[0] = 0;

	leftpane = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (leftpane);

	if (prefs.paned_userlist)
	{
		sess->gui->paned = gtk_hpaned_new ();
		gtk_container_add (GTK_CONTAINER (vbox), sess->gui->paned);
		gtk_widget_show (sess->gui->paned);
	}
	rightpane = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (rightpane);
	sess->gui->userlistbox = rightpane;

	if (prefs.paned_userlist)
	{
		gtk_paned_pack1 (GTK_PANED (sess->gui->paned), leftpane, TRUE, TRUE);
		gtk_paned_pack2 (GTK_PANED (sess->gui->paned), rightpane, FALSE, TRUE);
#ifndef WIN32	/* not available in gtk 1.3 */
		gtk_paned_set_gutter_size (GTK_PANED (sess->gui->paned), 4);
#endif
	} else
	{
		wid = gtk_hbox_new (0, 2);
		gtk_container_add (GTK_CONTAINER (vbox), wid);
		gtk_widget_show (wid);
		gtk_container_add (GTK_CONTAINER (wid), leftpane);
		gtk_box_pack_end (GTK_BOX (wid), rightpane, 0, 0, 0);
	}

	sess->gui->nl_box = nlbox = gtk_vbox_new (FALSE, 1);
	gtk_container_add (GTK_CONTAINER (rightpane), nlbox);
	gtk_widget_show (nlbox);

	sess->gui->namelistinfo = infbox = gtk_hbox_new (0, 0);
	gtk_box_pack_start (GTK_BOX (nlbox), infbox, 0, 0, 0);

	sess->gui->namelistinfo_o = create_infoframe (pix_op, mask_op, infbox);
	sess->gui->namelistinfo_v = create_infoframe (pix_voice, mask_voice, infbox);
	sess->gui->namelistinfo_t = create_infoframe (0, 0, infbox);

	maingui_create_textlist (sess, leftpane);
	sess->gui->leftpane = leftpane;

	if (prefs.showhostname_in_userlist)
	{
		sess->gui->namelistgad =
			gtkutil_clist_new (2, 0, nlbox, GTK_POLICY_AUTOMATIC,
									 maingui_userlist_selected, sess, 0, 0,
									 GTK_SELECTION_MULTIPLE);
		gtk_clist_set_column_auto_resize ((GtkCList *) sess->gui->namelistgad,
													 0, TRUE);
		gtk_clist_set_column_auto_resize ((GtkCList *) sess->gui->namelistgad,
													 1, TRUE);
	} else
	{
		sess->gui->namelistgad =
			gtkutil_clist_new (1, 0, nlbox, GTK_POLICY_AUTOMATIC,
									 maingui_userlist_selected, sess, 0, 0,
									 GTK_SELECTION_MULTIPLE);
	}

	GTK_WIDGET_UNSET_FLAGS (sess->gui->namelistgad, GTK_CAN_FOCUS);

	if (prefs.style_namelistgad)
		gtk_widget_set_style (sess->gui->namelistgad, inputgad_style);

#ifdef USE_GNOME
	/* set up drops */
	gtk_drag_dest_set (sess->gui->namelistgad, GTK_DEST_DEFAULT_ALL,
							 dnd_targets, 1,
							 GDK_ACTION_MOVE | GDK_ACTION_COPY | GDK_ACTION_LINK);
	gtk_signal_connect (GTK_OBJECT (sess->gui->namelistgad), "drag_motion",
							  GTK_SIGNAL_FUNC (userlist_dnd_motion), 0);
	gtk_signal_connect (GTK_OBJECT (sess->gui->namelistgad), "drag_leave",
							  GTK_SIGNAL_FUNC (userlist_dnd_leave), 0);
	gtk_signal_connect (GTK_OBJECT (sess->gui->namelistgad),
							  "drag_data_received",
							  GTK_SIGNAL_FUNC (userlist_dnd_drop), sess);
#endif

	gtk_clist_set_selection_mode (GTK_CLIST (sess->gui->namelistgad),
											GTK_SELECTION_MULTIPLE);
	gtk_clist_set_column_width (GTK_CLIST (sess->gui->namelistgad), 0, 10);
	gtk_widget_set_usize (sess->gui->namelistgad->parent, 115, 0);
	gtk_signal_connect (GTK_OBJECT (sess->gui->namelistgad),
							  "button_press_event", GTK_SIGNAL_FUNC (ul_button_rel),
							  sess);
	/*gtk_signal_connect (GTK_OBJECT (sess->gui->namelistgad),
							  "key_press_event", GTK_SIGNAL_FUNC (ul_keypress_cb),
							  sess);*/

	if ((prefs.lagometer & 2) || (prefs.throttlemeter & 2))
	{
		infbox = gtk_hbox_new (0, 0);
		gtk_box_pack_start (GTK_BOX (nlbox), infbox, 0, 0, 0);
		gtk_widget_show (infbox);
	}

	if (prefs.lagometer & 1)
	{
		sess->gui->lagometer = wid = gtk_progress_bar_new ();
		gtk_widget_set_usize (wid, 1, 8);
		gtk_box_pack_start (GTK_BOX (nlbox), wid, 0, 0, 0);
		gtk_widget_show (wid);
	}
	if (prefs.lagometer & 2)
	{
		sess->gui->laginfo = wid = create_infoframe (0, 0, infbox);
		gtk_label_set_text ((GtkLabel *) wid, "Lag");
	}

	if (prefs.throttlemeter & 1)
	{
		sess->gui->throttlemeter = wid = gtk_progress_bar_new ();
		gtk_widget_set_usize (wid, 1, 8);
		gtk_box_pack_start (GTK_BOX (nlbox), wid, 0, 0, 0);
		gtk_widget_show (wid);
	}
	if (prefs.throttlemeter & 2)
	{
		sess->gui->throttleinfo = wid = create_infoframe (0, 0, infbox);
		gtk_label_set_text ((GtkLabel *) wid, "Throttle");
	}

	if (prefs.userlistbuttons)
		maingui_createbuttons (sess);
	else
		sess->gui->button_box = 0;

	/* bottom hbox */
	bbox = gtk_hbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (bbox), 0);
	gtk_box_pack_end (GTK_BOX (vbox), bbox, FALSE, TRUE, 2);
	gtk_widget_show (bbox);

	if (prefs.nickgad)
	{
		sess->gui->op_box = gtk_hbox_new (0, 0);
		gtk_box_pack_start (GTK_BOX (bbox), sess->gui->op_box, FALSE, FALSE, 2);
		gtk_widget_show (sess->gui->op_box);

		sess->gui->nickgad = gtk_label_new (sess->server->nick);
		gtk_box_pack_start (GTK_BOX (bbox), sess->gui->nickgad, FALSE, FALSE,
								  4);
		gtk_widget_show (sess->gui->nickgad);
	}

	sess->gui->inputgad = gtk_entry_new_with_max_length (2048);
	gtk_container_add (GTK_CONTAINER (bbox), sess->gui->inputgad);
	gtk_signal_connect (GTK_OBJECT (sess->gui->inputgad), "key_press_event",
							  GTK_SIGNAL_FUNC (key_handle_key_press), sess);
	if (prefs.style_inputbox)
		gtk_widget_set_style (sess->gui->inputgad, inputgad_style);
	gtk_widget_show (sess->gui->inputgad);
	if (prefs.newtabstofront || justopened)
		gtk_widget_grab_focus (sess->gui->inputgad);

	gui_create_toolbox (sess, bbox);

	gtk_widget_show (sess->gui->window);

	if (prefs.tabchannels)
	{
		sess->gui->changad = wins_get_notebooklabel (sess->gui->window);
		gtk_widget_realize (sess->gui->textgad);

		if (justopened)
		{
			gtk_widget_show (main_window);
			if (prefs.mainwindow_left || prefs.mainwindow_top)
				gdk_window_move (main_window->window,
									  prefs.mainwindow_left, prefs.mainwindow_top);
		}

		if (!redtab_style)
			maingui_init_styles (sess->gui->changad->style);

		if (prefs.newtabstofront && !justopened)
			wins_bring_tofront (sess->gui->window);

		/* make switching tabs super smooth! */
		gtk_widget_realize (rightpane);
		gdk_window_set_background (rightpane->window,
											&rightpane->style->bg[GTK_STATE_NORMAL]);
		gdk_window_set_back_pixmap (main_book->window, 0, 0);
		gdk_window_set_background (((GtkNotebook*)main_book)->panel,
											&main_book->style->bg[GTK_STATE_NORMAL]);

	} else
	{
		if (!redtab_style)
			maingui_init_styles (gtk_widget_get_style (sess->gui->changad));

		if (prefs.mainwindow_left || prefs.mainwindow_top)
			gdk_window_move (sess->gui->window->window,
								  prefs.mainwindow_left, prefs.mainwindow_top);
	}

	if (prefs.hideuserlist)
	{
		userlist_hide (0, sess);
		if (sess->gui->paned)
			sess->userlisthidden = GTK_PANED (sess->gui->paned)->handle_xpos;
	}

#ifdef USE_GNOME
	if (!sess->is_tab || justopened)
		menu_showhide ();
#endif

	fe_set_nonchannel (sess, FALSE);
}

void
fe_session_callback (struct session *sess)
{
	tree_update ();

#ifdef USE_PANEL
	if (sess->gui->panel_button)
		gtk_widget_destroy (sess->gui->panel_button);
#endif

	if (sess->gui->bar)
		fe_progressbar_end (sess);

	if (menu_sess == sess && sess_list)
		menu_sess = (struct session *) sess_list->data;
}

/*void
fe_server_callback (struct server *serv)
{
	if (serv->gui->rawlog_window)
	{ 
		we don't want the "destroy" callback to be called
		gtk_signal_disconnect_by_data (GTK_OBJECT (serv->gui->rawlog_window), serv);
		gtk_widget_destroy (serv->gui->rawlog_window);
	} 
}*/

static void
my_gtk_togglebutton_state (GtkWidget *wid, int state)
{
	gpointer udata;

	/* remove the userdata so the callback ignores this event */
	udata = gtk_object_get_user_data (GTK_OBJECT (wid));
	gtk_object_set_user_data (GTK_OBJECT (wid), 0);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (wid), state);

	/* now put it back */
	gtk_object_set_user_data (GTK_OBJECT (wid), udata);
}

void
fe_update_mode_buttons (struct session *sess, char mode, char sign)
{
	int state, i;

	if (sign == '+')
		state = TRUE;
	else
		state = FALSE;

	if (sess->gui->flag_wid[0])
	{
		for (i = 0; i < NUM_FLAG_WIDS - 1; i++)
		{
			if (chan_flags[i] == mode)
			{
				if (GTK_TOGGLE_BUTTON (sess->gui->flag_wid[i])->active != state)
					my_gtk_togglebutton_state (sess->gui->flag_wid[i], state);
			}
		}
	}

/*	fe_set_title (sess);*/
}

void
handle_inputgad (GtkWidget * igad, struct session *sess)
{
	char *cmd = gtk_entry_get_text (GTK_ENTRY (igad));

	if (cmd[0] == 0)
		return;

	handle_multiline (sess, cmd, TRUE, FALSE);
	gtk_entry_set_text (GTK_ENTRY (igad), "");
}
