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
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "fe-gtk.h"
#include "../common/xchat.h"
#include "../common/xchatc.h"
#include "../common/cfgfiles.h"
#include "../common/plugin.h"
#include "../common/outbound.h"
#include "../common/text.h"
#include "gtkutil.h"
#include "xtext.h"
#include "wins.h"
#include "palette.h"
#include "textgui.h"

struct text_event
{
	char *name;
	char **help;
	char *def;
	int num_args;
	char *sound;
};

extern struct text_event te[];
extern char **pntevts_text;
extern char **pntevts;

static GtkWidget *pevent_dialog = NULL, *pevent_dialog_twid,
	*pevent_dialog_entry, *pevent_dialog_sound_entry,
	*pevent_dialog_list, *pevent_dialog_hlist;

int
get_stamp_str (time_t tim, char *dest, int size)
{
	return strftime (dest, size, prefs.stamp_format, localtime (&tim));
}

static void
PrintTextLine (GtkWidget * textwidget, unsigned char *text, int len, int indent)
{
	char *tab, *new_text;
	int leftlen;

	if (len == 0)
		len = 1;

	if (!indent)
	{
		if (prefs.timestamp)
		{
			char buf[64];
			int stamp_size;

			stamp_size = get_stamp_str (time (0), buf, sizeof (buf));
			new_text = malloc (len + stamp_size + 1);
			memcpy (new_text, buf, stamp_size);
			memcpy (new_text + stamp_size, text, len);
			gtk_xtext_append (GTK_XTEXT (textwidget), new_text, len + stamp_size);
			free (new_text);
		} else
			gtk_xtext_append (GTK_XTEXT (textwidget), text, len);
		return;
	}

	tab = strchr (text, '\t');
	if (tab && (unsigned long) tab < (unsigned long) (text + len))
	{
		leftlen = (unsigned long) tab - (unsigned long) text;
		gtk_xtext_append_indent (GTK_XTEXT (textwidget),
										 text, leftlen, tab + 1, len - (leftlen + 1));
	} else
		gtk_xtext_append_indent (GTK_XTEXT (textwidget), 0, 0, text, len);
}

void
PrintTextRaw (GtkWidget * textwidget, unsigned char *text, int indent)
{
	char *last_text = text;
	int len = 0;

	/* split the text into separate lines */
	while (1)
	{
		switch (*text)
		{
		case 0:
			PrintTextLine (textwidget, last_text, len, indent);
			return;
		case '\n':
			PrintTextLine (textwidget, last_text, len, indent);
			text++;
			if (*text == 0)
				return;
			last_text = text;
			len = 0;
			break;
		case ATTR_BEEP:
			*text = ' ';
			if (!prefs.filterbeep)
				gdk_beep ();
		default:
			text++;
			len++;
		}
	}
}

static void
pevent_dialog_close (gpointer * arg)
{
	pevent_dialog = NULL;
	pevent_save (NULL);
}

static void
pevent_dialog_update_sound (GtkWidget * wid, GtkWidget * clist)
{
	int row, sig;

	row = gtkutil_clist_selection (pevent_dialog_list);
	if (row == -1)
		return;

	gtk_clist_set_text (GTK_CLIST (clist), row, 2,
							  gtk_entry_get_text (GTK_ENTRY (wid)));

	sig = (int) gtk_clist_get_row_data (GTK_CLIST (clist), row);

	if (te[sig].sound)
		free (te[sig].sound);

	te[sig].sound = strdup (gtk_entry_get_text (GTK_ENTRY (wid)));
}

static void
pevent_dialog_update (GtkWidget * wid, GtkWidget * twid)
{
	int row, len, m;
	char *text, *out;
	int sig;

	row = gtkutil_clist_selection (pevent_dialog_list);
	if (row == -1)
		return;

	sig = (int) gtk_clist_get_row_data (GTK_CLIST (pevent_dialog_list), row);

	text = gtk_entry_get_text (GTK_ENTRY (wid));
	len = strlen (text);

	if (pevt_build_string (text, &out, &m) != 0)
	{
		gtkutil_simpledialog (_("There was an error parsing the string"));
		return;
	}
	if (m > te[sig].num_args)
	{
		free (out);
		out = malloc (4096);
		snprintf (out, 4096,
					 _("This signal is only passed %d args, $%d is invalid"),
					 te[sig].num_args, m);
		gtkutil_simpledialog (out);
		free (out);
		return;
	}
	gtk_clist_set_text (GTK_CLIST (pevent_dialog_list), row, 1, text);

	if (pntevts_text[sig])
		free (pntevts_text[sig]);
	if (pntevts[sig])
		free (pntevts[sig]);

	pntevts_text[sig] = malloc (len + 1);
	memcpy (pntevts_text[sig], text, len + 1);
	pntevts[sig] = out;

	out = malloc (len + 2);
	memcpy (out, text, len + 1);
	out[len] = '\n';
	out[len + 1] = 0;
	check_special_chars (out, TRUE);

	PrintTextRaw (twid, out, 0);
	free (out);
}

static void
pevent_dialog_unselect (GtkWidget * clist, gint row, gint column,
								GdkEventButton * even, gpointer none)
{
	gtk_entry_set_text (GTK_ENTRY (pevent_dialog_sound_entry), "");
	gtk_entry_set_text (GTK_ENTRY (pevent_dialog_entry), "");
	gtk_clist_clear (GTK_CLIST (pevent_dialog_hlist));
}

static void
pevent_dialog_hfill (GtkWidget * list, int e)
{
	gchar *nnew[2];
	int i = 0;
	char *text, buf[64];

	if (!text_event (e))
		return;

	text = _(te[e].help[i]);

	gtk_clist_clear (GTK_CLIST (list));
	while (text)
	{
		snprintf (buf, sizeof (buf), "%d", i + 1);
		if (text[0] == '\001')
			text++;
		nnew[0] = buf;
		nnew[1] = text;
		gtk_clist_append (GTK_CLIST (list), nnew);

		i++;
		text = _(te[e].help[i]);
	}
}

static void
pevent_dialog_select (GtkWidget * clist, gint row, gint column,
							 GdkEventButton * even, gpointer none)
{
	char *cmd, *snd;
	int sig;

	row = gtkutil_clist_selection (pevent_dialog_list);
	if (row != -1)
	{
		gtk_clist_get_text (GTK_CLIST (clist), row, 1, &cmd);
		gtk_entry_set_text (GTK_ENTRY (pevent_dialog_entry), cmd);
		gtk_clist_get_text (GTK_CLIST (clist), row, 2, &snd);
		gtk_entry_set_text (GTK_ENTRY (pevent_dialog_sound_entry), snd);
		sig = (int) gtk_clist_get_row_data (GTK_CLIST (clist), row);
		pevent_dialog_hfill (pevent_dialog_hlist, sig);
	} else
	{
		pevent_dialog_unselect (0, 0, 0, 0, 0);
	}
}

static void
pevent_dialog_fill (GtkWidget * list)
{
	int i, row;
	gchar *nnew[3];

	gtk_clist_clear (GTK_CLIST (list));

	for (i = 0; i < NUM_XP; i++)
	{
		if (!text_event (i))
			continue;
		nnew[0] = te[i].name;
		nnew[1] = pntevts_text[i];
		if (te[i].sound)
			nnew[2] = te[i].sound;
		else
			nnew[2] = "";
		row = gtk_clist_append (GTK_CLIST (list), nnew);
		gtk_clist_set_row_data (GTK_CLIST (list), row, (void *) i);
	}
}

static void
pevent_save_req_cb (void *arg1, void *arg2, char *file)
{
	if (file)
	{
		pevent_save (file);
		free (file);
	}
}

static void
pevent_save_cb (GtkWidget * wid, void *data)
{
	if (data)
	{
		gtkutil_file_req (_("Print Texts File"), pevent_save_req_cb, NULL, NULL,
								TRUE);
		return;
	}
	pevent_save (NULL);
}

static void
pevent_load_req_cb (void *arg1, void *arg2, char *file)
{
	if (file)
	{
		pevent_load (file);
		pevent_make_pntevts ();
		pevent_dialog_fill (pevent_dialog_list);
		pevent_dialog_select (pevent_dialog_list, -1, -1, NULL, NULL);
		free (file);
	}
}

static void
pevent_load_cb (GtkWidget * wid, void *data)
{
	gtkutil_file_req (_("Print Texts File"), pevent_load_req_cb, NULL, NULL,
							FALSE);
}

static void
pevent_ok_cb (GtkWidget * wid, void *data)
{
	gtk_widget_destroy (pevent_dialog);
}

static void
pevent_test_cb (GtkWidget * wid, GtkWidget * twid)
{
	int len, n;
	char *out, *text;

	for (n = 0; n < NUM_XP; n++)
	{
		if (!text_event (n))
			continue;
		text = _(pntevts_text[n]);
		len = strlen (text);

		out = malloc (len + 2);
		memcpy (out, text, len + 1);
		out[len] = '\n';
		out[len + 1] = 0;
		check_special_chars (out, TRUE);

		PrintTextRaw (twid, out, 0);
		free (out);
	}
}

/* from settings.c - but its not there anymore */
static void
gui_entry (char *label, int max, GtkWidget * box, GtkWidget ** entry)
{
	GtkWidget *wid, *hbox;

	hbox = gtk_hbox_new (0, 0);
	gtk_widget_show (hbox);

	gtkutil_label_new (label, hbox);

	*entry = wid = gtk_entry_new_with_max_length (max);
	gtk_container_add (GTK_CONTAINER (hbox), wid);
	gtk_widget_show (wid);

	gtk_box_pack_start (GTK_BOX (box), hbox, 0, 0, 0);
}

void
pevent_dialog_show ()
{
	GtkWidget *vbox, *vbox2, *hbox, *tbox, *wid, *bh, *th;
	gchar *titles[] = { _("Event"), _("Text"), _("Sound") };
	gchar *help_titles[] = { _("$ Number"), _("Description") };

	if (pevent_dialog)
	{
		wins_bring_tofront (pevent_dialog);
		return;
	}

	pevent_dialog = maingui_window ("edit events", _("Edit Events"),
											  TRUE, FALSE, pevent_dialog_close, NULL,
											  600, 455, NULL);

	vbox2 = gtk_vbox_new (0, 2);
	vbox = wins_get_vbox (pevent_dialog);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);

	wid = gtk_vpaned_new ();
	th = gtk_vbox_new (0, 2);
	bh = gtk_vbox_new (0, 2);
	gtk_widget_show (th);
	gtk_widget_show (bh);
	gtk_paned_pack1 (GTK_PANED (wid), th, 1, 1);
	gtk_paned_pack2 (GTK_PANED (wid), bh, 0, 1);
	gtk_box_pack_start (GTK_BOX (vbox), wid, 1, 1, 0);
	gtk_widget_show (wid);
	pevent_dialog_list = gtkutil_clist_new (3, titles, th, GTK_POLICY_ALWAYS,
														 pevent_dialog_select, 0,
														 pevent_dialog_unselect, 0,
														 GTK_SELECTION_BROWSE);
	gtk_clist_set_column_width (GTK_CLIST (pevent_dialog_list), 0, 80);
	gtk_clist_set_column_width (GTK_CLIST (pevent_dialog_list), 1, 380);

	pevent_dialog_twid = gtk_xtext_new (0, 0);

	gtk_xtext_set_palette (GTK_XTEXT (pevent_dialog_twid), colors);
	gtk_xtext_set_font (GTK_XTEXT (pevent_dialog_twid), font_normal, 0);
	gtk_xtext_set_background (GTK_XTEXT (pevent_dialog_twid),
									  channelwin_pix, prefs.transparent, prefs.tint);

	pevent_dialog_entry = gtk_entry_new_with_max_length (255);
	gtk_widget_set_usize (pevent_dialog_entry, 96, 0);

	gtk_signal_connect (GTK_OBJECT (pevent_dialog_entry), "activate",
							  GTK_SIGNAL_FUNC (pevent_dialog_update),
							  pevent_dialog_twid);

	gtk_box_pack_start (GTK_BOX (bh), pevent_dialog_entry, 0, 0, 0);
	gtk_widget_show (pevent_dialog_entry);

	gui_entry (_("Sound file: "), 64, bh, &pevent_dialog_sound_entry);
	gtk_signal_connect (GTK_OBJECT (pevent_dialog_sound_entry), "activate",
							  GTK_SIGNAL_FUNC (pevent_dialog_update_sound),
							  pevent_dialog_list);

	tbox = gtk_hbox_new (0, 0);
	gtk_container_add (GTK_CONTAINER (bh), tbox);
	gtk_widget_show (tbox);

	gtk_widget_set_usize (pevent_dialog_twid, 150, 20);
	gtk_container_add (GTK_CONTAINER (tbox), pevent_dialog_twid);

	gtk_widget_show (pevent_dialog_twid);

	wid = gtk_vscrollbar_new (GTK_XTEXT (pevent_dialog_twid)->adj);
	gtk_box_pack_start (GTK_BOX (tbox), wid, FALSE, FALSE, 0);
	show_and_unfocus (wid);

	pevent_dialog_hlist = gtkutil_clist_new (2, help_titles, bh,
														  GTK_POLICY_ALWAYS,
														  NULL, 0, NULL, 0,
														  GTK_SELECTION_BROWSE);
	gtk_clist_set_column_width (GTK_CLIST (pevent_dialog_list), 0, 120);
	gtk_widget_show (pevent_dialog_hlist);

	pevent_dialog_fill (pevent_dialog_list);

	hbox = gtk_hbox_new (0, 2);
	gtk_box_pack_end (GTK_BOX (vbox), hbox, 0, 0, 0);
	wid = gtk_button_new_with_label (_("Save"));
	gtk_box_pack_end (GTK_BOX (hbox), wid, 0, 0, 0);
	gtk_signal_connect (GTK_OBJECT (wid), "clicked",
							  GTK_SIGNAL_FUNC (pevent_save_cb), NULL);
	gtk_widget_show (wid);
	wid = gtk_button_new_with_label (_("Save As"));
	gtk_box_pack_end (GTK_BOX (hbox), wid, 0, 0, 0);
	gtk_signal_connect (GTK_OBJECT (wid), "clicked",
							  GTK_SIGNAL_FUNC (pevent_save_cb), (void *) 1);
	gtk_widget_show (wid);
	wid = gtk_button_new_with_label (_("Load From"));
	gtk_box_pack_end (GTK_BOX (hbox), wid, 0, 0, 0);
	gtk_signal_connect (GTK_OBJECT (wid), "clicked",
							  GTK_SIGNAL_FUNC (pevent_load_cb), (void *) 0);
	gtk_widget_show (wid);
	wid = gtk_button_new_with_label (_("Test All"));
	gtk_box_pack_end (GTK_BOX (hbox), wid, 0, 0, 0);
	gtk_signal_connect (GTK_OBJECT (wid), "clicked",
							  GTK_SIGNAL_FUNC (pevent_test_cb), pevent_dialog_twid);
	gtk_widget_show (wid);
#ifdef USE_GNOME
	wid = gnome_stock_button (GNOME_STOCK_BUTTON_OK);
#else
	wid = gtk_button_new_with_label (_("Ok"));
#endif
	gtk_box_pack_start (GTK_BOX (hbox), wid, 0, 0, 0);
	gtk_signal_connect (GTK_OBJECT (wid), "clicked",
							  GTK_SIGNAL_FUNC (pevent_ok_cb), NULL);
	gtk_widget_show (wid);

	gtk_widget_show (hbox);

	gtk_widget_show (pevent_dialog);
}
