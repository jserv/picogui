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
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#define WANTSOCKET
#define WANTARPA
#include "../common/inet.h"
#include "fe-gtk.h"
#include "../common/xchat.h"
#include "../common/xchatc.h"
#include "../common/plugin.h"
#include "../common/fe.h"
#include "../common/util.h"
#include "../common/network.h"
#include "gtkutil.h"
#include "wins.h"
#include "palette.h"

struct dccwindow
{
	GtkWidget *window;
	GtkWidget *list;
};

struct dcc_send
{
	struct session *sess;
	char nick[64];
	GtkWidget *freq;
};

static struct dccwindow dccrwin;	/* recv */
static struct dccwindow dccswin;	/* send */
static struct dccwindow dcccwin;	/* chat */


static void
dcc_send_filereq_done (struct session *sess, char *nick, char *file)
{
	char tbuf[400];

	if (file)
	{
		dcc_send (sess, tbuf, nick, file);
		free (file);
	}
	free (nick);
}

void
fe_dcc_send_filereq (struct session *sess, char *nick)
{
	char tbuf[128];

	nick = strdup (nick);
	snprintf (tbuf, sizeof tbuf, _("Send file to %s"), nick);
	gtkutil_file_req (tbuf, dcc_send_filereq_done, sess, nick, FALSE);
}

void
fe_dcc_update_recv (struct DCC *dcc)
{
	char pos[14], cps[14], perc[14], eta[14];
	gint row;
	int to_go;
	float per;

	if (!dccrwin.window)
		return;

	row =
		gtk_clist_find_row_from_data (GTK_CLIST (dccrwin.list), (gpointer) dcc);

	/* percentage done */
	per = (float) ((dcc->pos * 100.00) / dcc->size);

	snprintf (pos, sizeof (pos), "%d", dcc->pos);
	snprintf (cps, sizeof (cps), "%d", dcc->cps);
	snprintf (perc, sizeof (perc), "%.0f%%", per);

	gtk_clist_freeze (GTK_CLIST (dccrwin.list));
	gtk_clist_set_text (GTK_CLIST (dccrwin.list), row, 0,
							  _(dccstat[(int) dcc->dccstat].name));
	gtk_clist_set_text (GTK_CLIST (dccrwin.list), row, 3, pos);
	gtk_clist_set_text (GTK_CLIST (dccrwin.list), row, 4, perc);
	gtk_clist_set_text (GTK_CLIST (dccrwin.list), row, 5, cps);
	if (dcc->cps != 0)
	{
		to_go = (dcc->size - dcc->pos) / dcc->cps;
		snprintf (eta, sizeof (eta), "%.2d:%.2d:%.2d",
					 to_go / 3600, (to_go / 60) % 60, to_go % 60);
	} else
		strcpy (eta, "--:--:--");
	gtk_clist_set_text (GTK_CLIST (dccrwin.list), row, 6, eta);
	if (dccstat[(int) dcc->dccstat].color != 1)
		gtk_clist_set_foreground
			(GTK_CLIST (dccrwin.list), row,
			 colors + dccstat[(int) dcc->dccstat].color);
#ifdef USE_GNOME
	if (dcc->dccstat == STAT_DONE)
		gtk_clist_set_text (GTK_CLIST (dccrwin.list), row, 8,
								  (char *) gnome_mime_type_of_file (dcc->destfile));
#endif
	gtk_clist_thaw (GTK_CLIST (dccrwin.list));
}

void
fe_dcc_update_send (struct DCC *dcc)
{
	char pos[14], cps[14], ack[14], perc[14], eta[14];
	gint row;
	int to_go;
	float per;

	if (!dccswin.window)
		return;

	row =
		gtk_clist_find_row_from_data (GTK_CLIST (dccswin.list), (gpointer) dcc);

	/* percentage ack'ed */
	per = (float) ((dcc->ack * 100.00) / dcc->size);

	snprintf (pos, sizeof (pos), "%d", dcc->pos);
	snprintf (cps, sizeof (cps), "%d", dcc->cps);
	snprintf (ack, sizeof (ack), "%d", dcc->ack);
	snprintf (perc, sizeof (perc), "%.0f%%", per);
	if (dcc->cps != 0)
	{
		to_go = (dcc->size - dcc->ack) / dcc->cps;
		snprintf (eta, sizeof (eta), "%.2d:%.2d:%.2d",
					 to_go / 3600, (to_go / 60) % 60, to_go % 60);
	} else
		strcpy (eta, "--:--:--");
	gtk_clist_freeze (GTK_CLIST (dccswin.list));
	gtk_clist_set_text (GTK_CLIST (dccswin.list), row, 0,
							  _(dccstat[(int) dcc->dccstat].name));
	if (dccstat[(int) dcc->dccstat].color != 1)
		gtk_clist_set_foreground
			(GTK_CLIST (dccswin.list), row,
			 colors + dccstat[(int) dcc->dccstat].color);
	gtk_clist_set_text (GTK_CLIST (dccswin.list), row, 3, pos);
	gtk_clist_set_text (GTK_CLIST (dccswin.list), row, 4, ack);
	gtk_clist_set_text (GTK_CLIST (dccswin.list), row, 5, perc);
	gtk_clist_set_text (GTK_CLIST (dccswin.list), row, 6, cps);
	gtk_clist_set_text (GTK_CLIST (dccswin.list), row, 7, eta);
	gtk_clist_thaw (GTK_CLIST (dccswin.list));
}

static void
dcc_abort (struct DCC *dcc)
{
	if (dcc)
	{
		switch (dcc->dccstat)
		{
		case STAT_QUEUED:
		case STAT_CONNECTING:
		case STAT_ACTIVE:
			dcc_close (dcc, STAT_ABORTED, FALSE);

			EMIT_SIGNAL (XP_TE_DCCABORT, dcc->serv->front_session,
							 dcctypes[(int) dcc->type], file_part (dcc->file),
							 dcc->nick, NULL, 0);
			break;
		default:
			dcc_close (dcc, 0, TRUE);
		}
	}
}

static void
close_dcc_recv_window (void)
{
	dccrwin.window = 0;
}

void
fe_dcc_update_recv_win (void)
{
	struct DCC *dcc;
	GSList *list = dcc_list;
	gchar *nnew[9];
	char size[16];
	char pos[16];
	char cps[16];
	char perc[14];
	char eta[16];
	gint row;
	int selrow;
	int to_go;
	float per;

	if (!dccrwin.window)
		return;

	selrow = gtkutil_clist_selection (dccrwin.list);

	gtk_clist_clear ((GtkCList *) dccrwin.list);
	nnew[2] = size;
	nnew[3] = pos;
	nnew[4] = perc;
	nnew[5] = cps;
	nnew[6] = eta;
	while (list)
	{
		dcc = (struct DCC *) list->data;
		if (dcc->type == TYPE_RECV)
		{
			nnew[0] = _(dccstat[(int) dcc->dccstat].name);
			nnew[1] = dcc->file;
			nnew[7] = dcc->nick;
#ifdef USE_GNOME
			if (dcc->dccstat == STAT_DONE)
				nnew[8] = (char *) gnome_mime_type_of_file (dcc->destfile);
			else
				nnew[8] = "";
#endif
			sprintf (size, "%d", dcc->size);
			if (dcc->dccstat == STAT_QUEUED)
				sprintf (pos, "%d", dcc->resumable);
			else
				sprintf (pos, "%d", dcc->pos);
			snprintf (cps, sizeof (cps), "%d", dcc->cps);
			/* percentage recv'ed */
			per = (float) ((dcc->pos * 100.00) / dcc->size);
			snprintf (perc, sizeof (perc), "%.0f%%", per);
			if (dcc->cps != 0)
			{
				to_go = (dcc->size - dcc->pos) / dcc->cps;
				snprintf (eta, sizeof (eta), "%.2d:%.2d:%.2d",
							 to_go / 3600, (to_go / 60) % 60, to_go % 60);
			} else
				strcpy (eta, "--:--:--");
			row = gtk_clist_append (GTK_CLIST (dccrwin.list), nnew);
			gtk_clist_set_row_data (GTK_CLIST (dccrwin.list), row,
											(gpointer) dcc);
			if (dccstat[(int) dcc->dccstat].color != 1)
				gtk_clist_set_foreground (GTK_CLIST (dccrwin.list), row,
												  colors +
												  dccstat[(int) dcc->dccstat].color);
		}
		list = list->next;
	}
	if (selrow != -1)
		gtk_clist_select_row ((GtkCList *) dccrwin.list, selrow, 0);
}

static void
dcc_info (struct DCC *dcc)
{
	char tbuf[256];
	snprintf (tbuf, 255, _("      File: %s\n"
				 "   To/From: %s\n"
				 "      Size: %d\n"
				 "      Port: %d\n"
				 " IP Number: %s\n"
				 "Start Time: %s"),
				 dcc->file,
				 dcc->nick,
				 dcc->size,
				 dcc->port,
				 net_ip (dcc->addr), ctime (&dcc->starttime));
	gtkutil_simpledialog (tbuf);
}

static void
resume_clicked (GtkWidget * wid, gpointer none)
{
	int row;
	struct DCC *dcc;

	row = gtkutil_clist_selection (dccrwin.list);
	if (row != -1)
	{
		dcc = gtk_clist_get_row_data (GTK_CLIST (dccrwin.list), row);
		gtk_clist_unselect_row (GTK_CLIST (dccrwin.list), row, 0);
		dcc_resume (dcc);
	}
}

static void
abort_clicked (GtkWidget * wid, gpointer none)
{
	int row;
	struct DCC *dcc;

	row = gtkutil_clist_selection (dccrwin.list);
	if (row != -1)
	{
		dcc = gtk_clist_get_row_data (GTK_CLIST (dccrwin.list), row);
		dcc_abort (dcc);
	}
}

static void
accept_clicked (GtkWidget * wid, gpointer none)
{
	int row;
	struct DCC *dcc;

	row = gtkutil_clist_selection (dccrwin.list);
	if (row != -1)
	{
		dcc = gtk_clist_get_row_data (GTK_CLIST (dccrwin.list), row);
		gtk_clist_unselect_row (GTK_CLIST (dccrwin.list), row, 0);
		dcc_get (dcc);
	}
}

static void
info_clicked (GtkWidget * wid, gpointer none)
{
	int row;
	struct DCC *dcc;

	row = gtkutil_clist_selection (dccrwin.list);
	if (row != -1)
	{
		dcc = gtk_clist_get_row_data (GTK_CLIST (dccrwin.list), row);
		if (dcc)
			dcc_info (dcc);
	}
}

#ifdef USE_GNOME

static void
open_clicked (void)
{
	int row;
	struct DCC *dcc;
	char *mime_type;
	char *mime_prog;
	char *tmp;

	row = gtkutil_clist_selection (dccrwin.list);
	if (row != -1)
	{
		dcc = gtk_clist_get_row_data (GTK_CLIST (dccrwin.list), row);
		if (dcc && dcc->dccstat == STAT_DONE)
		{
			mime_type = (char *) gnome_mime_type (dcc->destfile);
			if (mime_type)
			{
				mime_prog = (char *) gnome_mime_program (mime_type);
				if (mime_prog)
				{
					mime_prog = strdup (mime_prog);
					tmp = strstr (mime_prog, "%f");
					if (tmp)
					{
						tmp[1] = 's';
						tmp = malloc (strlen (dcc->destfile) + strlen (mime_prog));
						sprintf (tmp, mime_prog, dcc->destfile);
						xchat_exec (tmp);
						free (tmp);
					}
					free (mime_prog);
				}
			}
		}
	}
}

#endif

static void
recv_row_selected (GtkWidget * clist, gint row, gint column,
						 GdkEventButton * even)
{
	if (even && even->type == GDK_2BUTTON_PRESS)
		accept_clicked (0, 0);
}

void
fe_dcc_open_recv_win (int passive)
{
	GtkWidget *vbox, *bbox;
#ifdef USE_GNOME
	gchar *titles[] =
		{ _("Status"), _("File"), _("Size"), _("Position"), "%", _("CPS"), _("ETA"), _("From"),
			_("MIME Type") };
#else
	gchar *titles[] =
		{ _("Status"), _("File"), _("Size"), _("Position"), "%", _("CPS"), _("ETA"), _("From") };
#endif

	if (dccrwin.window)
	{
		fe_dcc_update_recv_win ();
		if (!passive)
			wins_bring_tofront (dccrwin.window);
		return;
	}
	dccrwin.window = maingui_window ("dccrecv", _("X-Chat: File Receive List"),
								FALSE, TRUE, close_dcc_recv_window, NULL, 601, 0,
								&dccrwin.window);
	vbox = wins_get_vbox (dccrwin.window);

#ifdef USE_GNOME
	dccrwin.list = gtkutil_clist_new (9, titles, vbox, GTK_POLICY_ALWAYS,
												 recv_row_selected, 0,
												 0, 0, GTK_SELECTION_SINGLE);
#else
	dccrwin.list = gtkutil_clist_new (8, titles, vbox, GTK_POLICY_ALWAYS,
												 recv_row_selected, 0,
												 0, 0, GTK_SELECTION_SINGLE);
#endif
	gtk_clist_set_column_width (GTK_CLIST (dccrwin.list), 0, 65);
	gtk_clist_set_column_width (GTK_CLIST (dccrwin.list), 1, 100);
	gtk_clist_set_column_width (GTK_CLIST (dccrwin.list), 2, 50);
	gtk_clist_set_column_width (GTK_CLIST (dccrwin.list), 3, 50);
	gtk_clist_set_column_width (GTK_CLIST (dccrwin.list), 4, 30);
	gtk_clist_set_column_width (GTK_CLIST (dccrwin.list), 5, 50);
	gtk_clist_set_column_width (GTK_CLIST (dccrwin.list), 6, 60);
	gtk_clist_set_column_width (GTK_CLIST (dccrwin.list), 7, 60);
	gtk_clist_set_column_justification (GTK_CLIST (dccrwin.list), 4,
													GTK_JUSTIFY_CENTER);

	bbox = gtk_hbox_new (FALSE, 2);
	gtk_box_pack_start (GTK_BOX (vbox), bbox, FALSE, FALSE, 2);
	gtk_widget_show (bbox);

	gtkutil_button (dccrwin.window, 0, _("Accept"), accept_clicked, 0, bbox);

	gtkutil_button (dccrwin.window, 0, _("Resume"), resume_clicked, 0, bbox);

	gtkutil_button (dccrwin.window, 0, _("Abort"), abort_clicked, 0, bbox);

	gtkutil_button (dccrwin.window, 0, _("Info"), info_clicked, 0, bbox);
#ifdef USE_GNOME
	gtkutil_button (dccrwin.window, 0, _("Open"), open_clicked, 0, bbox);
#endif
	gtk_widget_show (dccrwin.window);
	fe_dcc_update_recv_win ();
}

static void
close_dcc_send_window (void)
{
	dccswin.window = 0;
}

void
fe_dcc_update_send_win (void)
{
	struct DCC *dcc;
	GSList *list = dcc_list;
	gchar *nnew[9];
	char size[14];
	char pos[14];
	char cps[14];
	char ack[14];
	char perc[14];
	char eta[14];
	gint row;
	int selrow;
	int to_go;
	float per;

	if (!dccswin.window)
		return;

	selrow = gtkutil_clist_selection (dccswin.list);

	gtk_clist_clear ((GtkCList *) dccswin.list);
	nnew[2] = size;
	nnew[3] = pos;
	nnew[4] = ack;
	nnew[5] = perc;
	nnew[6] = cps;
	while (list)
	{
		nnew[7] = eta;
		dcc = (struct DCC *) list->data;
		if (dcc->type == TYPE_SEND)
		{
			nnew[0] = _(dccstat[(int) dcc->dccstat].name);
			nnew[1] = file_part (dcc->file);
			nnew[8] = dcc->nick;
			/* percentage ack'ed */
			per = (float) ((dcc->ack * 100.00) / dcc->size);
			snprintf (size, sizeof (size), "%d", dcc->size);
			snprintf (pos, sizeof (pos), "%d", dcc->pos);
			snprintf (cps, sizeof (cps), "%d", dcc->cps);
			snprintf (perc, sizeof (perc), "%.0f%%", per);
			snprintf (ack, sizeof (ack), "%d", dcc->ack);
			if (dcc->cps != 0)
			{
				to_go = (dcc->size - dcc->ack) / dcc->cps;
				snprintf (eta, sizeof (eta), "%.2d:%.2d:%.2d",
							 to_go / 3600, (to_go / 60) % 60, to_go % 60);
			} else
				strcpy (eta, "--:--:--");
			row = gtk_clist_append (GTK_CLIST (dccswin.list), nnew);
			gtk_clist_set_row_data (GTK_CLIST (dccswin.list), row,
											(gpointer) dcc);
			if (dccstat[(int) dcc->dccstat].color != 1)
				gtk_clist_set_foreground
					(GTK_CLIST (dccswin.list), row,
					 colors + dccstat[(int) dcc->dccstat].color);
		}
		list = list->next;
	}
	if (selrow != -1)
		gtk_clist_select_row ((GtkCList *) dccswin.list, selrow, 0);
}

static void
send_row_selected (GtkWidget * clist, gint row, gint column,
						 GdkEventButton * even)
{
	struct DCC *dcc;

	if (even && even->type == GDK_2BUTTON_PRESS)
	{
		dcc = gtk_clist_get_row_data (GTK_CLIST (clist), row);
		if (dcc)
		{
			switch (dcc->dccstat)
			{
			case STAT_FAILED:
			case STAT_ABORTED:
			case STAT_DONE:
				dcc_abort (dcc);
			}
		}
	}
}

static void
info_send_clicked (GtkWidget * wid, gpointer none)
{
	int row;
	struct DCC *dcc;

	row = gtkutil_clist_selection (dccswin.list);
	if (row != -1)
	{
		dcc = gtk_clist_get_row_data (GTK_CLIST (dccswin.list), row);
		if (dcc)
			dcc_info (dcc);
	}
}

static void
abort_send_clicked (GtkWidget * wid, gpointer none)
{
	int row;
	struct DCC *dcc;

	row = gtkutil_clist_selection (dccswin.list);
	if (row != -1)
	{
		dcc = gtk_clist_get_row_data (GTK_CLIST (dccswin.list), row);
		dcc_abort (dcc);
	}
}

void
fe_dcc_open_send_win (int passive)
{
	GtkWidget *vbox, *bbox;
	gchar *titles[] =
		{ _("Status"), _("File"), _("Size"), _("Position"), _("Ack"), "%", _("CPS"), _("ETA"),
			_("To") };

	if (dccswin.window)
	{
		fe_dcc_update_send_win ();
		if (!passive)
			wins_bring_tofront (dccswin.window);
		return;
	}

	dccswin.window = maingui_window ("dccsend", _("X-Chat: File Send List"),
								FALSE, TRUE, close_dcc_send_window, NULL, 595, 0,
								&dccswin.window);
	vbox = wins_get_vbox (dccswin.window);

	dccswin.list = gtkutil_clist_new (9, titles, vbox, GTK_POLICY_ALWAYS,
												 send_row_selected, 0,
												 0, 0, GTK_SELECTION_SINGLE);
	gtk_clist_set_column_width (GTK_CLIST (dccswin.list), 0, 65);
	gtk_clist_set_column_width (GTK_CLIST (dccswin.list), 1, 100);
	gtk_clist_set_column_width (GTK_CLIST (dccswin.list), 2, 50);
	gtk_clist_set_column_width (GTK_CLIST (dccswin.list), 3, 50);
	gtk_clist_set_column_width (GTK_CLIST (dccswin.list), 4, 50);
	gtk_clist_set_column_width (GTK_CLIST (dccswin.list), 5, 30);
	gtk_clist_set_column_width (GTK_CLIST (dccswin.list), 6, 50);
	gtk_clist_set_column_width (GTK_CLIST (dccswin.list), 7, 50);
	gtk_clist_set_column_justification (GTK_CLIST (dccswin.list), 5,
													GTK_JUSTIFY_CENTER);

	bbox = gtk_hbox_new (FALSE, 2);
	gtk_box_pack_start (GTK_BOX (vbox), bbox, FALSE, FALSE, 2);
	gtk_widget_show (bbox);

	gtkutil_button (dccswin.window, 0, _("Abort"), abort_send_clicked, 0, bbox);
	gtkutil_button (dccswin.window, 0, _("Info"), info_send_clicked, 0, bbox);

	gtk_widget_show (dccswin.window);
	fe_dcc_update_send_win ();
}


/* DCC CHAT GUIs BELOW */

static void
accept_chat_clicked (GtkWidget * wid, gpointer none)
{
	int row;
	struct DCC *dcc;

	row = gtkutil_clist_selection (dcccwin.list);
	if (row != -1)
	{
		dcc = gtk_clist_get_row_data (GTK_CLIST (dcccwin.list), row);
		gtk_clist_unselect_row (GTK_CLIST (dcccwin.list), row, 0);
		dcc_get (dcc);
	}
}

static void
abort_chat_clicked (GtkWidget * wid, gpointer none)
{
	int row;
	struct DCC *dcc;

	row = gtkutil_clist_selection (dcccwin.list);
	if (row != -1)
	{
		dcc = gtk_clist_get_row_data (GTK_CLIST (dcccwin.list), row);
		dcc_abort (dcc);
	}
}

static void
chat_row_selected (GtkWidget * clist, gint row, gint column,
						 GdkEventButton * even)
{
	if (even && even->type == GDK_2BUTTON_PRESS)
		accept_chat_clicked (0, 0);
}

void
fe_dcc_update_chat_win (void)
{
	struct DCC *dcc;
	GSList *list = dcc_list;
	gchar *nnew[5];
	char pos[14];
	char siz[14];
	gint row;
	int selrow;

	if (!dcccwin.window)
		return;

	selrow = gtkutil_clist_selection (dcccwin.list);

	gtk_clist_clear ((GtkCList *) dcccwin.list);
	nnew[2] = pos;
	nnew[3] = siz;
	while (list)
	{
		dcc = (struct DCC *) list->data;
		if ((dcc->type == TYPE_CHATSEND || dcc->type == TYPE_CHATRECV))
		{
			nnew[0] = _(dccstat[(int) dcc->dccstat].name);
			nnew[1] = dcc->nick;
			sprintf (pos, "%d", dcc->pos);
			sprintf (siz, "%d", dcc->size);
			nnew[4] = ctime (&dcc->starttime);
			row = gtk_clist_append (GTK_CLIST (dcccwin.list), nnew);
			gtk_clist_set_row_data (GTK_CLIST (dcccwin.list), row,
											(gpointer) dcc);
		}
		list = list->next;
	}
	if (selrow != -1)
		gtk_clist_select_row ((GtkCList *) dcccwin.list, selrow, 0);
}

static void
close_dcc_chat_window (void)
{
	dcccwin.window = 0;
}

void
fe_dcc_open_chat_win (int passive)
{
	GtkWidget *vbox, *bbox;
	gchar *titles[] =
		{ _("Status"), _("To/From"), _("Recv"), _("Sent"), _("StartTime") };

	if (dcccwin.window)
	{
		fe_dcc_update_chat_win ();
		if (!passive)
			wins_bring_tofront (dcccwin.window);
		return;
	}

	dcccwin.window = maingui_window ("dccchat", _("X-Chat: DCC Chat List"),
								FALSE, TRUE, close_dcc_chat_window, NULL, 550, 0,
								&dcccwin.window);
	vbox = wins_get_vbox (dcccwin.window);

	dcccwin.list = gtkutil_clist_new (5, titles, vbox, GTK_POLICY_ALWAYS,
												 chat_row_selected, 0,
												 0, 0, GTK_SELECTION_BROWSE);
	gtk_clist_set_column_width (GTK_CLIST (dcccwin.list), 0, 65);
	gtk_clist_set_column_width (GTK_CLIST (dcccwin.list), 1, 100);
	gtk_clist_set_column_width (GTK_CLIST (dcccwin.list), 2, 65);
	gtk_clist_set_column_width (GTK_CLIST (dcccwin.list), 3, 65);

	bbox = gtk_hbox_new (FALSE, 2);
	gtk_box_pack_start (GTK_BOX (vbox), bbox, FALSE, FALSE, 2);
	gtk_widget_show (bbox);

	gtkutil_button (dcccwin.window, 0, _("Accept"), accept_chat_clicked, 0, bbox);
	gtkutil_button (dcccwin.window, 0, _("Abort"), abort_chat_clicked, 0, bbox);

	gtk_widget_show (dcccwin.window);
	fe_dcc_update_chat_win ();
}
