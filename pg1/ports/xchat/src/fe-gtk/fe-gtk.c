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
#include "../../config.h"
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#include <stdlib.h>
#include <unistd.h>

#include "fe-gtk.h"
#include "../common/xchat.h"
#include "../common/fe.h"
#include "../common/util.h"
#include "../common/text.h"
#include "../common/cfgfiles.h"
#include "../common/xchatc.h"
#include "gtkutil.h"
#include "maingui.h"
#include "pixmaps.h"
#include "xtext.h"
#include "palette.h"
#include "menu.h"
#include "notifygui.h"
#include "textgui.h"
#include "fkeys.h"
#include "urlgrab.h"

#ifdef USE_PANEL
#include "panel.h"
#include <applet-widget.h>
#endif

#ifdef USE_XLIB
#include <gdk/gdkx.h>
#include <gtk/gtkinvisible.h>
#endif

#ifdef USE_GDK_PIXBUF
#include <gdk-pixbuf/gdk-pixbuf.h>
#endif

GdkFont *font_normal;
GdkFont *dialog_font_normal;
GdkPixmap *channelwin_pix;
GdkPixmap *dialogwin_pix;


extern void open_server_list (GtkWidget *wid, gpointer sess, int dontshow);
extern void serverlist_autoconnect (struct session *sess, int);


#ifdef USE_XLIB

static GtkWidget *proxy_invisible;

static void
redraw_trans_xtexts (void)
{
	GSList *list = sess_list;
	struct session *sess;
	while (list)
	{
		sess = (struct session *) list->data;
#ifdef USE_ZVT
		if (sess->type == SESS_SHELL)
		{
			menu_newshell_set_palette (sess);
			gtk_widget_queue_draw (sess->gui->textgad);
		} else
#endif
			if (GTK_XTEXT (sess->gui->textgad)->transparent)
				gtk_xtext_refresh (GTK_XTEXT (sess->gui->textgad), 1);
		list = list->next;
	}
}

static gboolean
handle_property_notify (GtkWidget *widget, GdkEventProperty *event,
								gpointer user_data)
{
	static Atom prop = None;

	if (prop == None)
		prop = XInternAtom (GDK_DISPLAY (), "_XROOTPMAP_ID", True);

	if (event->atom == prop)
		redraw_trans_xtexts ();

	return FALSE;
}

#endif

int
fe_args (int argc, char *argv[])
{
#ifdef USE_GNOME
	struct poptOption options[] = {
		{"cfgdir", 'd', POPT_ARG_STRING, 0, 0, _("Config dir"), 0},
		{"noauto", 'a', POPT_ARG_NONE, 0, 0, _("Don't Auto connect"), 0},
#ifdef USE_PANEL
		{"no-panel", 'n', POPT_ARG_NONE, 0, 0, _("Don't use GNOME Panel"), 0},
#endif
		POPT_AUTOHELP {0, '\0', 0, 0}
	};
#endif

	if (argc > 1)
	{
		if (!strcasecmp (argv[1], "-a") || !strcasecmp (argv[1], "--noauto"))
		{
			auto_connect = 0;
		}
#ifdef USE_PANEL
		if (!strcasecmp (argv[1], "-n") || !strcasecmp (argv[1], "--no-panel"))
		{
			nopanel = TRUE;
		}
#endif
		if (!strcasecmp (argv[1], "-v") || !strcasecmp (argv[1], "--version"))
		{
			printf (PACKAGE" "VERSION"\n");
			return 0;
		}
#ifndef USE_GNOME
#ifdef ENABLE_NLS
		bindtextdomain (PACKAGE, PREFIX"/share/locale");
		textdomain (PACKAGE);
#endif
		if (!strcasecmp (argv[1], "-h") || !strcasecmp (argv[1], "--help"))
		{
			printf(_("%s %s Options:\n\n"
					"   --cfgdir <dir> -d\t : use a different config dir\n"
					"   --noauto       -a\t : don't auto connect\n"
					"   --version      -v\t : show version information\n"
                ), PACKAGE, VERSION);
			return 0;
		}
#endif
	}
#ifdef ENABLE_NLS
#ifdef USE_GNOME
	bindtextdomain (PACKAGE, PREFIX"/share/locale");
	textdomain (PACKAGE);
#endif
#endif

	if (argc > 2)
	{
		if (!strcasecmp (argv[1], "-d") || !strcasecmp (argv[1], "--cfgdir"))
		{
			xdir = strdup (argv[2]);
			if (xdir[strlen (xdir) - 1] == '/')
				xdir[strlen (xdir) - 1] = 0;
		}
	}

#ifndef USE_GNOME
	gtk_set_locale ();
#endif

#ifdef USE_PANEL
	if (nopanel)
		gnome_init_with_popt_table (argv[0], VERSION, argc, argv, options, 0, 0);
	else {
		CORBA_Environment ev;
		CORBA_exception_init (&ev);

		gnome_CORBA_init_with_popt_table (argv[0], VERSION,
						  &argc, argv, options, 0, 0,
						  GNORBA_INIT_SERVER_FUNC, &ev);
		CORBA_exception_free (&ev);
	}
#else
#ifdef USE_GNOME
	gnome_init_with_popt_table (argv[0], VERSION, argc, argv, options, 0, 0);
#else
	gtk_init (&argc, &argv);
#endif
#endif

#ifndef USE_GNOME
#ifdef USE_GDK_PIXBUF
	gdk_rgb_init();
#endif
#endif

#ifdef USE_XLIB
	proxy_invisible = gtk_invisible_new ();
	gtk_widget_show (proxy_invisible);

	/* Make the root window send events to the invisible proxy widget */
	gdk_window_set_user_data (GDK_ROOT_PARENT (), proxy_invisible);
	
	/* Select for PropertyNotify events from the root window */
	XSelectInput (GDK_DISPLAY (), GDK_ROOT_WINDOW (), PropertyChangeMask);

	gtk_signal_connect (GTK_OBJECT (proxy_invisible), "property-notify-event",
							 GTK_SIGNAL_FUNC (handle_property_notify), NULL);
#endif

	return 1;
}

GdkFont *
my_font_load (char *fontname)
{
	GdkFont *font;
	char temp[256];

	if (!*fontname)
		fontname = "fixed";
	if (prefs.use_fontset)
		font = gdk_fontset_load (fontname);
	else
		font = gdk_font_load (fontname);
	if (!font)
	{
		snprintf (temp, sizeof (temp), _("Cannot open font:\n\n%s"), fontname);
		gtkutil_simpledialog (temp);
		font = gdk_font_load ("fixed");
		if (!font)
		{
			g_error (_("gdk_font_load failed"));
			gtk_exit (0);
		}
	}
	return font;
}

GtkStyle *
create_inputgad_style (void)
{
	GtkStyle *style;

	style = gtk_style_new ();

	gdk_font_unref (style->font);
	gdk_font_ref (font_normal);
	style->font = font_normal;

	style->base[GTK_STATE_NORMAL] = colors[19];
	style->bg[GTK_STATE_NORMAL] = colors[19];
	style->fg[GTK_STATE_NORMAL] = colors[18];

	return style;
}

void
fe_init (void)
{
	font_normal = my_font_load (prefs.font_normal);
	dialog_font_normal = my_font_load (prefs.dialog_font_normal);

	palette_load ();
	key_init ();
	pixmaps_init ();

	channelwin_pix = pixmap_load_from_file (prefs.background);
	dialogwin_pix = pixmap_load_from_file (prefs.background_dialog);
	inputgad_style = create_inputgad_style ();
}

void
fe_main (void)
{
#ifdef USE_PANEL
	if (nopanel)
		gtk_main ();
	else
		applet_widget_gtk_main ();
#else
	gtk_main ();
#endif

	/* sleep for 3 seconds so any QUIT messages are not lost. The  */
	/* GUI is closed at this point, so the user doesn't even know! */
	sleep (3);
}

void
fe_cleanup (void)
{
	palette_save ();

	if (prefs.autosave_url)
		url_autosave ();

#ifdef USE_PANEL
	panel_cleanup ();
#endif
}

void
fe_exit (void)
{
	gtk_main_quit ();
}

int
fe_timeout_add (int interval, void *callback, void *userdata)
{
	return g_timeout_add (interval, (GSourceFunc) callback, userdata);
}

void
fe_timeout_remove (int tag)
{
	g_source_remove (tag);
}

void
fe_new_window (struct session *sess)
{
	sess->gui = malloc (sizeof (struct session_gui));
	memset (sess->gui, 0, sizeof (struct session_gui));
	create_window (sess);
}

void
fe_new_server (struct server *serv)
{
	serv->gui = malloc (sizeof (struct server_gui));
	memset (serv->gui, 0, sizeof (struct server_gui));
}

static void
null_this_var (GtkWidget * unused, GtkWidget ** dialog)
{
	*dialog = 0;
}

void
fe_message (char *msg, int wait)
{
	GtkWidget *dialog;

	dialog = gtkutil_simpledialog (msg);
	if (wait)
	{
		gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
								  null_this_var, &dialog);
		while (dialog)
			gtk_main_iteration ();
	}
}

void
fe_input_remove (int tag)
{
	g_source_remove (tag);
}

int
fe_input_add (int sok, int read, int write, int ex, void *func, void *data)
{
	int tag, type = 0;
	GIOChannel *channel;

#ifdef WIN32
	if (read == 3)
		channel = g_io_channel_win32_new_fd (sok);
	else
		channel = g_io_channel_win32_new_socket (sok);
#else
	channel = g_io_channel_unix_new (sok);
#endif

	if (read)
		type |= G_IO_IN | G_IO_HUP | G_IO_ERR;
	if (write)
		type |= G_IO_OUT | G_IO_ERR;
	if (ex)
		type |= G_IO_PRI;

	tag = g_io_add_watch (channel, type, (GIOFunc) func, data);
	g_io_channel_unref (channel);

	return tag;
}

void
fe_set_topic (struct session *sess, char *topic)
{
	char buf[512];

	gtk_entry_set_text (GTK_ENTRY (sess->gui->topicgad), topic);

	if (sess->type == SESS_CHANNEL)
	{
		snprintf (buf, sizeof (buf), _("Topic for %s is: %s"), sess->channel,
					 topic);
		add_tip (sess->gui->topicgad, buf);
	}
}

void
fe_set_hilight (struct session *sess)
{
#ifdef USE_PANEL
	if (sess->gui->panel_button)
		gtk_widget_set_style (GTK_BIN (sess->gui->panel_button)->child,
									 bluetab_style);
#endif
	gtk_widget_set_style (sess->gui->changad, bluetab_style);
	if (prefs.treeview)
		tree_blue_style (sess);
}

void
fe_update_channel_key (struct session *sess)
{
	if (sess->gui->flag_wid[0])	/* channel mode buttons enabled? */
		gtk_entry_set_text ((GtkEntry*)sess->gui->key_entry, sess->channelkey);
	fe_set_title (sess);
}

void
fe_update_channel_limit (struct session *sess)
{
	char tmp[16];

	if (sess->gui->flag_wid[0])	/* channel mode buttons enabled? */
	{
		sprintf (tmp, "%d", sess->limit);
		gtk_entry_set_text ((GtkEntry*)sess->gui->limit_entry, tmp);
	}
}

int
fe_is_chanwindow (struct server *serv)
{
	if (!serv->gui->chanlist_window)
		return 0;
	return 1;
}

int
fe_is_banwindow (struct session *sess)
{
   if (!sess->gui->banlist_window)
     return 0;
   return 1;
}

void
fe_chan_list_end (struct server *serv)
{
	gtk_widget_set_sensitive (serv->gui->chanlist_refresh, TRUE);
}

void
fe_notify_update (char *name)
{
	if (name)
		update_all_of (name);
	else
		notify_gui_update ();
}

void
fe_text_clear (struct session *sess)
{
	gtk_xtext_clear (GTK_XTEXT (sess->gui->textgad));
}

void
fe_close_window (struct session *sess)
{
	gtk_widget_destroy (sess->gui->window);
}

static int
updatedate_bar (struct session *sess)
{
	static int type = 0;
	static float pos = 0;

	if (!is_session (sess))
		return 0;

	pos += 0.05;
	if (pos >= 0.99)
	{
		if (type == 0)
		{
			type = 1;
			gtk_progress_bar_set_orientation ((GtkProgressBar *) sess->gui->bar,
														 GTK_PROGRESS_RIGHT_TO_LEFT);
		} else
		{
			type = 0;
			gtk_progress_bar_set_orientation ((GtkProgressBar *) sess->gui->bar,
														 GTK_PROGRESS_LEFT_TO_RIGHT);
		}
		pos = 0.05;
	}
	gtk_progress_bar_update ((GtkProgressBar *) sess->gui->bar, pos);
	return 1;
}

void
fe_progressbar_start (struct session *sess)
{
	if (sess->gui->op_box)
	{
		sess->gui->bar = gtk_progress_bar_new ();
		gtk_box_pack_start (GTK_BOX (sess->gui->op_box), sess->gui->bar, 0, 0,
								  0);
		gtk_widget_show (sess->gui->bar);
		sess->server->bartag = fe_timeout_add (50, updatedate_bar, sess);
	}
}

void
fe_progressbar_end (struct session *sess)
{
	struct server *serv;
	GSList *list = sess_list;

	if (sess)
	{
		serv = sess->server;
		while (list)				  /* check all windows that use this server and  *
										   * remove the connecting graph, if it has one. */
		{
			sess = (struct session *) list->data;
			if (sess->server == serv && sess->gui->bar)
			{
				if (GTK_IS_WIDGET (sess->gui->bar))
					gtk_widget_destroy (sess->gui->bar);
				sess->gui->bar = 0;
				fe_timeout_remove (sess->server->bartag);
			}
			list = list->next;
		}
	}
}

void
fe_print_text (struct session *sess, char *text)
{
	int indent;

	if (sess->type == SESS_DIALOG)
		indent = prefs.dialog_indent_nicks;
	else
		indent = prefs.indent_nicks;

	PrintTextRaw (sess->gui->textgad, text, indent);

	if (prefs.limitedtabhighlight && !sess->highlight_tab)
		return;

	sess->highlight_tab = FALSE;

	if (!sess->new_data && !sess->nick_said && sess != current_tab)
	{
#ifdef USE_PANEL

		if (sess->gui->panel_button)
			gtk_widget_set_style (GTK_BIN (sess->gui->panel_button)->child,
										 redtab_style);
#endif
		if (prefs.treeview)
			tree_red_style (sess);
	}

	if (!sess->new_data && sess != current_tab &&
		 sess->is_tab && !sess->nick_said)
	{
		sess->new_data = TRUE;
		gtk_widget_set_style (sess->gui->changad, redtab_style);
	}
}

void
fe_beep (void)
{
	gdk_beep ();
}

int
fe_is_beep (struct session *sess)
{
	if (GTK_TOGGLE_BUTTON (sess->gui->beepbutton)->active)
		return 1;
	return 0;
}

int
fe_is_confmode (struct session *sess)
{
	if (GTK_TOGGLE_BUTTON (sess->gui->confbutton)->active)
		return 1;
	return 0;
}

void
fe_lastlog (session *sess, session *lastlog_sess, char *sstr)
{
	textentry *ent;

	ent = GTK_XTEXT(sess->gui->textgad)->text_first;
	if (!ent)
	{
		PrintText (lastlog_sess, "Search buffer is empty.\n");
	} else
	{
		do
		{
			if (nocasestrstr (ent->str, sstr))
				PrintText (lastlog_sess, ent->str);
			ent = ent->next;
		}
		while (ent);
	}
}

void
fe_set_lag (server *serv, int lag)
{
	GSList *list = sess_list;
	session *sess;
	gdouble per;
	char tip[64];
	unsigned long nowtim;

	if (lag == -1)
	{
		if (!serv->lag_sent)
			return;
		nowtim = make_ping_time ();
		lag = (nowtim - serv->lag_sent) / 100000;
	}

	per = (double)((double)lag / (double)40);
	if (per > 1.0)
		per = 1.0;

	while (list)
	{
		sess = list->data;
		if (sess->server == serv)
		{
			if (sess->gui->lagometer)
			{
				gtk_progress_bar_update ((GtkProgressBar *) sess->gui->lagometer, per);
			}
			if (sess->gui->laginfo)
			{
				snprintf (tip, sizeof(tip) - 1, "%s%d.%ds",
					serv->lag_sent ? "+" : "", lag / 10, lag % 10);
				gtk_label_set_text ((GtkLabel *) sess->gui->laginfo, tip);
			}
		}
		list = list->next;
	}
}

void
fe_set_throttle (server *serv)
{
	GSList *list = sess_list;
	struct session *sess;
	float per;
	char tbuf[64];

	per = (float) serv->sendq_len / 1024.0;
	if (per > 1.0)
		per = 1.0;

	while (list)
	{
		sess = list->data;
		if (sess->server == serv)
		{
			if (sess->gui->throttlemeter)
			{
				gtk_progress_bar_update ((GtkProgressBar *) sess->gui->throttlemeter, per);
			}
			if (sess->gui->throttleinfo)
			{
				snprintf (tbuf, sizeof(tbuf) - 1, "%d bytes", serv->sendq_len);
				gtk_label_set_text ((GtkLabel *) sess->gui->throttleinfo, tbuf);
			}
		}
		list = list->next;
	}
}

