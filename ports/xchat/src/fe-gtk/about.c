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
#include "fe-gtk.h"
#include "../common/xchat.h"
#include "../common/util.h"
#include "palette.h"
#include "gtkutil.h"
#ifdef USE_XLIB
#include <gdk/gdkx.h>
#endif
#include "about.h"


#ifdef USE_GNOME

void
menu_about (GtkWidget * wid, gpointer sess)
{
	char buf[512];
	const gchar *author[] = { "Peter Zelezny <zed@linux.com>", 0 };

	snprintf (buf, sizeof (buf),
				 _("An IRC Client for UNIX.\n\n"
				 "This binary was compiled on "__DATE__"\n"
				 "Using GTK %d.%d.%d X %d\n"
				 "Running on %s"),
				 gtk_major_version, gtk_minor_version, gtk_micro_version,
#ifdef USE_XLIB
				VendorRelease (GDK_DISPLAY ()), get_cpu_str());
#else
				666, get_cpu_str());
#endif

	gtk_widget_show (gnome_about_new ("X-Chat", VERSION,
							"(C) 1998-2001 Peter Zelezny", author, buf, 0));
}

#else

static GtkWidget *about = 0;

static int
about_close (void)
{
	about = 0;
	return 0;
}

void
menu_about (GtkWidget * wid, gpointer sess)
{
	GtkWidget *vbox, *label, *hbox;
	GtkStyle *about_style;
	GtkStyle *head_style;
	char buf[512];

	if (about)
	{
		gdk_window_show (about->window);
		return;
	}
	head_style = gtk_style_new ();
#ifndef WIN32
	gdk_font_unref (head_style->font);
	head_style->font = gdk_font_load ("-*-times-bold-i-*-*-*-240-*");
	if (!head_style->font)
		head_style->font = gdk_font_load ("fixed");
#endif
	head_style->fg[GTK_STATE_NORMAL] = colors[2];

	about_style = gtk_style_new ();
	gdk_font_unref (about_style->font);
	about_style->font = gdk_font_load ("fixed");

	about = gtk_window_new (GTK_WINDOW_DIALOG);
	gtk_window_position (GTK_WINDOW (about), GTK_WIN_POS_CENTER);
	gtk_window_set_title (GTK_WINDOW (about), _("About X-Chat"));
	gtk_container_set_border_width (GTK_CONTAINER (about), 6);
	gtk_signal_connect (GTK_OBJECT (about), "destroy",
							  GTK_SIGNAL_FUNC (about_close), 0);
	gtk_widget_realize (about);

	vbox = gtk_vbox_new (0, 2);
	gtk_container_add (GTK_CONTAINER (about), vbox);
	/*gtk_widget_show (vbox);*/

	label = gtk_entry_new ();
	gtk_entry_set_editable (GTK_ENTRY (label), FALSE);
	gtk_entry_set_text (GTK_ENTRY (label), "X-Chat " VERSION);
	gtk_widget_set_style (label, head_style);
	gtk_style_unref (head_style);
	gtk_container_add (GTK_CONTAINER (vbox), label);

	snprintf (buf, sizeof (buf),
				 _("(C) 1998-2001 Peter Zelezny <zed@linux.com>\n\n"
				 "An IRC Client for UNIX.\n\n"
				 "This binary was compiled on "__DATE__"\n"
				 "Using GTK %d.%d.%d X %d\n"
				 "Running on %s\n"),
				 gtk_major_version, gtk_minor_version, gtk_micro_version,
#ifdef USE_XLIB
				VendorRelease (GDK_DISPLAY ()), get_cpu_str());
#else
				666, get_cpu_str());
#endif

	label = gtk_label_new (buf);
	gtk_container_add (GTK_CONTAINER (vbox), label);
	gtk_widget_set_style (label, about_style);
	gtk_style_unref (about_style);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);

	wid = gtk_hseparator_new ();
	gtk_container_add (GTK_CONTAINER (vbox), wid);

	hbox = gtk_hbox_new (0, 2);
	gtk_container_add (GTK_CONTAINER (vbox), hbox);

	wid = gtk_button_new_with_label ("  Continue  ");
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);
	GTK_WIDGET_SET_FLAGS (GTK_WIDGET (wid), GTK_CAN_DEFAULT);
	gtk_box_pack_end (GTK_BOX (hbox), wid, 0, 0, 0);
	gtk_widget_grab_default (wid);
	gtk_signal_connect (GTK_OBJECT (wid), "clicked",
							  GTK_SIGNAL_FUNC (gtkutil_destroy), about);

	gtk_widget_show_all (about);
}
#endif
