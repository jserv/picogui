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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "fe-gtk.h"
#include "../common/xchat.h"
#include "../common/util.h"
#include "../common/xchatc.h"
#include "../common/cfgfiles.h"
#include "gtkutil.h"
#include "xtext.h"
#include "wins.h"


GdkColor colors[] = {
	{0, 0xcf3c, 0xcf3c, 0xcf3c}, /* 0  white */
	{0, 0x0000, 0x0000, 0x0000}, /* 1  black */
	{0, 0x0000, 0x0000, 0xcccc}, /* 2  blue */
	{0, 0x0000, 0xcccc, 0x0000}, /* 3  green */
	{0, 0xdddd, 0x0000, 0x0000}, /* 4  red */
	{0, 0xaaaa, 0x0000, 0x0000}, /* 5  light red */
	{0, 0xbbbb, 0x0000, 0xbbbb}, /* 6  purple */
	{0, 0xffff, 0xaaaa, 0x0000}, /* 7  orange */
	{0, 0xeeee, 0xdddd, 0x2222}, /* 8  yellow */
	{0, 0x3333, 0xdede, 0x5555}, /* 9  green */
	{0, 0x0000, 0xcccc, 0xcccc}, /* 10 aqua */
	{0, 0x3333, 0xdddd, 0xeeee}, /* 11 light aqua */
	{0, 0x0000, 0x0000, 0xffff}, /* 12 blue */
	{0, 0xeeee, 0x2222, 0xeeee}, /* 13 light purple */
	{0, 0x7777, 0x7777, 0x7777}, /* 14 grey */
	{0, 0x9999, 0x9999, 0x9999}, /* 15 light grey */
	{0, 0xbe00, 0xbe00, 0xbe00}, /* 16 marktext Back (white) */
	{0, 0x0000, 0x0000, 0x0000}, /* 17 marktext Fore (black) */
	{0, 0xcf3c, 0xcf3c, 0xcf3c}, /* 18 foreground (white) */
	{0, 0x0000, 0x0000, 0x0000}, /* 19 background (black) */
};

static GtkWidget *palwin = 0;
static GtkWidget *c_button;


void
palette_alloc (GtkWidget * widget)
{
	int i;
	static int done_alloc = FALSE;

	if (!done_alloc)		  /* don't do it again */
	{
		done_alloc = TRUE;
		for (i = 19; i >= 0; i--)
		{
			colors[i].pixel = (gulong) ((colors[i].red & 0xff00) * 256 +
												 (colors[i].green & 0xff00) +
												 (colors[i].blue & 0xff00) / 256);
			if (!gdk_color_alloc (gtk_widget_get_colormap (widget), &colors[i]))
				fprintf (stderr, "*** X-CHAT: cannot alloc colors\n");
		}
	}
}

void
palette_load (void)
{
	int i, l, fh, res;
	char prefname[256];
	struct stat st;
	char *cfg;
	unsigned long red, green, blue;

   snprintf (prefname, sizeof (prefname), "%s/palette.conf", get_xdir ());
   fh = open (prefname, O_RDONLY | OFLAGS);
	if (fh != -1)
	{
		fstat (fh, &st);
		cfg = malloc (st.st_size + 1);
		if (cfg)
		{
			cfg[0] = '\0';
			l = read (fh, cfg, st.st_size);
			if (l >= 0)
				cfg[l] = '\0';

			for (i = 0; i < 20; i++)
			{
				snprintf (prefname, sizeof prefname, "color_%d_red", i);
				red = cfg_get_int (cfg, prefname);

				snprintf (prefname, sizeof prefname, "color_%d_grn", i);
				green = cfg_get_int (cfg, prefname);

				snprintf (prefname, sizeof prefname, "color_%d_blu", i);
				blue = cfg_get_int_with_result (cfg, prefname, &res);

				if (res)
				{
					colors[i].red = red;
					colors[i].green = green;
					colors[i].blue = blue;
				}
			}
			free (cfg);
		}
		close (fh);
	}
}

void
palette_save (void)
{
	int i, fh;
	char prefname[256];

   snprintf (prefname, sizeof (prefname), "%s/palette.conf", get_xdir ());
   fh = open (prefname, O_TRUNC | O_WRONLY | O_CREAT | OFLAGS, 0600);
	if (fh != -1)
	{
		for (i = 0; i < 20; i++)
		{
			snprintf (prefname, sizeof prefname, "color_%d_red", i);
			cfg_put_int (fh, colors[i].red, prefname);

			snprintf (prefname, sizeof prefname, "color_%d_grn", i);
			cfg_put_int (fh, colors[i].green, prefname);

			snprintf (prefname, sizeof prefname, "color_%d_blu", i);
			cfg_put_int (fh, colors[i].blue, prefname);
		}
		close (fh);
	}
}

static void
palette_edit_destroy_callback ()
{
	GSList *list;
	session *sess;

	list = sess_list;
	while (list)
	{
		sess = list->data;
		if (sess->type != SESS_SHELL)
		{
			gtk_xtext_set_palette (GTK_XTEXT (sess->gui->textgad), colors);
			gtk_xtext_refresh (GTK_XTEXT (sess->gui->textgad), 0);
		}
		list = list->next;
	}

	palwin = 0;
}

static void
palette_okclicked (GtkWidget * ok, GtkWidget * color_dialog)
{
	GdkColor *color;
	gdouble col[3];
	GtkStyle *style;

	if (!GTK_IS_WIDGET (c_button))
	{
		gtk_widget_destroy (color_dialog);
		return;
	}

	color = gtk_object_get_user_data (GTK_OBJECT (c_button));

	gtk_color_selection_get_color ((GtkColorSelection *)
											 ((GtkColorSelectionDialog *) color_dialog)->
											 colorsel, col);

	/* is this line correct ?? */
	gdk_colormap_free_colors (gtk_widget_get_colormap (c_button), color, 1);

	color->red = (guint16) (col[0] * 65535.0);
	color->green = (guint16) (col[1] * 65535.0);
	color->blue = (guint16) (col[2] * 65535.0);

	color->pixel = (gulong) ((color->red & 0xff00) * 256 +
									 (color->green & 0xff00) +
									 (color->blue & 0xff00) / 256);
	gdk_color_alloc (gtk_widget_get_colormap (ok), color);

	style = gtk_style_new ();
	style->bg[0] = *color;
	gtk_widget_set_style (c_button, style);
	gtk_style_unref (style);

	gtk_widget_destroy (color_dialog);
}

static void
palette_editcolor (GtkWidget * button, GdkColor * color)
{
	gdouble col[3];
	GtkWidget *dialog;

	col[0] = color->red / 65535.0;
	col[1] = color->green / 65535.0;
	col[2] = color->blue / 65535.0;

	dialog = gtk_color_selection_dialog_new (_("Select color"));
	gtk_widget_hide (GTK_COLOR_SELECTION_DIALOG (dialog)->help_button);
	gtk_signal_connect (GTK_OBJECT
							  (((GtkColorSelectionDialog *) dialog)->ok_button),
							  "clicked", GTK_SIGNAL_FUNC (palette_okclicked),
							  dialog);
	gtk_signal_connect (GTK_OBJECT
							  (((GtkColorSelectionDialog *) dialog)->cancel_button),
							  "clicked", GTK_SIGNAL_FUNC (gtkutil_destroy), dialog);
	gtk_widget_set_sensitive (((GtkColorSelectionDialog *) dialog)->
									  help_button, FALSE);
	gtk_color_selection_set_color ((GtkColorSelection *)
											 ((GtkColorSelectionDialog *) dialog)->
											 colorsel, col);
	gtk_widget_show (dialog);
	c_button = button;
}

void
palette_edit ()
{
	char tbuf[64];
	GtkWidget *wid, *mainbox, *hbox, *vbox, *bbox, *bbbox;
	GtkStyle *style;
	int i;

	if (palwin)
	{
		wins_bring_tofront (palwin);
		return;
	}

	palwin = maingui_window ("palette", _("X-Chat: Palette"), TRUE, FALSE,
									 palette_edit_destroy_callback, NULL, 0, 0, NULL);
	mainbox = wins_get_vbox (palwin);

	hbox = gtk_hbox_new (0, 2);
	gtk_container_add (GTK_CONTAINER (mainbox), hbox);
	gtk_widget_show (hbox);

	vbox = gtk_vbox_new (0, 0);
	gtk_container_add (GTK_CONTAINER (hbox), vbox);
	gtk_widget_show (vbox);

	for (i = 0; i < 20; i++)
	{
		style = gtk_style_new ();
		style->bg[0] = colors[i];
		switch (i)
		{
		case 18:
			strcpy (tbuf, _("Foreground"));
			break;
		case 19:
			strcpy (tbuf, _("Background"));
			break;
		case 16:
			strcpy (tbuf, _("Mark Background"));
			break;
		case 17:
			strcpy (tbuf, _("Mark Foreground"));
			break;
		default:
			snprintf (tbuf, sizeof tbuf, _("Color %d"), i);
		}
		wid = gtk_button_new_with_label (tbuf);
		gtk_object_set_user_data (GTK_OBJECT (wid), &colors[i]);
		gtk_container_add (GTK_CONTAINER (vbox), wid);
		gtk_signal_connect (GTK_OBJECT (wid), "clicked",
								  GTK_SIGNAL_FUNC (palette_editcolor), &colors[i]);
		gtk_widget_set_usize (wid, 150, 0);
		if (colors[i].pixel == 0)
		{
			style->fg[0] = style->white;
			gtk_widget_set_style (GTK_BIN (wid)->child, style);
		}
		gtk_widget_set_style (wid, style);
		gtk_widget_show (wid);
		gtk_style_unref (style);

		if (i == 9)
		{
			vbox = gtk_vbox_new (0, 0);
			gtk_container_add (GTK_CONTAINER (hbox), vbox);
			gtk_widget_show (vbox);
		}
	}

	wid = gtk_hseparator_new ();
	gtk_container_add (GTK_CONTAINER (mainbox), wid);
	gtk_widget_show (wid);

	bbox = gtk_hbox_new (0, 1);
	gtk_box_pack_end (GTK_BOX (mainbox), bbox, 0, 0, 0);
	gtk_widget_show (bbox);

	bbbox = gtk_hbox_new (0, 1);
	gtk_box_pack_end (GTK_BOX (bbox), bbbox, 0, 0, 0);
	gtk_widget_set_usize (bbbox, 100, 0);
	gtk_widget_show (bbbox);

	gtkutil_button (palwin, GNOME_STOCK_BUTTON_OK, _("OK"),
						 gtkutil_destroy, palwin, bbbox);

	gtk_widget_show (palwin);
}
