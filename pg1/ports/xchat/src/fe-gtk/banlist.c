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
#include <fcntl.h>
#include <time.h>

#include "fe-gtk.h"
#include "../common/xchat.h"
#include "../common/outbound.h"
#include "../common/xchatc.h"
#include "gtkutil.h"
#include "wins.h"
#include "banlist.h"


void
fe_add_ban_list (struct session *sess, char *mask, char *who, char *when)
{
	gchar *next_row[3];

	next_row[0] = mask;
	next_row[1] = who;
	next_row[2] = when;

	gtk_clist_append (GTK_CLIST (sess->gui->banlist_clistBan), next_row);
}

void
fe_ban_list_end (struct session *sess)
{
	gtk_widget_set_sensitive (sess->gui->banlist_butRefresh, TRUE);
}

/**
 *  * Performs the actual refresh operations.
 *  */
static void
banlist_do_refresh (struct session *sess)
{
	char tbuf[256];
	if (sess->server->connected)
	{
		gtk_clist_clear (GTK_CLIST (sess->gui->banlist_clistBan));
		gtk_widget_set_sensitive (sess->gui->banlist_butRefresh, FALSE);

		snprintf (tbuf, sizeof tbuf, "X-Chat: Ban List (%s, %s)",
						sess->channel, sess->server->servername);
		wins_set_title (sess->gui->banlist_window, tbuf);

		snprintf (tbuf, sizeof tbuf, "MODE %s +b\r\n", sess->channel);
		tcp_send (sess->server, tbuf);
	} else
		gtkutil_simpledialog ("Not connected.");
}

static void
banlist_refresh (GtkWidget * wid, struct session *sess)
{
	/* JG NOTE: Didn't see actual use of wid here, so just forwarding
	   *          * this to chanlist_do_refresh because I use it without any widget
	   *          * param in chanlist_build_gui_list when the user presses enter
	   *          * or apply for the first time if the list has not yet been
	   *          * received.
	   *          */
	banlist_do_refresh (sess);
}

static void
banlist_unban (GtkWidget * wid, struct session *sess)
{
	char tbuf[2048], *mask;
	unsigned int i;
	unsigned int sel_length =
		g_list_length (GTK_CLIST (sess->gui->banlist_clistBan)->selection);
	char **masks;
	int row;

	if (sel_length < 1)
		return;

	/* create an array of all the masks */
	masks = malloc (sel_length * sizeof (char *));
	for (i = 0; i < sel_length; i++)
	{
		row = (int)g_list_nth_data (GTK_CLIST (sess->gui->banlist_clistBan)->
											 selection, i);
		gtk_clist_get_text (GTK_CLIST (sess->gui->banlist_clistBan), row, 0,
								  &mask);
		masks[i] = mask;
	}

	send_channel_modes (sess, tbuf, masks, 0, i, '-', 'b');
	free (masks);
	banlist_do_refresh (sess);
}

static void
banlist_wipe (GtkWidget * wid, struct session *sess)
{
	gtk_clist_select_all (GTK_CLIST (sess->gui->banlist_clistBan));
	banlist_unban (wid, sess);
}

static void
banlist_crop (GtkWidget * wid, struct session *sess)
{
	GList *oldsel;
	unsigned int sel_length;
	unsigned int i;
	int row;

	oldsel = g_list_copy (GTK_CLIST (sess->gui->banlist_clistBan)->selection);
	gtk_clist_select_all (GTK_CLIST (sess->gui->banlist_clistBan));
	sel_length = g_list_length (oldsel);
	if (sel_length)
	{
		for (i = 0; i < sel_length; i++)
		{
			row = (int) g_list_nth_data (oldsel, i);
			gtk_clist_unselect_row (GTK_CLIST (sess->gui->banlist_clistBan), row,
											0);
		}
		banlist_unban (wid, sess);
	}
	g_list_free (oldsel);
}

static void
banlist_closegui (struct session *sess)
{
	if (is_session (sess))
		sess->gui->banlist_window = 0;
}

void
banlist_opengui (struct session *sess)
{
	GtkWidget *vbox1;
	GtkWidget *hbox1;
	GtkWidget *vbox2;
	GtkWidget *butUnban;
	GtkWidget *butCrop;
	GtkWidget *butWipe;
/*   GtkWidget *hseparator1;
   GtkWidget *hbox2;
   GtkWidget *entryBanExp;
   GtkWidget *butBanEntry;
   GtkWidget *butUnbanEntry;
*/
	char tbuf[256];
	gchar *titles[] = { _("Mask"), _("From"), _("Date") };

	if (sess->gui->banlist_window)
	{
		wins_bring_tofront (sess->gui->banlist_window);
		return;
	}

	snprintf (tbuf, sizeof tbuf, _("X-Chat: Ban List (%s)"),
					sess->server->servername);
	sess->gui->banlist_window =
			maingui_window ("banlist", tbuf,
								 FALSE, TRUE, banlist_closegui, sess, 550, 200,
								 &sess->gui->banlist_window);
	vbox1 = wins_get_vbox (sess->gui->banlist_window);

	hbox1 = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox1);
	gtk_box_pack_start (GTK_BOX (vbox1), hbox1, TRUE, TRUE, 0);

	sess->gui->banlist_clistBan =
		gtkutil_clist_new (3, titles, hbox1, GTK_POLICY_AUTOMATIC, NULL,
								NULL, NULL, NULL, GTK_SELECTION_EXTENDED);
	gtk_widget_show (sess->gui->banlist_clistBan);
	gtk_clist_set_column_width (GTK_CLIST (sess->gui->banlist_clistBan), 0,
										 196);
	gtk_clist_set_column_width (GTK_CLIST (sess->gui->banlist_clistBan), 1,
										 117);
	gtk_clist_set_column_width (GTK_CLIST (sess->gui->banlist_clistBan), 2,
										 80);

	vbox2 = gtk_vbox_new (TRUE, 0);
	gtk_widget_show (vbox2);
	gtk_box_pack_start (GTK_BOX (hbox1), vbox2, FALSE, TRUE, 0);

	butUnban = gtk_button_new_with_label (_("Unban"));
	gtk_widget_show (butUnban);
	gtk_box_pack_start (GTK_BOX (vbox2), butUnban, FALSE, FALSE, 0);
	gtk_widget_set_usize (butUnban, 80, -2);
	gtk_container_set_border_width (GTK_CONTAINER (butUnban), 5);

	gtk_signal_connect (GTK_OBJECT (butUnban), "clicked",
							  GTK_SIGNAL_FUNC (banlist_unban), (gpointer) sess);

	butCrop = gtk_button_new_with_label (_("Crop"));
	gtk_widget_show (butCrop);
	gtk_box_pack_start (GTK_BOX (vbox2), butCrop, FALSE, FALSE, 0);
	gtk_widget_set_usize (butCrop, 80, -2);
	gtk_container_set_border_width (GTK_CONTAINER (butCrop), 5);

	gtk_signal_connect (GTK_OBJECT (butCrop), "clicked",
							  GTK_SIGNAL_FUNC (banlist_crop), (gpointer) sess);

	butWipe = gtk_button_new_with_label (_("Wipe"));
	gtk_widget_show (butWipe);
	gtk_box_pack_start (GTK_BOX (vbox2), butWipe, FALSE, FALSE, 0);
	gtk_widget_set_usize (butWipe, 80, -2);
	gtk_container_set_border_width (GTK_CONTAINER (butWipe), 5);

	gtk_signal_connect (GTK_OBJECT (butWipe), "clicked",
							  GTK_SIGNAL_FUNC (banlist_wipe), (gpointer) sess);

	sess->gui->banlist_butRefresh = gtk_button_new_with_label (_("Refresh"));
	gtk_widget_show (sess->gui->banlist_butRefresh);
	gtk_box_pack_start (GTK_BOX (vbox2), sess->gui->banlist_butRefresh, FALSE,
							  FALSE, 0);
	gtk_widget_set_usize (sess->gui->banlist_butRefresh, 80, -2);
	gtk_container_set_border_width (GTK_CONTAINER
											  (sess->gui->banlist_butRefresh), 5);

	gtk_signal_connect (GTK_OBJECT (sess->gui->banlist_butRefresh), "clicked",
							  GTK_SIGNAL_FUNC (banlist_refresh), (gpointer) sess);
/*     
   hseparator1 = gtk_hseparator_new ();
   gtk_widget_show (hseparator1);
   gtk_box_pack_start (GTK_BOX (vbox1), hseparator1, FALSE, FALSE, 0);
   
   hbox2 = gtk_hbox_new (FALSE, 0);
   gtk_widget_show (hbox2);
   gtk_box_pack_start (GTK_BOX (vbox1), hbox2, FALSE, FALSE, 0);
   
   entryBanExp = gtk_entry_new ();
   gtk_widget_show (entryBanExp);
   gtk_box_pack_start (GTK_BOX (hbox2), entryBanExp, TRUE, TRUE, 5);
	  
   butBanEntry = gtk_button_new_with_label (_("Ban"));
   gtk_widget_show (butBanEntry);
   gtk_box_pack_start (GTK_BOX (hbox2), butBanEntry, FALSE, FALSE, 0);
   gtk_widget_set_usize (butBanEntry, 80, -2);
   gtk_container_set_border_width (GTK_CONTAINER (butBanEntry), 4);
   
   butUnbanEntry = gtk_button_new_with_label (_("Unban"));
   gtk_widget_show (butUnbanEntry);
   gtk_box_pack_start (GTK_BOX (hbox2), butUnbanEntry, FALSE, FALSE, 0);
   gtk_widget_set_usize (butUnbanEntry, 80, -2);
   gtk_container_set_border_width (GTK_CONTAINER (butUnbanEntry), 4);
*/
	banlist_do_refresh (sess);

	gtk_widget_show (sess->gui->banlist_window);
}
