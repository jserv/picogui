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
#include <stdlib.h>

#include "fe-gtk.h"
#include "../common/xchat.h"
#include "../common/util.h"
#include "../common/userlist.h"
#include "../common/modes.h"
#include "../common/notify.h"
#include "../common/xchatc.h"
#include "../common/dcc.h"
#include "gtkutil.h"
#include "palette.h"
#include "maingui.h"
#include "pixmaps.h"
#include "userlistgui.h"


static GdkPixmap *
get_prefix_pixmap (char prefix, GdkBitmap **mask)
{
	switch (prefix)
	{
	case '*':
		*mask = mask_admin;
		return pix_admin;
	case '@':
		*mask = mask_op;
		return pix_op;
	case '%':
		*mask = mask_hop;
		return pix_hop;
	case '+':
		*mask = mask_voice;
		return pix_voice;
	}
	return 0;
}

/* change the little icon to the left of your nickname */

static void
mode_myself (struct session_gui *gui, GdkPixmap *pix, GdkBitmap *mask, char prefix)
{
	char buf[2];

	if (gui->op_xpm)
	{
		gtk_widget_destroy (gui->op_xpm);
		gui->op_xpm = 0;
	}

	if (!prefs.userlist_icons && prefix != ' ')
	{
		buf[0] = prefix;
		buf[1] = 0;
		gui->op_xpm = gtk_label_new (buf);
		gtk_box_pack_start (GTK_BOX (gui->op_box), gui->op_xpm, 0, 0, 0);
		gtk_widget_show (gui->op_xpm);
		return;
	}

	if (pix)
	{
		gui->op_xpm = gtk_pixmap_new (pix, mask);
		gtk_box_pack_start (GTK_BOX (gui->op_box), gui->op_xpm, 0, 0, 0);
		gtk_widget_show (gui->op_xpm);
	}
}

void
fe_userlist_numbers (struct session *sess)
{
	char tbuf[256];

	sprintf (tbuf, "%d", sess->ops);
	gtk_label_set_text (GTK_LABEL (sess->gui->namelistinfo_o), tbuf);
	sprintf (tbuf, "%d", sess->voices);
	gtk_label_set_text (GTK_LABEL (sess->gui->namelistinfo_v), tbuf);
	sprintf (tbuf, "%d", sess->total);
	gtk_label_set_text (GTK_LABEL (sess->gui->namelistinfo_t), tbuf);

	gtk_widget_show (sess->gui->namelistinfo);
}

void
fe_userlist_remove (struct session *sess, struct User *user)
{
	gint row =
		gtk_clist_find_row_from_data (GTK_CLIST (sess->gui->namelistgad),
												(gpointer) user);
	GtkAdjustment *adj;
	gfloat val, end;

	adj = gtk_clist_get_vadjustment (GTK_CLIST (sess->gui->namelistgad));
	val = adj->value;

	gtk_clist_remove (GTK_CLIST (sess->gui->namelistgad), row);

	end = adj->upper - adj->lower - adj->page_size;
	if (val > end)
		val = end;
	gtk_adjustment_set_value (adj, val);
}

int
fe_userlist_insert (struct session *sess, struct User *newuser, int row)
{
	char *name[2];
	GtkAdjustment *adj;
	GdkPixmap *pix = NULL, *mask;
	gfloat val;

	name[0] = newuser->nick;
	name[1] = newuser->hostname;

	if (!prefs.userlist_icons && newuser->prefix != ' ')
	{
		name[0] = malloc (strlen (newuser->nick) + 2);
		name[0][0] = newuser->prefix;
		strcpy (name[0] + 1, newuser->nick);
	}

	adj = gtk_clist_get_vadjustment (GTK_CLIST (sess->gui->namelistgad));
	val = adj->value;

	switch (row)
	{
	case -1:
		row = gtk_clist_append (GTK_CLIST (sess->gui->namelistgad), name);
		break;
	case 0:
		row = gtk_clist_prepend (GTK_CLIST (sess->gui->namelistgad), name);
		break;
	default:
		gtk_clist_insert (GTK_CLIST (sess->gui->namelistgad), row, name);
	}

	gtk_clist_set_row_data (GTK_CLIST (sess->gui->namelistgad), row,
									(gpointer) newuser);

	if (prefs.userlist_icons)
	{
		pix = get_prefix_pixmap (newuser->prefix, &mask);
		if (pix)
			gtk_clist_set_pixtext ((GtkCList *) sess->gui->namelistgad, row, 0,
										  newuser->nick, 3, pix, mask);
	}

	/* is it me? */
	if (!strcmp (newuser->nick, sess->server->nick) && sess->gui->op_box)
		mode_myself (sess->gui, pix, mask, newuser->prefix);

	if (name[0] != newuser->nick)
		free (name[0]);

	if (prefs.hilitenotify && notify_isnotify (sess, newuser->nick))
	{
		gtk_clist_set_foreground ((GtkCList *) sess->gui->namelistgad, row,
										  &colors[prefs.nu_color]);
	}

	gtk_adjustment_set_value (adj, val);

	return row;
}

void
fe_userlist_move (struct session *sess, struct User *user, int new_row)
{
	gint old_row;
	int sel = FALSE;

	old_row =
		gtk_clist_find_row_from_data (GTK_CLIST (sess->gui->namelistgad),
												(gpointer) user);

	if (old_row == gtkutil_clist_selection (sess->gui->namelistgad))
		sel = TRUE;

	gtk_clist_remove (GTK_CLIST (sess->gui->namelistgad), old_row);
	new_row = fe_userlist_insert (sess, user, new_row);

	if (sel)
		gtk_clist_select_row ((GtkCList *) sess->gui->namelistgad, new_row, 0);
}

void
fe_userlist_clear (struct session *sess)
{
	gtk_clist_clear (GTK_CLIST (sess->gui->namelistgad));
}

#ifdef USE_GNOME

void
userlist_dnd_drop (GtkWidget * widget, GdkDragContext * context,
						 gint x, gint y,
						 GtkSelectionData * selection_data,
						 guint info, guint32 time, struct session *sess)
{
	struct User *user;
	char tbuf[400];
	char *file;
	int row, col;
	GList *list;

	if (gtk_clist_get_selection_info (GTK_CLIST (widget), x, y, &row, &col) < 0)
		return;

	user = gtk_clist_get_row_data (GTK_CLIST (widget), row);
	if (!user)
		return;
	list = gnome_uri_list_extract_filenames (selection_data->data);
	while (list)
	{
		file = (char *) (list->data);
		dcc_send (sess, tbuf, user->nick, file);
		list = list->next;
	}
	gnome_uri_list_free_strings (list);
}

int
userlist_dnd_motion (GtkWidget * widget, GdkDragContext * context, gint x,
							gint y, guint ttime)
{
	int row, col;

	if (gtk_clist_get_selection_info (GTK_CLIST (widget), x, y, &row, &col) !=
		 -1)
	{
		gtk_clist_select_row (GTK_CLIST (widget), row, col);
	}
	return 1;
}

int
userlist_dnd_leave (GtkWidget * widget, GdkDragContext * context, guint ttime)
{
	gtk_clist_unselect_all (GTK_CLIST (widget));
	return 1;
}

#endif
