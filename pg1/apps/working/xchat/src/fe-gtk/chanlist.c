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
#include "../../config.h"
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#include "fe-gtk.h"
#include "../common/xchat.h"
#include "../common/xchatc.h"
#include "../common/outbound.h"
#include "../common/util.h"
#include <gdk/gdkkeysyms.h>
#include "gtkutil.h"
#include "wins.h"


/**
 * Accepts a regex_t pointer and string to test it with 
 * Returns 0 if no match, 1 if a match.
 */
#ifndef WIN32

static int
reg_match (regex_t * regexptr, const char *str)
{
	int m;

	m = regexec (regexptr, str, 1, NULL, REG_NOTBOL);

	/* regex returns 0 if it's a match: */
	return m == 0 ? 1 : 0;
}

#else

static int
reg_match (char ** regexptr, const char *str)
{
	return match (*regexptr, str);
}

#endif

/**
 * Sorts the channel list based upon the user field.
 */
static gint
chanlist_compare_user (GtkCList * clist,
							  gconstpointer ptr1, gconstpointer ptr2)
{
	int int1;
	int int2;

	GtkCListRow *row1 = (GtkCListRow *) ptr1;
	GtkCListRow *row2 = (GtkCListRow *) ptr2;

	int1 = atoi (GTK_CELL_TEXT (row1->cell[clist->sort_column])->text);
	int2 = atoi (GTK_CELL_TEXT (row2->cell[clist->sort_column])->text);

	return int1 > int2 ? 1 : -1;
}

/**
 * Provides the default case-insensitive sorting for the channel 
 * list.
 */
static gint
chanlist_compare_text_ignore_case (GtkCList * clist,
											  gconstpointer ptr1, gconstpointer ptr2)
{
	GtkCListRow *row1 = (GtkCListRow *) ptr1;
	GtkCListRow *row2 = (GtkCListRow *) ptr2;

	return strcasecmp (GTK_CELL_TEXT (row1->cell[clist->sort_column])->text,
							 GTK_CELL_TEXT (row2->cell[clist->sort_column])->text);
}

/**
 * Updates the caption to reflect the number of users and channels
 */
static void
chanlist_update_caption (struct server *serv)
{
	static gchar *title =
		N_("User and Channel Statistics: %d/%d Users on %d/%d Channels");

	gchar tbuf[256];

	snprintf (tbuf, sizeof tbuf, _(title),
				 serv->gui->chanlist_users_shown_count,
				 serv->gui->chanlist_users_found_count,
				 serv->gui->chanlist_channels_shown_count,
				 serv->gui->chanlist_channels_found_count);

	gtk_label_set_text (GTK_LABEL (serv->gui->chanlist_label), tbuf);
}

/**
 * Resets the various integer counters 
 */
static void
chanlist_reset_counters (struct server *serv)
{
	serv->gui->chanlist_users_found_count = 0;
	serv->gui->chanlist_users_shown_count = 0;
	serv->gui->chanlist_channels_found_count = 0;
	serv->gui->chanlist_channels_shown_count = 0;

	chanlist_update_caption (serv);
}

/**
 * Resets the vars that keep track of sort options.
 */
static void
chanlist_reset_sort_vars (struct server *serv)
{
	serv->gui->chanlist_sort_type = GTK_SORT_ASCENDING;
	serv->gui->chanlist_last_column = 0;
}

/**
 * Frees up the dynamic memory needed to store the channel information.
 */
static void
chanlist_data_free (struct server *serv)
{
	GSList *rows;
	gchar **data;

	if (serv->gui->chanlist_data_stored_rows)
	{

		for (rows = serv->gui->chanlist_data_stored_rows; rows != NULL;
			  rows = rows->next)
		{
			data = (gchar **) rows->data;
			free (data[0]);
			free (data[1]);
			free (data[2]);
			free (data);
		}

		g_slist_free (serv->gui->chanlist_data_stored_rows);
		serv->gui->chanlist_data_stored_rows = NULL;
	}
}

/**
 * Prepends a row of channel information to the chanlist_data_stored_rows 
 * GSList.
 */
static void
chanlist_data_prepend_row (struct server *serv, gchar ** next_row)
{
	serv->gui->chanlist_data_stored_rows =
		g_slist_prepend (serv->gui->chanlist_data_stored_rows, next_row);
}

/**
 * Places a data row into the gui GtkCList, if and only if the row matches
 * the user and regex requirements.
 */
static void
chanlist_place_row_in_gui (struct server *serv, gchar ** next_row)
{
	int num_users = atoi (next_row[1]);

	/* First, update the 'found' counter values */
	serv->gui->chanlist_users_found_count += num_users;
	serv->gui->chanlist_channels_found_count++;

	if (num_users < serv->gui->chanlist_minusers)
	{
		chanlist_update_caption (serv);
		return;
	}

	if (num_users > serv->gui->chanlist_maxusers
		 && serv->gui->chanlist_maxusers > 0)
	{
		chanlist_update_caption (serv);
		return;
	}

	if (serv->gui->chanlist_wild_text && serv->gui->chanlist_wild_text[0])
	{
		/* Check what the user wants to match. If both buttons or _neither_
		 * button is checked, look for match in both by default. 
		 */
		if ((serv->gui->chanlist_match_wants_channel
			  && serv->gui->chanlist_match_wants_topic)
			 ||
			 (!serv->gui->chanlist_match_wants_channel
			  && !serv->gui->chanlist_match_wants_topic))
		{
			if (!reg_match (&serv->gui->chanlist_match_regex, next_row[0])
				 && !reg_match (&serv->gui->chanlist_match_regex, next_row[2]))
			{
				chanlist_update_caption (serv);
				return;
			}
		}

		else if (serv->gui->chanlist_match_wants_channel)
		{
			if (!reg_match (&serv->gui->chanlist_match_regex, next_row[0]))
			{
				chanlist_update_caption (serv);
				return;
			}
		}

		else if (serv->gui->chanlist_match_wants_topic)
		{
			if (!reg_match (&serv->gui->chanlist_match_regex, next_row[2]))
			{
				chanlist_update_caption (serv);
				return;
			}
		}
	}

	/*
	 * If all the above above tests passed or if no text was in the 
	 * chanlist_wild_text, add this entry to the GUI
	 */
	gtk_clist_prepend (GTK_CLIST (serv->gui->chanlist_list), next_row);

	/* Update the 'shown' counter values */
	serv->gui->chanlist_users_shown_count += num_users;
	serv->gui->chanlist_channels_shown_count++;

	chanlist_update_caption (serv);
}

/**
 * Performs the actual refresh operations.
 */
static void
chanlist_do_refresh (struct server *serv)
{
	if (serv->connected)
	{
		chanlist_data_free (serv);
		chanlist_reset_counters (serv);
		chanlist_update_caption (serv);

		gtk_clist_clear (GTK_CLIST (serv->gui->chanlist_list));
		gtk_widget_set_sensitive (serv->gui->chanlist_refresh, FALSE);

		handle_command ("/LIST", serv->front_session, FALSE, FALSE);
	} else
		gtkutil_simpledialog ("Not connected.");
}

static void
chanlist_refresh (GtkWidget * wid, struct server *serv)
{
	/* JG NOTE: Didn't see actual use of wid here, so just forwarding
	 * this to chanlist_do_refresh because I use it without any widget
	 * param in chanlist_build_gui_list when the user presses enter
	 * or apply for the first time if the list has not yet been 
	 * received.
	 */
	chanlist_do_refresh (serv);
}

/**
 * Fills the gui GtkCList with stored items from the GSList.
 */
static void
chanlist_build_gui_list (struct server *serv)
{
	GSList *rows;
	GtkCList *clist;

	/* first check if the list is present */
	if (serv->gui->chanlist_data_stored_rows == NULL)
	{
		chanlist_do_refresh (serv);
		return;
	}

	clist = GTK_CLIST (serv->gui->chanlist_list);

	/* turn off sorting because this _greatly_ quickens the reinsertion */
	gtk_clist_set_auto_sort (clist, FALSE);
	/* freeze that GtkCList to make it go fasssster as well */
	gtk_clist_freeze (clist);
	gtk_clist_clear (clist);

	/* Reset the counters */
	chanlist_reset_counters (serv);

	/* Refill the list */
	for (rows = serv->gui->chanlist_data_stored_rows; rows != NULL;
		  rows = rows->next)
	{
		chanlist_place_row_in_gui (serv, (gchar **) rows->data);
	}

	gtk_clist_thaw (clist);
	gtk_clist_set_auto_sort (clist, TRUE);
	gtk_clist_sort (clist);
}

/**
 * Accepts incoming channel data from inbound.c, allocates new space for a
 * gchar**, forwards newly allocated row to chanlist_data_prepend_row
 * and chanlist_place_row_in_gui.
 */
void
fe_add_chan_list (struct server *serv, char *chan, char *users, char *topic)
{
	gchar **next_row;

	next_row = (gchar **) malloc (sizeof (gchar *) * 3);
	next_row[0] = strdup (chan);
	next_row[1] = strdup (users);
	next_row[2] = strip_color (topic);

	/* add this row to the data */
	chanlist_data_prepend_row (serv, next_row);

	/* _possibly_ add the row to the gui */
	chanlist_place_row_in_gui (serv, next_row);
}

/**
 * The next several functions simply handle signals from widgets to update 
 * the list and state variables. 
 */
static void
chanlist_editable_keypress (GtkWidget * widget, GdkEventKey * event,
									 struct server *serv)
{
	if (event->keyval == GDK_Return)
		chanlist_build_gui_list (serv);
}

static void
chanlist_apply_pressed (GtkButton * button, struct server *serv)
{
	chanlist_build_gui_list (serv);
}

static void
chanlist_wild (GtkWidget * wid, struct server *serv)
{
	/* Store the pattern text in the wild_text var so next time the window is 
	 * opened it is remembered. 
	 */
	strncpy (serv->gui->chanlist_wild_text,
				gtk_entry_get_text (GTK_ENTRY (wid)), 255);

#ifdef WIN32
	serv->gui->chanlist_match_regex =
		strdup (gtk_entry_get_text (GTK_ENTRY (wid)));
#else
	/* recompile the regular expression. */
	regfree (&serv->gui->chanlist_match_regex);
	regcomp (&serv->gui->chanlist_match_regex,
				gtk_entry_get_text (GTK_ENTRY (wid)),
				REG_ICASE | REG_EXTENDED | REG_NOSUB);
#endif
}

static void
chanlist_match_channel_button_toggled (GtkWidget * wid, struct server *serv)
{
	serv->gui->chanlist_match_wants_channel =
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (wid));
}

static void
chanlist_match_topic_button_toggled (GtkWidget * wid, struct server *serv)
{
	serv->gui->chanlist_match_wants_topic =
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (wid));
}

static void
chanlist_click_column (GtkWidget * clist, gint column, struct server *serv)
{
	/* If the user clicks the same column twice in a row,
	 * swap the sort type. Otherwise, assume he wishes
	 * to sort by another column, but using the same
	 * direction. 
	 */

	if (serv->gui->chanlist_last_column == column)
	{
		if (serv->gui->chanlist_sort_type == GTK_SORT_ASCENDING)
			serv->gui->chanlist_sort_type = GTK_SORT_DESCENDING;
		else
			serv->gui->chanlist_sort_type = GTK_SORT_ASCENDING;
	}

	serv->gui->chanlist_last_column = column;

	gtk_clist_set_sort_type (GTK_CLIST (clist), serv->gui->chanlist_sort_type);
	gtk_clist_set_sort_column (GTK_CLIST (clist), column);

	/* Since ascii sorting the numbers is a no go, use a custom
	 * sorter function for the 'users' column.
	 */
	if (column == 1)
		gtk_clist_set_compare_func (GTK_CLIST (clist), (GtkCListCompareFunc)
											 chanlist_compare_user);
	else
		/* In the 0 or 2 case, use the case-insensitive string 
		 * compare function. 
		 */
		gtk_clist_set_compare_func (GTK_CLIST (clist), (GtkCListCompareFunc)
											 chanlist_compare_text_ignore_case);

	gtk_clist_sort (GTK_CLIST (clist));
}

static void
chanlist_join (GtkWidget * wid, struct server *serv)
{
	int row;
	char *chan;
	char tbuf[256];

	row = gtkutil_clist_selection (serv->gui->chanlist_list);
	if (row != -1)
	{
		gtk_clist_get_text (GTK_CLIST (serv->gui->chanlist_list), row, 0,
								  &chan);
		if (serv->connected && (strcmp (chan, "*") != 0))
		{
			snprintf (tbuf, sizeof tbuf, "JOIN %s\r\n", chan);
			tcp_send (serv, tbuf);
		} else
			gdk_beep ();
	}
}

static void
chanlist_filereq_done (struct server *serv, void *data2, char *file)
{
	time_t t = time (0);
	int i = 0;
	int fh;
	char *chan, *users, *topic;
	char buf[1024];

	if (!file)
		return;

	fh = open (file, O_TRUNC | O_WRONLY | O_CREAT, 0600);
	free (file);

	if (fh == -1)
		return;

	snprintf (buf, sizeof buf, "X-Chat Channel List: %s - %s\n",
				 serv->servername, ctime (&t));
	write (fh, buf, strlen (buf));

	while (1)
	{
		if (!gtk_clist_get_text
			 (GTK_CLIST (serv->gui->chanlist_list), i, 0, &chan))
			break;
		gtk_clist_get_text (GTK_CLIST (serv->gui->chanlist_list), i, 1, &users);
		gtk_clist_get_text (GTK_CLIST (serv->gui->chanlist_list), i, 2, &topic);
		i++;
		snprintf (buf, sizeof buf, "%-16s %-5s%s\n", chan, users, topic);
		write (fh, buf, strlen (buf));
	}

	close (fh);
}

static void
chanlist_save (GtkWidget * wid, struct server *serv)
{
	char *temp;

	if (!gtk_clist_get_text
		 (GTK_CLIST (serv->gui->chanlist_list), 0, 0, &temp))
	{
		gtkutil_simpledialog (_("I can't save an empty list!"));
		return;
	}
	gtkutil_file_req (_("Select an output filename"), chanlist_filereq_done,
							serv, 0, TRUE);
}

static void
chanlist_minusers (GtkWidget * wid, struct server *serv)
{
	serv->gui->chanlist_minusers = atoi (gtk_entry_get_text (GTK_ENTRY (wid)));
}

static void
chanlist_maxusers (GtkWidget * wid, struct server *serv)
{
	serv->gui->chanlist_maxusers = atoi (gtk_entry_get_text (GTK_ENTRY (wid)));
}

static void
chanlist_row_selected (GtkWidget * clist, gint row, gint column,
							  GdkEventButton * even, struct server *serv)
{
	if (even && even->type == GDK_2BUTTON_PRESS)
	{
		chanlist_join (0, (struct server *) serv);
	}
}

/**
 * Handles the window's destroy event to free allocated memory.
 */
static void
chanlist_destroy_widget (GtkObject * object, struct server *serv)
{
	chanlist_data_free (serv);
#ifndef WIN32
	regfree (&serv->gui->chanlist_match_regex);
#else
	free (serv->gui->chanlist_match_regex);
#endif
}

static void
chanlist_closegui (server *serv)
{
	if (is_server (serv))
		serv->gui->chanlist_window = 0;
}

void
chanlist_opengui (struct server *serv)
{
	gchar *titles[] = { _("Channel"), _("Users"), _("Topic") };
	GtkWidget *frame, *vbox, *hbox, *sortvbox, *sorthbox, *numtable,
		*table, *wid, *real_wid;
	char tbuf[256];

	if (serv->gui->chanlist_window)
	{
		gdk_window_show (serv->gui->chanlist_window->window);
		return;
	}

	snprintf (tbuf, sizeof tbuf, _("X-Chat: Channel List (%s)"),
				 serv->servername);

	serv->gui->chanlist_data_stored_rows = NULL;

	if (!serv->gui->chanlist_minusers)
		serv->gui->chanlist_minusers = 3;

	if (!serv->gui->chanlist_maxusers)
		serv->gui->chanlist_maxusers = 0;


	serv->gui->chanlist_window =
		maingui_window ("chanlist", tbuf, FALSE, TRUE, chanlist_closegui, serv,
								450, 300, &serv->gui->chanlist_window);
	vbox = wins_get_vbox (serv->gui->chanlist_window);

	frame = gtk_frame_new (_("List display options:"));
	gtk_container_set_border_width (GTK_CONTAINER (frame), 2);
	gtk_widget_show (frame);

	table = gtk_table_new (2, 3, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (table), 2);
	gtk_container_add (GTK_CONTAINER (frame), table);
	gtk_widget_show (table);
	gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 5);

	wid = gtk_alignment_new (0.0, 0.0, 0.0, 0.0);
	gtk_widget_show (wid);
	numtable = gtk_table_new (2, 2, FALSE);
	gtk_container_add (GTK_CONTAINER (wid), numtable);
	gtk_table_attach_defaults (GTK_TABLE (table), wid, 0, 1, 0, 1);
	gtk_widget_show (numtable);

	wid = gtk_alignment_new (0.0, 0.0, 0.0, 0.0);
	real_wid = gtk_label_new (_("Minimum Users: "));
	gtk_container_add (GTK_CONTAINER (wid), real_wid);
	gtk_table_attach_defaults (GTK_TABLE (numtable), wid, 0, 1, 0, 1);
	gtk_widget_show (wid);

	wid = gtk_entry_new_with_max_length (6);
	gtk_widget_set_usize (wid, 40, 0);
	sprintf (tbuf, "%d", serv->gui->chanlist_minusers);
	gtk_entry_set_text (GTK_ENTRY (wid), tbuf);
	gtk_signal_connect (GTK_OBJECT (wid), "changed",
							  GTK_SIGNAL_FUNC (chanlist_minusers), serv);
	gtk_signal_connect (GTK_OBJECT (wid), "key_press_event",
							  GTK_SIGNAL_FUNC (chanlist_editable_keypress),
							  (gpointer) serv);
	gtk_table_attach_defaults (GTK_TABLE (numtable), wid, 1, 2, 0, 1);
	gtk_widget_show (real_wid);
	gtk_widget_show (wid);

	wid = gtk_alignment_new (0.0, 0.0, 0.0, 0.0);
	real_wid = gtk_label_new (_("Maximum Users: "));
	gtk_container_add (GTK_CONTAINER (wid), real_wid);
	gtk_table_attach_defaults (GTK_TABLE (numtable), wid, 0, 1, 1, 2);
	gtk_widget_show (real_wid);
	gtk_widget_show (wid);

	wid = gtk_entry_new_with_max_length (6);
	gtk_widget_set_usize (wid, 40, 0);
	if (serv->gui->chanlist_maxusers > 0)
		sprintf (tbuf, "%d", serv->gui->chanlist_maxusers);
	else
		strcpy (tbuf, "");
	gtk_entry_set_text (GTK_ENTRY (wid), tbuf);
	gtk_signal_connect (GTK_OBJECT (wid), "changed",
							  GTK_SIGNAL_FUNC (chanlist_maxusers), serv);
	gtk_signal_connect (GTK_OBJECT (wid), "key_press_event",
							  GTK_SIGNAL_FUNC (chanlist_editable_keypress),
							  (gpointer) serv);
	gtk_table_attach_defaults (GTK_TABLE (numtable), wid, 1, 2, 1, 2);
	gtk_widget_show (wid);

	wid = gtk_alignment_new (1.0, 0.0, 0.0, 0.0);
	sorthbox = gtk_hbox_new (FALSE, 2);
	gtk_container_add (GTK_CONTAINER (wid), sorthbox);
	gtk_widget_show (sorthbox);

	gtk_table_attach_defaults (GTK_TABLE (table), wid, 2, 3, 0, 1);
	gtk_widget_show (wid);

	wid = gtk_alignment_new (1.0, 0.0, 1.0, 0.0);
	real_wid = gtk_label_new (_("Regex Match: "));
	gtk_container_add (GTK_CONTAINER (wid), real_wid);
	gtk_box_pack_start (GTK_BOX (sorthbox), wid, 0, 0, 0);
	gtk_widget_show (real_wid);
	gtk_widget_show (wid);

	sortvbox = gtk_vbox_new (FALSE, 2);
	gtk_box_pack_start (GTK_BOX (sorthbox), sortvbox, 0, 0, 0);
	gtk_widget_show (sortvbox);

	wid = gtk_entry_new_with_max_length (255);
	gtk_widget_set_usize (wid, 155, 0);
	gtk_entry_set_text (GTK_ENTRY (wid), serv->gui->chanlist_wild_text);
	gtk_signal_connect (GTK_OBJECT (wid), "changed",
							  GTK_SIGNAL_FUNC (chanlist_wild), serv);
	gtk_signal_connect (GTK_OBJECT (wid), "key_press_event",
							  GTK_SIGNAL_FUNC (chanlist_editable_keypress),
							  (gpointer) serv);
	gtk_box_pack_start (GTK_BOX (sortvbox), wid, 0, 0, 0);
	gtk_widget_show (wid);
	serv->gui->chanlist_wild = wid;

	chanlist_wild (wid, serv);

	sorthbox = gtk_hbox_new (FALSE, 2);
	gtk_box_pack_start (GTK_BOX (sortvbox), sorthbox, 0, 0, 0);
	gtk_widget_show (sorthbox);

	wid = gtk_check_button_new_with_label (_("Channel"));
	gtk_box_pack_start (GTK_BOX (sorthbox), wid, 0, 0, 0);
	gtk_signal_connect (GTK_OBJECT (wid), "toggled",
							  GTK_SIGNAL_FUNC
							  (chanlist_match_channel_button_toggled), serv);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (wid), TRUE);
	gtk_widget_show (wid);

	wid = gtk_check_button_new_with_label (_("Topic"));
	gtk_signal_connect (GTK_OBJECT (wid), "toggled",
							  GTK_SIGNAL_FUNC (chanlist_match_topic_button_toggled),
							  serv);
	gtk_box_pack_start (GTK_BOX (sorthbox), wid, 0, 0, 0);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (wid), TRUE);
	gtk_widget_show (wid);

	wid = gtk_alignment_new (0.5, 0.0, 0.0, 0.0);
	real_wid = gtk_button_new_with_label (_(" Apply "));
	gtk_container_add (GTK_CONTAINER (wid), real_wid);
	gtk_table_attach_defaults (GTK_TABLE (table), wid, 0, 3, 1, 2);
	gtk_signal_connect (GTK_OBJECT (real_wid), "pressed",
							  GTK_SIGNAL_FUNC (chanlist_apply_pressed),
							  (gpointer) serv);
	gtk_widget_show (real_wid);
	gtk_widget_show (wid);

	serv->gui->chanlist_list =
		gtkutil_clist_new (3, titles, vbox, GTK_POLICY_ALWAYS,
								 chanlist_row_selected, (gpointer) serv, 0, 0,
								 GTK_SELECTION_BROWSE);
	gtk_clist_set_column_width (GTK_CLIST (serv->gui->chanlist_list), 0, 90);
	gtk_clist_set_column_width (GTK_CLIST (serv->gui->chanlist_list), 1, 45);
	gtk_clist_set_column_width (GTK_CLIST (serv->gui->chanlist_list), 2, 165);
	gtk_clist_column_titles_active (GTK_CLIST (serv->gui->chanlist_list));
	gtk_signal_connect (GTK_OBJECT (serv->gui->chanlist_list), "click_column",
							  GTK_SIGNAL_FUNC (chanlist_click_column),
							  (gpointer) serv);
	gtk_clist_set_compare_func (GTK_CLIST (serv->gui->chanlist_list),
										 (GtkCListCompareFunc)
										 chanlist_compare_text_ignore_case);
	gtk_clist_set_sort_column (GTK_CLIST (serv->gui->chanlist_list), 0);
	gtk_clist_set_auto_sort (GTK_CLIST (serv->gui->chanlist_list), 1);
	/* makes the horiz. scrollbar appear when needed */
	gtk_clist_set_column_auto_resize (GTK_CLIST (serv->gui->chanlist_list),
											2, TRUE);

	/* make a label to store the user/channel info */
	wid = gtk_label_new ("");
	gtk_widget_show (wid);
	gtk_box_pack_start (GTK_BOX (vbox), wid, 0, 0, 0);

	serv->gui->chanlist_label = wid;

	hbox = gtk_hbox_new (0, 1);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
	gtk_box_pack_end (GTK_BOX (vbox), hbox, 0, 0, 0);
	gtk_widget_show (hbox);

	serv->gui->chanlist_refresh =
		gtkutil_button (serv->gui->chanlist_window, GNOME_STOCK_PIXMAP_REFRESH,
							 _("Refresh the list"), chanlist_refresh, serv, hbox);
	gtkutil_button (serv->gui->chanlist_window, GNOME_STOCK_PIXMAP_SAVE,
						 _("Save the list"), chanlist_save, serv, hbox);
	gtkutil_button (serv->gui->chanlist_window, GNOME_STOCK_PIXMAP_JUMP_TO,
						 _("Join Channel"), chanlist_join, serv, hbox);

	/* connect a destroy event handler to this window so that the dynamic
	   memory can be freed */
	gtk_signal_connect (GTK_OBJECT (serv->gui->chanlist_window), "destroy",
							  GTK_SIGNAL_FUNC (chanlist_destroy_widget),
							  (gpointer) serv);

	/* reset the sort vars and counters. */
	chanlist_reset_counters (serv);
	chanlist_reset_sort_vars (serv);

	gtk_widget_show (serv->gui->chanlist_window);
}
