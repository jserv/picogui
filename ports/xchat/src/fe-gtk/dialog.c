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

#include <string.h>
#include <time.h>
#include "fe-gtk.h"
#include "../common/xchat.h"
#include "../common/fe.h"
#include "../common/text.h"
#include "../common/util.h"
#include "../common/outbound.h"
#include "../common/xchatc.h"
#include "gtkutil.h"
#include "menu.h"
#include "xtext.h"
#include "wins.h"
#include "fkeys.h"
#include "maingui.h"
#include "palette.h"
#include "dialog.h"

#ifdef USE_GNOME
extern GtkTargetEntry dnd_targets[];
#endif


#ifdef USE_GNOME
static void
dialog_dnd_drop (GtkWidget * widget, GdkDragContext * context, gint x,
					  gint y, GtkSelectionData * selection_data, guint info,
					  guint32 time, struct session *sess)
{
	GList *list;
	char *file, tbuf[400];

	if (!sess->channel[0])
		return;

	list = gnome_uri_list_extract_filenames (selection_data->data);
	while (list)
	{
		file = (char *) (list->data);
		dcc_send (sess, tbuf, sess->channel, file);
		list = list->next;
	}
	gnome_uri_list_free_strings (list);
}
#endif

void
fe_change_nick (struct server *serv, char *nick, char *newnick)
{
	struct session *sess = find_dialog (serv, nick);
	if (sess)
	{
		safe_strcpy (sess->channel, newnick, CHANLEN);
		fe_set_title (sess);
	}
}

static void
dialog_button_cb (GtkWidget *wid, char *cmd)
{
	/* the longest cmd is 12, and the longest nickname is 64 */
	char buf[128];

	if (!menu_sess)
		return;

	auto_insert (buf, cmd, 0, 0, "", "", "", "", "", menu_sess->channel);

	handle_command (buf, menu_sess, FALSE, FALSE);
}

static void
dialog_button (GtkWidget *box, char *label, char *cmd)
{
	GtkWidget *wid;

	wid = gtk_button_new_with_label (label);
	gtk_box_pack_start (GTK_BOX (box), wid, FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (wid), "clicked",
							  GTK_SIGNAL_FUNC (dialog_button_cb), cmd);
	if (prefs.inputgad_superfocus)
		show_and_unfocus (wid);
	else
		gtk_widget_show (wid);
}

static void
dialog_createbuttons (session * sess)
{
	struct popup *pop;
	GSList *list = dlgbutton_list;

	sess->gui->button_box = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (sess->gui->tbox), sess->gui->button_box,
							FALSE, TRUE, 2);
	gtk_widget_show (sess->gui->button_box);

	while (list)
	{
		pop = (struct popup *) list->data;
		if (pop->cmd[0])
			dialog_button (sess->gui->button_box, pop->name, pop->cmd);
		list = list->next;
	}
}

static void
open_dialog_window (struct session *sess)
{
	GtkWidget *hbox, *vbox, *bbox;
	int page = prefs.privmsgtab;
	struct User *user;

	if (!main_window)
		page = 0;
	if (!page)
	{
		sess->gui->window = wins_new (sess->channel, "",
												kill_session_callback, sess, link_cb,
												sess, FALSE, FALSE, NULL);
		gtk_widget_set_usize (sess->gui->window, prefs.dialog_width,
									 prefs.dialog_height);
		gtk_signal_connect ((GtkObject *) sess->gui->window, "focus_in_event",
								  GTK_SIGNAL_FUNC (focus_in), sess);
		sess->is_tab = FALSE;
	} else
	{
		sess->gui->window = wins_new (sess->channel, "",
												kill_session_callback, sess, link_cb,
												sess, TRUE, FALSE, NULL);
		sess->is_tab = TRUE;
	}

	sess->gui->vbox = vbox = wins_get_vbox (sess->gui->window);

	if (!page)
		sess->gui->changad = NULL;
	else
		sess->gui->changad = wins_get_notebooklabel (sess->gui->window);

	sess->gui->tbox = hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 2);
	if (prefs.topicbar)
		gtk_widget_show (hbox);

	/* create the X ^ < > buttons */
	wins_create_linkbuttons (sess->gui->window, hbox);

	sess->gui->topicgad = gtk_entry_new ();
	gtk_entry_set_editable ((GtkEntry *) sess->gui->topicgad, FALSE);
	gtk_container_add (GTK_CONTAINER (hbox), sess->gui->topicgad);
	if (prefs.inputgad_superfocus)
		show_and_unfocus (sess->gui->topicgad);
	else
		gtk_widget_show (sess->gui->topicgad);

	/*if (prefs.dialogbuttons)*/
		dialog_createbuttons (sess);
	/*else
		sess->gui->button_box = 0;*/

	hbox = gtk_hbox_new (FALSE, 0);
	sess->gui->leftpane = hbox;
	gtk_container_add (GTK_CONTAINER (vbox), hbox);
	gtk_widget_show (hbox);

	sess->gui->textgad =
		gtk_xtext_new (prefs.dialog_indent_pixels * prefs.dialog_indent_nicks,
							prefs.dialog_show_separator);

	gtk_object_set_user_data (GTK_OBJECT (sess->gui->textgad), sess);

	((GtkXText *) sess->gui->textgad)->wordwrap = prefs.dialog_wordwrap;
	((GtkXText *) sess->gui->textgad)->max_auto_indent = prefs.max_auto_indent;
	((GtkXText *) sess->gui->textgad)->auto_indent = prefs.auto_indent;
	((GtkXText *) sess->gui->textgad)->thinline = prefs.thin_separator;
	((GtkXText *) sess->gui->textgad)->max_lines = prefs.max_lines;
	((GtkXText *) sess->gui->textgad)->error_function = gtkutil_simpledialog;
	((GtkXText *) sess->gui->textgad)->urlcheck_function = maingui_word_check;

	((GtkXText *) sess->gui->textgad)->tint_red = prefs.dialog_tint_red;
	((GtkXText *) sess->gui->textgad)->tint_green = prefs.dialog_tint_green;
	((GtkXText *) sess->gui->textgad)->tint_blue = prefs.dialog_tint_blue;

	if (prefs.timestamp && prefs.dialog_indent_nicks)
		((GtkXText *) sess->gui->textgad)->time_stamp = TRUE;

	gtk_xtext_set_palette (GTK_XTEXT (sess->gui->textgad), colors);
	gtk_xtext_set_font (GTK_XTEXT (sess->gui->textgad), dialog_font_normal, 0);
	gtk_xtext_set_background (GTK_XTEXT (sess->gui->textgad),
									  dialogwin_pix,
									  prefs.dialog_transparent, prefs.dialog_tint);

	gtk_container_add (GTK_CONTAINER (hbox), sess->gui->textgad);
	show_and_unfocus (sess->gui->textgad);

#ifdef USE_GNOME
	gtk_drag_dest_set (sess->gui->textgad, GTK_DEST_DEFAULT_ALL, dnd_targets,
							 1, GDK_ACTION_MOVE | GDK_ACTION_COPY | GDK_ACTION_LINK);
	gtk_signal_connect (GTK_OBJECT (sess->gui->textgad), "drag_data_received",
							  GTK_SIGNAL_FUNC (dialog_dnd_drop), sess);
#endif

	sess->gui->vscrollbar =
		gtk_vscrollbar_new (GTK_XTEXT (sess->gui->textgad)->adj);
	gtk_box_pack_start (GTK_BOX (hbox), sess->gui->vscrollbar, FALSE, FALSE,
							  1);
	show_and_unfocus (sess->gui->vscrollbar);

	if (!sess->is_tab)
		gtk_signal_connect_object (GTK_OBJECT (sess->gui->window),
											"configure_event",
											GTK_SIGNAL_FUNC (maingui_configure),
											GTK_OBJECT (sess));

	bbox = gtk_hbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (bbox), 0);
	gtk_box_pack_end (GTK_BOX (vbox), bbox, FALSE, TRUE, 2);
	gtk_widget_show (bbox);

	gtk_signal_connect (GTK_OBJECT (sess->gui->textgad), "word_click",
							  GTK_SIGNAL_FUNC (maingui_word_clicked), sess);

	sess->gui->inputgad = gtk_entry_new_with_max_length (2048);
	gtk_container_add (GTK_CONTAINER (bbox), sess->gui->inputgad);
	gtk_signal_connect (GTK_OBJECT (sess->gui->inputgad), "key_press_event",
							  GTK_SIGNAL_FUNC (key_handle_key_press), sess);
	if (prefs.style_inputbox)
		gtk_widget_set_style (sess->gui->inputgad, inputgad_style);
	gtk_widget_show (sess->gui->inputgad);

	/* create the < ^ buttons on the bottom right (maingui.c) */
	gui_create_toolbox (sess, bbox);

	gtk_widget_show (sess->gui->window);

	if (page && prefs.newtabstofront)
		wins_bring_tofront (sess->gui->window);

	fe_set_title (sess);

	user = find_name_global (sess->server, sess->channel);
	if (user)
	{
		if (user->hostname)
			gtk_entry_set_text (GTK_ENTRY (sess->gui->topicgad), user->hostname);
	}
}

void
new_dialog (session *sess)
{
	if (prefs.logging)
		setup_logging (sess);

	open_dialog_window (sess);
	sess->new_data = FALSE;
}

void
fe_dlgbuttons_update (struct session *sess)
{
	if (sess->type == SESS_DIALOG)
	{
		if (sess->gui->button_box)
		{
			gtk_widget_destroy (sess->gui->button_box);
			sess->gui->button_box = 0;
		}
		/*if (prefs.dialogbuttons)*/
			dialog_createbuttons (sess);
	}
}
