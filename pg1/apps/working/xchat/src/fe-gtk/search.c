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

#include "fe-gtk.h"
#include "../common/xchat.h"
#include "../common/util.h"
#include "../common/xchatc.h"
#include "gtkutil.h"
#include "xtext.h"
#include "wins.h"


static void
search_search (session * sess, char *text)
{
	static void *last = NULL;

	if (!is_session (sess))
	{
		gtkutil_simpledialog (_("The window you opened this Search "
									 "for doesn't exist anymore."));
		return;
	}

	last = gtk_xtext_search (GTK_XTEXT (sess->gui->textgad), text, last);
}

static void
search_find_cb (GtkWidget * button, session * sess)
{
	GtkEntry *entry;
	char *text;

	entry = gtk_object_get_user_data (GTK_OBJECT (button));
	text = gtk_entry_get_text (entry);
	search_search (sess, text);
}

static void
search_clear_cb (GtkWidget * button, GtkEntry * entry)
{
	gtk_entry_set_text (entry, "");
	gtk_widget_grab_focus (GTK_WIDGET (entry));
}

static void
search_close_cb (GtkWidget * button, GtkWidget * win)
{
	gtk_widget_destroy (win);
}

static void
search_entry_cb (GtkWidget * entry, session * sess)
{
	search_search (sess, gtk_entry_get_text (GTK_ENTRY (entry)));
}

void
search_open (session * sess)
{
	GtkWidget *win, *hbox, *vbox, *entry, *wid;

	if (sess->type == SESS_SHELL)
		return;

	win = maingui_window ("search", _("X-Chat: Search"), TRUE, FALSE,
								 NULL, NULL, 0, 0, NULL);

	gtk_container_set_border_width (GTK_CONTAINER (win), 20);

	vbox = wins_get_vbox (win);

	hbox = gtk_hbox_new (0, 10);
	gtk_container_add (GTK_CONTAINER (vbox), hbox);
	gtk_widget_show (hbox);

	gtkutil_label_new (_("Find:"), hbox);

	entry = gtk_entry_new ();
	gtk_signal_connect (GTK_OBJECT (entry), "activate",
							  GTK_SIGNAL_FUNC (search_entry_cb), sess);
	gtk_container_add (GTK_CONTAINER (hbox), entry);
	gtk_widget_show (entry);
	gtk_widget_grab_focus (entry);

	hbox = gtk_hbox_new (0, 10);
	gtk_box_pack_end (GTK_BOX (vbox), hbox, 0, 0, 0);
	gtk_widget_show (hbox);

	wid = gtkutil_button (win, 0, _("Find"), search_find_cb, sess, hbox);
	gtk_object_set_user_data (GTK_OBJECT (wid), entry);
	gtkutil_button (win, 0, _("Clear"), search_clear_cb, entry, hbox);
	gtkutil_button (win, 0, _("Close"), search_close_cb, win, hbox);

	gtk_widget_show (win);
}
