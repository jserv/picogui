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
#include <signal.h>
#include <string.h>
#include "../../config.h"
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#include <stdlib.h>

#include "fe-gtk.h"
#include "../common/xchat.h"
#include "../common/cfgfiles.h"
#include "../common/util.h"
#include "../common/xchatc.h"
#include "gtkutil.h"
#include "menu.h"
#include "wins.h"
#include "urlgrab.h"


static GSList *url_list = 0;
static GtkWidget *urlgrabberwindow = 0;
static GtkWidget *urlgrabberlist;
static void url_addurlgui (char *urltext);


static void
url_closegui (gpointer userdata)
{
	urlgrabberwindow = 0;
}

static void
url_button_clear (void)
{
	while (url_list)
	{
		free (url_list->data);
		url_list = g_slist_remove (url_list, url_list->data);
	}
	gtk_clist_clear (GTK_CLIST (urlgrabberlist));
}

void
url_autosave (void)
{
	FILE *fd;
	GSList *list;
	char *buf;

	buf = malloc (1000);
	snprintf (buf, 1000, "%s/url.save", get_xdir ());

	fd = fopen (buf, "a");
	free (buf);
	if (fd == NULL)
		return;

	list = url_list;

	while (list)
	{
		fprintf (fd, "%s\n", (char *) list->data);
		list = list->next;
	}

	fclose (fd);
}

static void
url_save_callback (void *arg1, void *arg2, char *file)
{
	FILE *fd;
	GSList *list;

	if (file)
	{
		fd = fopen (file, "w");
		if (fd == NULL)
			return;
		list = url_list;
		while (list)
		{
			fprintf (fd, "%s\n", (char *) list->data);
			list = list->next;
		}
		fclose (fd);
		free (file);
	}
}

static void
url_button_save (void)
{
	gtkutil_file_req (_("Select a file to save to"),
							url_save_callback, 0, 0, TRUE);
}

static void
url_clicklist (GtkWidget * widget, GdkEventButton * event)
{
	int row, col;
	char *text;

	if (event->button == 3)
	{
		if (gtk_clist_get_selection_info
			 (GTK_CLIST (widget), event->x, event->y, &row, &col) < 0)
			return;
		gtk_clist_unselect_all (GTK_CLIST (widget));
		gtk_clist_select_row (GTK_CLIST (widget), row, 0);
		if (gtk_clist_get_text (GTK_CLIST (widget), row, 0, &text))
		{
			if (text && text[0])
			{
				menu_urlmenu (event, text);
			}
		}
	}
}

void
url_opengui ()
{
	GtkWidget *vbox, *hbox;
	GSList *list;

	if (urlgrabberwindow)
	{
		wins_bring_tofront (urlgrabberwindow);
		return;
	}

	urlgrabberwindow =
		maingui_window ("urlgrabber", _("X-Chat: URL Grabber"), FALSE, TRUE,
							 url_closegui, NULL, 350, 100, &urlgrabberwindow);
	vbox = wins_get_vbox (urlgrabberwindow);

	urlgrabberlist = gtkutil_clist_new (1, 0, vbox, GTK_POLICY_AUTOMATIC,
													0, 0, 0, 0, GTK_SELECTION_BROWSE);
	gtk_signal_connect (GTK_OBJECT (urlgrabberlist), "button_press_event",
							  GTK_SIGNAL_FUNC (url_clicklist), 0);
	gtk_widget_set_usize (urlgrabberlist, 350, 0);
	gtk_clist_set_column_width (GTK_CLIST (urlgrabberlist), 0, 100);

	hbox = gtk_hbox_new (FALSE, 1);
	gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 2);
	gtk_widget_show (hbox);

	gtkutil_button (urlgrabberwindow, GNOME_STOCK_PIXMAP_CLEAR,
						 _("Clear"), url_button_clear, 0, hbox);
	gtkutil_button (urlgrabberwindow, GNOME_STOCK_PIXMAP_SAVE,
						 _("Save"), url_button_save, 0, hbox);

	gtk_widget_show (urlgrabberwindow);

	list = url_list;
	while (list)
	{
		url_addurlgui ((char *) list->data);
		list = list->next;
	}
}

static void
url_addurlgui (char *urltext)
{
	if (urlgrabberwindow)
		gtk_clist_prepend ((GtkCList *) urlgrabberlist, &urltext);
}

static int
url_findurl (char *urltext)
{
	GSList *list = url_list;
	while (list)
	{
		if (!strcasecmp (urltext, (char *) list->data))
			return 1;
		list = list->next;
	}
	return 0;
}

static void
url_addurl (char *urltext)
{
	char *data = strdup (urltext);
	if (!data)
		return;

	if (data[strlen (data) - 1] == '.')	/* chop trailing dot */
		data[strlen (data) - 1] = 0;

	if (url_findurl (data))
	{
		free (data);
		return;
	}

	url_list = g_slist_prepend (url_list, data);
	url_addurlgui (data);
}

void
fe_checkurl (char *buf)
{
	char t, *po, *urltext = nocasestrstr (buf, "http:");
	if (!urltext)
		urltext = nocasestrstr (buf, "www.");
	if (!urltext)
		urltext = nocasestrstr (buf, "ftp.");
	if (!urltext)
		urltext = nocasestrstr (buf, "ftp:");
	if (!urltext)
		urltext = nocasestrstr (buf, "irc://");
	if (!urltext)
		urltext = nocasestrstr (buf, "irc.");
	if (urltext)
	{
		po = strchr (urltext, ' ');
		if (po)
		{
			t = *po;
			*po = 0;
			url_addurl (urltext);
			*po = t;
		} else
			url_addurl (urltext);
	}
}
