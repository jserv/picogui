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
#include <fcntl.h>
#include <string.h>

#include "fe-gtk.h"
#include "../common/xchat.h"
#include "../common/cfgfiles.h"
#include "../common/fe.h"
#include "../common/util.h"
#include "../common/outbound.h"
#include "../common/notify.h"
#include "../common/xchatc.h"
#include "settings.h"
#include "gtkutil.h"
#include "xtext.h"
#include "palette.h"
#include "maingui.h"
#include "menu.h"
#include "pixmaps.h"


struct setup
{
	GtkWidget *settings_window;
	GtkWidget *nu_color;
	GtkWidget *bt_color;

	GtkWidget *font_normal;
	GtkWidget *font_bold;

	GtkWidget *dialog_font_normal;
	GtkWidget *dialog_font_bold;

	GtkWidget *entry_stamp_format;
	GtkWidget *entry_bluestring;
	GtkWidget *entry_doubleclickuser;
	GtkWidget *entry_max_lines;
	GtkWidget *entry_nick_suffix;
	GtkWidget *entry_quit;
	GtkWidget *entry_part;
	GtkWidget *entry_away;
	GtkWidget *entry_timeout;
	GtkWidget *entry_dccdir;
	GtkWidget *entry_dcctimeout;
	GtkWidget *entry_dccstalltimeout;
	GtkWidget *entry_permissions;
	GtkWidget *entry_dcc_blocksize;
	GtkWidget *entry_dnsprogram;
	GtkWidget *entry_recon_delay;

	GtkWidget *entry_proxy_host;
	GtkWidget *entry_proxy_port;

	GtkWidget *entry_dcc_send_port_first;
	GtkWidget *entry_dcc_send_port_last;

	GtkWidget *entry_sounddir;
	GtkWidget *entry_soundcmd;

	GtkWidget *entry_mainw_left;
	GtkWidget *entry_mainw_top;
	GtkWidget *entry_mainw_width;
	GtkWidget *entry_mainw_height;

	GtkWidget *entry_hostname;
	GtkWidget *entry_dcc_ip_str;

	GtkWidget *background;
	GtkWidget *background_dialog;

	GtkWidget *check_transparent;
	GtkWidget *check_tint;
	GtkWidget *check_detecthost;
	GtkWidget *check_detectip;
	GtkWidget *check_ip;

	GtkWidget *entry_trans_file;

	GtkWidget *dialog_check_transparent;
	GtkWidget *dialog_check_tint;

	GtkWidget *cancel_button;

	GtkWidget *logmask_entry;
	GtkWidget *logtimestamp_entry;

	struct xchatprefs prefs;
};


static GtkWidget *fontdialog = 0;

extern GtkStyle *create_inputgad_style (void);


/* Generic interface creation functions */

static void
settings_create_optmenu (GtkWidget *table, char *label, char **text,
								void *callback, void *userdata, int row_index,
								int history)
{
	GtkWidget *wid, *menu, *align;
	int i;

	align = gtk_alignment_new (1.0, 0.5, 0.0, 0.0);
	gtk_table_attach (GTK_TABLE (table), align, 0, 1, row_index, row_index + 1,
							GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
	gtk_widget_show (align);

	wid = gtk_label_new (label);
	gtk_container_add (GTK_CONTAINER (align), wid);
	gtk_widget_show (wid);

	wid = gtk_option_menu_new ();
	menu = gtk_menu_new ();

	i = 0;
	while (text[i])
	{
		menu_quick_item_with_callback (callback, text[i], menu, (void *) i);
		i++;
	}

	gtk_object_set_user_data (GTK_OBJECT (menu), userdata);
	gtk_option_menu_set_menu (GTK_OPTION_MENU (wid), menu);
	gtk_option_menu_set_history (GTK_OPTION_MENU (wid), history);
	gtk_widget_show (menu);
	align = gtk_alignment_new (0.0, 1.0, 0.0, 0.0);
	gtk_table_attach_defaults (GTK_TABLE (table), align, 2, 3, row_index,
										row_index + 1);
	gtk_widget_show (align);
	gtk_container_add (GTK_CONTAINER (align), wid);
	gtk_widget_show (wid);
}

static void
settings_create_tworows (GtkWidget *box, GtkWidget **left_box, GtkWidget **right_box)
{
	GtkWidget *ac_box;

	ac_box = gtk_hbox_new (0, 0);
	gtk_container_add (GTK_CONTAINER (box), ac_box);
	gtk_widget_show (ac_box);

	*left_box = gtk_vbox_new (0, 0);
	gtk_container_add (GTK_CONTAINER (ac_box), *left_box);
	gtk_widget_show (*left_box);

	*right_box = gtk_vbox_new (0, 0);
	gtk_container_add (GTK_CONTAINER (ac_box), *right_box);
	gtk_widget_show (*right_box);
}

static GtkWidget *
settings_create_group (GtkWidget * vvbox, gchar * title)
{
	GtkWidget *frame;
	GtkWidget *vbox;

	frame = gtk_frame_new (title);
	gtk_box_pack_start (GTK_BOX (vvbox), frame, FALSE, FALSE, 0);
	gtk_widget_show (frame);

	vbox = gtk_vbox_new (FALSE, 2);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
	gtk_container_add (GTK_CONTAINER (frame), vbox);
	gtk_widget_show (vbox);

	return vbox;
}

static GtkWidget *
settings_create_table (GtkWidget * vvbox)
{
	GtkWidget *table;

	table = gtk_table_new (3, 2, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (table), 2);
	gtk_table_set_row_spacings (GTK_TABLE (table), 2);
	gtk_table_set_col_spacings (GTK_TABLE (table), 4);
	gtk_box_pack_start (GTK_BOX (vvbox), table, FALSE, FALSE, 0);
	gtk_widget_show (table);

	return table;
}

static GtkWidget *
settings_create_entry (char *text, int maxlen, GtkWidget * table,
							  char *defval, GtkWidget ** entry, char *suffix,
							  void *suffixcallback, int row)
{
	GtkWidget *label;
	GtkWidget *align;
	GtkWidget *hbox;
	GtkWidget *wid;

	label = gtk_label_new (text);
	gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_RIGHT);
	gtk_table_attach (GTK_TABLE (table), GTK_WIDGET (label), 0, 1, row,
							row + 1, GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0,
							0);
	gtk_widget_show (label);

	wid = gtk_entry_new_with_max_length (maxlen);
	if (entry)
		*entry = wid;
	gtk_entry_set_text (GTK_ENTRY (wid), defval);
	gtk_widget_show (wid);

	align = gtk_alignment_new (0.0, 1.0, 0.0, 0.0);
	gtk_table_attach_defaults (GTK_TABLE (table), align, 2, 3, row, row + 1);
	gtk_widget_show (align);

	hbox = gtk_hbox_new (FALSE, 2);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);
	gtk_table_attach (GTK_TABLE (table), GTK_WIDGET (wid), 2, 3, row,
							row + 1, GTK_FILL, GTK_FILL, 0, 0);
	gtk_widget_show (hbox);

	if (suffixcallback || suffix)
	{
		GtkWidget *swid;

		if (suffixcallback)
		{
			swid = gtk_button_new_with_label (_("Browse..."));
			gtk_signal_connect (GTK_OBJECT (swid), "clicked",
									  GTK_SIGNAL_FUNC (suffixcallback), *entry);
		} else
		{
			swid = gtk_label_new (suffix);
		}
		gtk_widget_show (swid);
		gtk_table_attach (GTK_TABLE (table), swid, 3, 4, row,
							row + 1, GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
	}
	gtk_container_add (GTK_CONTAINER (align), hbox);
	return wid;
}

static GtkWidget *
settings_create_numberentry (char *text, int min, int max, int climb,
										GtkWidget *table, int defval, int row, char *suffix)
{
	GtkWidget *label, *wid, *rbox, *align;
	GtkAdjustment *adj;

	label = gtk_label_new (text);
	gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), GTK_WIDGET (label), 0, 1, row,
							row + 1, GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0,
							0);
	gtk_widget_show (label);

	align = gtk_alignment_new (0.0, 1.0, 0.0, 0.0);
	gtk_table_attach_defaults (GTK_TABLE (table), align, 2, 3, row, row + 1);
	gtk_widget_show (align);

	rbox = gtk_hbox_new (0, 0);
	gtk_container_add (GTK_CONTAINER (align), rbox);
	gtk_widget_show (rbox);

	wid = gtk_spin_button_new (0, climb, 0);
	adj = gtk_spin_button_get_adjustment ((GtkSpinButton*)wid);
	adj->lower = min;
	adj->upper = max;
	adj->step_increment = climb;
	gtk_adjustment_changed (adj);
	gtk_spin_button_set_value ((GtkSpinButton*)wid, defval);
	gtk_widget_set_usize (wid, 60, 0);
	gtk_box_pack_start (GTK_BOX (rbox), wid, 0, 0, 0);
	gtk_widget_show (wid);

	if (suffix)
	{
		label = gtk_label_new (suffix);
		gtk_box_pack_start (GTK_BOX (rbox), label, 0, 0, 0);
		gtk_widget_show (label);
	}

	return wid;
}

static GtkWidget *
settings_create_toggle (char *label, GtkWidget * box, int state,
								void *callback, void *sess)
{
	GtkWidget *wid;

	wid = gtk_check_button_new_with_label (label);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (wid), state);
	gtk_signal_connect (GTK_OBJECT (wid), "toggled",
							  GTK_SIGNAL_FUNC (callback), sess);
	if (box)
		gtk_box_pack_start (GTK_BOX (box), wid, 0, 0, 0);
	gtk_widget_show (wid);

	return wid;
}

static void
settings_create_color_box (char *text, GtkWidget * table, int defval,
									GtkWidget ** entry, void *callback,
									struct session *sess, int row)
{
	GtkWidget *label;
	GtkWidget *align;
	GtkWidget *hbox;
	GtkWidget *swid;
	GtkWidget *wid;
	GtkStyle *style;
	char buf[127];

	label = gtk_label_new (text);
	gtk_misc_set_alignment (GTK_MISC (label), 1, 0.5);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_RIGHT);
	gtk_table_attach (GTK_TABLE (table), GTK_WIDGET (label), 0, 1, row,
							row + 1, GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0,
							0);
	gtk_widget_show (label);

	sprintf (buf, "%d", defval);
	wid = gtk_entry_new ();
	gtk_entry_set_text (GTK_ENTRY (wid), buf);
	gtk_widget_set_usize (GTK_WIDGET (wid), 40, -1);
	gtk_signal_connect (GTK_OBJECT (wid), "activate",
							  GTK_SIGNAL_FUNC (callback), sess);
	gtk_widget_show (wid);

	align = gtk_alignment_new (0.0, 1.0, 0.0, 0.0);
	gtk_table_attach_defaults (GTK_TABLE (table), align, 2, 3, row, row + 1);
	gtk_widget_show (align);

	hbox = gtk_hbox_new (FALSE, 2);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);
	gtk_box_pack_start (GTK_BOX (hbox), wid, FALSE, FALSE, 0);
	gtk_widget_show (hbox);

	style = gtk_style_new ();
	style->bg[0] = colors[defval];

	swid = gtk_event_box_new ();
	if (entry)
		*entry = swid;
	gtk_widget_set_style (GTK_WIDGET (swid), style);
	gtk_style_unref (style);
	gtk_widget_set_usize (GTK_WIDGET (swid), 40, -1);
	gtk_widget_show (swid);
	gtk_box_pack_start (GTK_BOX (hbox), swid, FALSE, FALSE, 0);

	gtk_container_add (GTK_CONTAINER (align), hbox);
}

static GtkWidget *
settings_create_page (GtkWidget * book, gchar * book_label, GtkWidget * ctree,
							 gchar * tree_label, GtkCTreeNode * parent,
							 GtkCTreeNode ** node, gint page_index,
							 void (*draw_func) (struct session *, GtkWidget *),
							 struct session *sess)
{
	GtkWidget *frame;
	GtkWidget *label;
	GtkWidget *vvbox;
	GtkWidget *vbox;

	gchar *titles[1];

	vvbox = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vvbox);

	/* border for the label */
	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
	gtk_box_pack_start (GTK_BOX (vvbox), frame, FALSE, TRUE, 0);
	gtk_widget_show (frame);

	/* label */
	label = gtk_label_new (book_label);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_misc_set_padding (GTK_MISC (label), 2, 1);
	gtk_container_add (GTK_CONTAINER (frame), label);
	gtk_widget_show (label);

	/* vbox for the tab */
	vbox = gtk_vbox_new (FALSE, 2);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
	gtk_container_add (GTK_CONTAINER (vvbox), vbox);
	gtk_widget_show (vbox);

	/* label on the tree */
	titles[0] = tree_label;
	*node = gtk_ctree_insert_node (GTK_CTREE (ctree), parent, NULL, titles, 0,
											 NULL, NULL, NULL, NULL, FALSE, FALSE);
	gtk_ctree_node_set_row_data (GTK_CTREE (ctree), *node,
										  (gpointer) page_index);

	/* call the draw func if there is one */
	if (draw_func)
		draw_func (sess, vbox);

	/* append page and return */
	gtk_notebook_append_page (GTK_NOTEBOOK (book), vvbox, NULL);
	return vbox;
}


/* Util functions and main callbacks */

static int
settings_closegui (GtkWidget * wid, struct session *sess)
{
	if (sess->setup)
	{
		free (sess->setup);
		sess->setup = 0;
	}
	return 0;
}

static void
settings_filereq_done (GtkWidget * entry, void *data2, char *file)
{
	if (file)
	{
		if (file[0])
			gtk_entry_set_text (GTK_ENTRY (entry), file);
		free (file);
	}
}

static void
settings_openfiledialog (GtkWidget * button, GtkWidget * entry)
{
	gtkutil_file_req (_("Choose File"), settings_filereq_done, entry, 0,
							FALSE);
/*#ifdef USE_IMLIB
   gtkutil_file_req (_ ("Choose Picture"), settings_filereq_done, entry, 0, FALSE);
#else
   gtkutil_file_req (_ ("Choose XPM"), settings_filereq_done, entry, 0, FALSE);
#endif*/
}

static void
settings_fontok (GtkWidget * ok_button, GtkWidget * entry)
{
	gchar *fontname;

	if (GTK_IS_WIDGET (entry))
	{
		fontname =
			gtk_font_selection_dialog_get_font_name (GTK_FONT_SELECTION_DIALOG
																  (fontdialog));
		if (fontname && fontname[0])
			gtk_entry_set_text (GTK_ENTRY (entry), fontname);
	}
	gtk_widget_destroy (fontdialog);
}

static void
settings_fontdialogdestroy (GtkWidget * dialog, gpointer unused)
{
	fontdialog = NULL;
}

static void
settings_openfontdialog (GtkWidget * button, GtkWidget * entry)
{
	GtkFontSelectionDialog *dialog;

	if (fontdialog)
	{
		gdk_window_show (GTK_WIDGET (fontdialog)->window);
		return;
	}

	dialog = (GtkFontSelectionDialog *)
				gtk_font_selection_dialog_new (_("Select Font"));
	gtk_signal_connect (GTK_OBJECT (dialog->ok_button), "clicked",
							  GTK_SIGNAL_FUNC (settings_fontok), entry);
	gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
							  GTK_SIGNAL_FUNC (settings_fontdialogdestroy), 0);
	gtk_signal_connect (GTK_OBJECT (dialog->cancel_button), "clicked",
							  GTK_SIGNAL_FUNC (gtkutil_destroy), dialog);
	gtk_font_selection_dialog_set_font_name (dialog,
							  gtk_entry_get_text (GTK_ENTRY (entry)));
	gtk_widget_show (GTK_WIDGET (dialog));

	fontdialog = (GtkWidget *)dialog;
}

static void
expand_homedir_inplace (char *file)
{
	char *new_file = expand_homedir (file);
	strcpy (file, new_file);
	free (new_file);
}

#define GETINT(x) gtk_spin_button_get_value_as_int((GtkSpinButton*)sess->setup->x)
#define GETSTR(x,y) strcpy(x,gtk_entry_get_text((GtkEntry*)sess->setup->y))

static void
settings_ok_clicked (GtkWidget * wid, struct session *sess)
{
	int noapply = FALSE;
	int fontchange = FALSE;
	int fontdialogchange = FALSE;
	struct session *s;
	GdkPixmap *old_pix;
	GtkStyle *new_style;
	GSList *list;

	/* get all the spinbutton and GtkEntry values into our structure */

	GETSTR (sess->setup->prefs.dcc_ip_str, entry_dcc_ip_str);
	GETSTR (sess->setup->prefs.hostname, entry_hostname);
	GETSTR (sess->setup->prefs.proxy_host, entry_proxy_host);
	sess->setup->prefs.proxy_port = GETINT (entry_proxy_port);

	sess->setup->prefs.first_dcc_send_port = GETINT (entry_dcc_send_port_first);
	sess->setup->prefs.last_dcc_send_port = GETINT (entry_dcc_send_port_last);

	GETSTR (sess->setup->prefs.background, background);
	expand_homedir_inplace (sess->setup->prefs.background);
	GETSTR (sess->setup->prefs.background_dialog, background_dialog);
	expand_homedir_inplace (sess->setup->prefs.background_dialog);
	GETSTR (sess->setup->prefs.soundcmd, entry_soundcmd);
	GETSTR (sess->setup->prefs.sounddir, entry_sounddir);

	GETSTR (sess->setup->prefs.dnsprogram, entry_dnsprogram);
	GETSTR (sess->setup->prefs.bluestring, entry_bluestring);
	GETSTR (sess->setup->prefs.doubleclickuser, entry_doubleclickuser);
	GETSTR (sess->setup->prefs.awayreason, entry_away);
	GETSTR (sess->setup->prefs.quitreason, entry_quit);
	GETSTR (sess->setup->prefs.partreason, entry_part);
	GETSTR (sess->setup->prefs.logmask, logmask_entry);
	GETSTR (sess->setup->prefs.timestamp_log_format, logtimestamp_entry);
	GETSTR (sess->setup->prefs.nick_suffix, entry_nick_suffix);
	GETSTR (sess->setup->prefs.stamp_format, entry_stamp_format);

	sess->setup->prefs.max_lines = GETINT (entry_max_lines);
	sess->setup->prefs.notify_timeout = GETINT (entry_timeout);
	sess->setup->prefs.recon_delay = GETINT (entry_recon_delay);

	sscanf (gtk_entry_get_text ((GtkEntry *) sess->setup->entry_permissions),
			  "%o", &sess->setup->prefs.dccpermissions);

	sess->setup->prefs.dcc_blocksize = GETINT (entry_dcc_blocksize);
	sess->setup->prefs.dccstalltimeout = GETINT (entry_dccstalltimeout);
	sess->setup->prefs.dcctimeout = GETINT (entry_dcctimeout);
	GETSTR (sess->setup->prefs.dccdir, entry_dccdir);
	expand_homedir_inplace (sess->setup->prefs.dccdir);

	GETSTR (sess->setup->prefs.font_normal, font_normal);
	GETSTR (sess->setup->prefs.dialog_font_normal, dialog_font_normal);

	sess->setup->prefs.mainwindow_left = GETINT (entry_mainw_left);
	sess->setup->prefs.mainwindow_top = GETINT (entry_mainw_top);
	sess->setup->prefs.mainwindow_width = GETINT (entry_mainw_width);
	sess->setup->prefs.mainwindow_height = GETINT (entry_mainw_height);

#ifdef USE_TRANS
	GETSTR (sess->setup->prefs.trans_file, entry_trans_file);
	if (sess->setup->prefs.use_trans)
	{
		if (load_trans_table (sess->setup->prefs.trans_file) == 0)
		{
			gtkutil_simpledialog (_("Failed to load translation table."));
			sess->setup->prefs.use_trans = 0;
		}
	}
#endif

	if (sess->setup->prefs.notify_timeout != prefs.notify_timeout)
	{
		if (notify_tag)
		{
			fe_timeout_remove (notify_tag);
			notify_tag = 0;
		}
		if (sess->setup->prefs.notify_timeout)
			notify_tag =
				fe_timeout_add (sess->setup->prefs.notify_timeout * 1000,
									 notify_checklist, 0);
	}

	if (prefs.use_fontset != sess->setup->prefs.use_fontset)
	{
		/* force font reloading */
		prefs.font_normal[0] = 0;
		prefs.dialog_font_normal[0] = 0;
		prefs.use_fontset = sess->setup->prefs.use_fontset;
	}

	if (strcmp (prefs.font_normal, sess->setup->prefs.font_normal) != 0)
	{
		gdk_font_unref (font_normal);
		font_normal = my_font_load (sess->setup->prefs.font_normal);
		fontchange = TRUE;
	}
	if (strcmp
		 (prefs.dialog_font_normal, sess->setup->prefs.dialog_font_normal) != 0)
	{
		gdk_font_unref (dialog_font_normal);
		dialog_font_normal =
			my_font_load (sess->setup->prefs.dialog_font_normal);
		fontdialogchange = TRUE;
	}

	if (prefs.tabchannels != sess->setup->prefs.tabchannels)
		noapply = TRUE;

	if (prefs.paned_userlist != sess->setup->prefs.paned_userlist)
		noapply = TRUE;

	if (prefs.chanmodebuttons != sess->setup->prefs.chanmodebuttons)
		noapply = TRUE;

	if (prefs.treeview != sess->setup->prefs.treeview)
		noapply = TRUE;

	if (prefs.nu_color != sess->setup->prefs.nu_color)
		noapply = TRUE;

	if (prefs.panel_vbox != sess->setup->prefs.panel_vbox)
		noapply = TRUE;

	if (prefs.auto_indent != sess->setup->prefs.auto_indent)
		noapply = TRUE;

	memcpy (&prefs, &sess->setup->prefs, sizeof (struct xchatprefs));

	if (main_window)
		maingui_set_tab_pos (prefs.tabs_position);

	old_pix = channelwin_pix;
	list = sess_list;
	channelwin_pix = pixmap_load_from_file (prefs.background);
	while (list)
	{
		s = (struct session *) list->data;
		if (s->type == SESS_DIALOG)
		{
			fe_dlgbuttons_update (s);
		}
		else if (s->type != SESS_SHELL)
		{
			GTK_XTEXT (s->gui->textgad)->tint_red = prefs.tint_red;
			GTK_XTEXT (s->gui->textgad)->tint_green = prefs.tint_green;
			GTK_XTEXT (s->gui->textgad)->tint_blue = prefs.tint_blue;

			gtk_xtext_set_background (GTK_XTEXT (s->gui->textgad),
											  channelwin_pix,
											  prefs.transparent, prefs.tint);

			if (!prefs.indent_nicks)
				GTK_XTEXT (s->gui->textgad)->indent = 0;
			else if (GTK_XTEXT (s->gui->textgad)->indent == 0)
				GTK_XTEXT (s->gui->textgad)->indent =
					prefs.indent_pixels * prefs.indent_nicks;

			GTK_XTEXT (s->gui->textgad)->wordwrap = prefs.wordwrap;
			GTK_XTEXT (s->gui->textgad)->max_lines = prefs.max_lines;
			GTK_XTEXT (s->gui->textgad)->separator = prefs.show_separator;

			if (fontchange)
				gtk_xtext_set_font (GTK_XTEXT (s->gui->textgad), font_normal, 0);

			if (prefs.timestamp && prefs.indent_nicks)
				GTK_XTEXT (s->gui->textgad)->time_stamp = TRUE;
			else
				GTK_XTEXT (s->gui->textgad)->time_stamp = FALSE;

			gtk_xtext_refresh (GTK_XTEXT (s->gui->textgad), 0);

			fe_buttons_update (s);
		}
		list = list->next;
	}
	if (old_pix)
		gdk_pixmap_unref (old_pix);

	old_pix = dialogwin_pix;
	list = sess_list;
	dialogwin_pix = pixmap_load_from_file (prefs.background_dialog);
	while (list)
	{
		s = (struct session *) list->data;
		if (s->type == SESS_DIALOG)
		{
			GTK_XTEXT (s->gui->textgad)->tint_red = prefs.dialog_tint_red;
			GTK_XTEXT (s->gui->textgad)->tint_green = prefs.dialog_tint_green;
			GTK_XTEXT (s->gui->textgad)->tint_blue = prefs.dialog_tint_blue;

			gtk_xtext_set_background (GTK_XTEXT (s->gui->textgad),
											  dialogwin_pix,
											  prefs.dialog_transparent,
											  prefs.dialog_tint);

			if (!prefs.dialog_indent_nicks)
				GTK_XTEXT (s->gui->textgad)->indent = 0;
			else if (GTK_XTEXT (s->gui->textgad)->indent == 0)
				GTK_XTEXT (s->gui->textgad)->indent =
					prefs.dialog_indent_pixels * prefs.dialog_indent_nicks;

			GTK_XTEXT (s->gui->textgad)->wordwrap = prefs.dialog_wordwrap;
			GTK_XTEXT (s->gui->textgad)->max_lines = prefs.max_lines;
			GTK_XTEXT (s->gui->textgad)->separator = prefs.dialog_show_separator;

			if (fontdialogchange)
				gtk_xtext_set_font (GTK_XTEXT (s->gui->textgad), dialog_font_normal,
									  0);

			if (prefs.timestamp && prefs.dialog_indent_nicks)
				GTK_XTEXT (s->gui->textgad)->time_stamp = TRUE;
			else
				GTK_XTEXT (s->gui->textgad)->time_stamp = FALSE;

			gtk_xtext_refresh (GTK_XTEXT (s->gui->textgad), 0);
		}
		list = list->next;
	}
	if (old_pix)
		gdk_pixmap_unref (old_pix);

	/* update the font in inputgad_style */
	if (fontchange && prefs.style_inputbox)
	{
		new_style = create_inputgad_style ();
		list = sess_list;
		while (list)
		{
			s = list->data;
			switch (s->type)
			{
			case SESS_CHANNEL:
			case SESS_DIALOG:
			case SESS_SERVER:
				gtk_widget_set_style (s->gui->inputgad, new_style);
			}
			list = list->next;
		}
		gtk_style_unref (inputgad_style);
		inputgad_style = new_style;
	}

	if (wid)
		gtk_widget_destroy (sess->setup->settings_window);
	else
		gtk_widget_set_sensitive (sess->setup->cancel_button, FALSE);

	if (noapply)
		gtkutil_simpledialog (_("The following prefs do not take effect\n"
									 "immediately, you will have to close the\n"
									 "window and re-open it:\n\n"
									 " - Channel Tabs\n"
									 " - Channel Mode Buttons\n"
									 " - Userlist Buttons\n"
									 " - Disable Paned Userlist\n"
									 " - Notify User color\n"
									 " - Layout for a vertical panel\n"
									 " - Auto Indent"));
}

static void
settings_apply_clicked (GtkWidget * wid, struct session *sess)
{
	settings_ok_clicked (0, sess);
}

static void
settings_color_clicked (GtkWidget * igad, GtkWidget * colgad, int *colset)
{
	GtkStyle *stylefg = gtk_style_new ();
	char buf[16];
	int col = atoi (gtk_entry_get_text (GTK_ENTRY (igad)));
	if (col < 0 || col > 15)
		col = 0;
	sprintf (buf, "%d", col);
	gtk_entry_set_text (GTK_ENTRY (igad), buf);
	stylefg->bg[0] = colors[col];
	gtk_widget_set_style (colgad, stylefg);
	gtk_style_unref (stylefg);
	*colset = col;
}

static void
settings_ctree_select (GtkWidget * ctree, GtkCTreeNode * node)
{
	GtkWidget *book;
	gint page;

	if (!GTK_CLIST (ctree)->selection)
		return;

	book = GTK_WIDGET (gtk_object_get_user_data (GTK_OBJECT (ctree)));
	page = (gint) gtk_ctree_node_get_row_data (GTK_CTREE (ctree), node);

	gtk_notebook_set_page (GTK_NOTEBOOK (book), page);
}

static void
settings_ptoggle_check (GtkWidget * widget, int *pref)
{
	*pref = (int) (GTK_TOGGLE_BUTTON (widget)->active);
}

/*static void
settings_pinvtoggle_check (GtkWidget * widget, int *pref)
{
	*pref = ((int) (GTK_TOGGLE_BUTTON (widget)->active) ? 0 : 1);
}*/

/* Misc callbacks */

static void
settings_transparent_check (GtkWidget * widget, struct session *sess)
{
	if (GTK_TOGGLE_BUTTON (widget)->active)
	{
		sess->setup->prefs.transparent = TRUE;
		gtk_widget_set_sensitive (sess->setup->background, FALSE);
#if defined(USE_GDK_PIXBUF) || defined(USE_MMX)
		gtk_widget_set_sensitive (sess->setup->check_tint, TRUE);
#endif
	} else
	{
		sess->setup->prefs.transparent = FALSE;
		gtk_toggle_button_set_active ((GtkToggleButton *) sess->setup->
												check_tint, FALSE);
		gtk_widget_set_sensitive (sess->setup->background, TRUE);
		gtk_widget_set_sensitive (sess->setup->check_tint, FALSE);
	}
}

static void
settings_transparent_dialog_check (GtkWidget * widget, struct session *sess)
{
	if (GTK_TOGGLE_BUTTON (widget)->active)
	{
		sess->setup->prefs.dialog_transparent = TRUE;
		gtk_widget_set_sensitive (sess->setup->background_dialog, FALSE);
#if defined(USE_GDK_PIXBUF) || defined(USE_MMX)
		gtk_widget_set_sensitive (sess->setup->dialog_check_tint, TRUE);
#endif
	} else
	{
		sess->setup->prefs.dialog_transparent = FALSE;
		gtk_toggle_button_set_active ((GtkToggleButton *) sess->setup->
												dialog_check_tint, FALSE);
		gtk_widget_set_sensitive (sess->setup->background_dialog, TRUE);
		gtk_widget_set_sensitive (sess->setup->dialog_check_tint, FALSE);
	}
}

static void
settings_nu_color_clicked (GtkWidget * igad, struct session *sess)
{
	settings_color_clicked (igad, sess->setup->nu_color,
									&sess->setup->prefs.nu_color);
}

static void
settings_slider_cb (GtkAdjustment * adj, int *value)
{
	session *sess;
	GtkWidget *tog;

	*value = adj->value;

	tog = gtk_object_get_user_data (GTK_OBJECT (adj));
	sess = gtk_object_get_user_data (GTK_OBJECT (tog));
	if (GTK_TOGGLE_BUTTON (tog)->active)
	{
		GTK_XTEXT (sess->gui->textgad)->tint_red = sess->setup->prefs.tint_red;
		GTK_XTEXT (sess->gui->textgad)->tint_green =
			sess->setup->prefs.tint_green;
		GTK_XTEXT (sess->gui->textgad)->tint_blue =
			sess->setup->prefs.tint_blue;
		if (GTK_XTEXT (sess->gui->textgad)->transparent)
			gtk_xtext_refresh (GTK_XTEXT (sess->gui->textgad), 1);
	}
}

static void
settings_slider (GtkWidget * vbox, char *label, int *value, GtkWidget * tog)
{
	GtkAdjustment *adj;
	GtkWidget *wid, *hbox, *lbox, *lab;

	hbox = gtk_hbox_new (0, 0);
	gtk_container_add (GTK_CONTAINER (vbox), hbox);
	gtk_widget_show (hbox);

	lbox = gtk_hbox_new (0, 0);
	gtk_box_pack_start (GTK_BOX (hbox), lbox, 0, 0, 2);
	gtk_widget_set_usize (lbox, 60, 0);
	gtk_widget_show (lbox);

	lab = gtk_label_new (label);
	gtk_box_pack_end (GTK_BOX (lbox), lab, 0, 0, 0);
	gtk_widget_show (lab);

	adj = (GtkAdjustment *) gtk_adjustment_new (*value, 0, 255.0, 1, 25, 0);
	gtk_object_set_user_data (GTK_OBJECT (adj), tog);
	gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
							  GTK_SIGNAL_FUNC (settings_slider_cb), value);

	wid = gtk_hscale_new (adj);
	gtk_scale_set_value_pos ((GtkScale *) wid, GTK_POS_RIGHT);
	gtk_scale_set_digits ((GtkScale *) wid, 0);
	gtk_container_add (GTK_CONTAINER (hbox), wid);
	gtk_widget_show (wid);
}

/* Functions to help build a page type */

static void
settings_create_window_page (GtkWidget * vvbox,
									  GtkWidget ** font_normal_wid,
									  char *font_normal_str,
									  GtkWidget ** backgroundwid,
									  char *background,
									  int transparent,
									  void *transparent_callback,
									  int tint,
									  int *tint_pref,
									  int *separator,
									  int *indent_nicks,
									  int *wordwrap,
									  GtkWidget ** transwid,
									  GtkWidget ** tintwid,
									  int *red, int *green, int *blue,
									  int *autoindent, struct session *sess)
{
	GtkWidget *vbox, *table, *wid, *left_box, *right_box;
	gint row_index = 0;

	vbox = settings_create_group (vvbox, _("Appearance"));
	table = settings_create_table (vbox);
	settings_create_tworows (vbox, &left_box, &right_box);

	settings_create_entry (_("Font:"), FONTNAMELEN, table, font_normal_str,
								  font_normal_wid, 0, settings_openfontdialog,
								  row_index++);
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
#ifdef USE_GDK_PIXBUF
	settings_create_entry (_("Background Picture:"), PATH_MAX, table,
								  background, backgroundwid, 0,
								  settings_openfiledialog, row_index++);
#else
	settings_create_entry (_("Background XPM:"), PATH_MAX, table, background,
								  backgroundwid, 0, settings_openfiledialog,
								  row_index++);
#endif
	if (transparent)
		gtk_widget_set_sensitive (*backgroundwid, FALSE);

	wid = settings_create_toggle (_("Indent Nicks"), left_box, *indent_nicks,
											settings_ptoggle_check, indent_nicks);
	add_tip (wid, _("Make nicknames right justified."));

	wid = settings_create_toggle (_("Auto Indent"), left_box, *autoindent,
											settings_ptoggle_check, autoindent);
	add_tip (wid, _("Auto adjust the separator bar position as needed."));

	wid = settings_create_toggle (_("Draw Separator Bar"), left_box, *separator,
											settings_ptoggle_check, separator);
	add_tip (wid, _("Make the separator an actual visible line."));

	wid = settings_create_toggle (_("Word Wrap"), right_box, *wordwrap,
											settings_ptoggle_check, wordwrap);
	add_tip (wid, _("Don't split words from one line to the next"));

	*transwid = settings_create_toggle (_("Transparent Background"), right_box,
													transparent, transparent_callback,
													sess);
	add_tip (*transwid, _("Make the text box seem see-through"));

	*tintwid = settings_create_toggle (_("Tint Transparency"), right_box, tint,
												  settings_ptoggle_check, tint_pref);
	add_tip (*tintwid, _("Tint the see-through text box to make it darker"));
	settings_create_toggle (_("Use a font set"),
									left_box, prefs.use_fontset, settings_ptoggle_check,
									&(sess->setup->prefs.use_fontset));

#ifndef USE_XLIB
	gtk_widget_set_sensitive (*transwid, FALSE);
#endif

#if defined(USE_GDK_PIXBUF) || defined(USE_MMX)
	if (!transparent)
#endif
		gtk_widget_set_sensitive (*tintwid, FALSE);

	vbox = settings_create_group (vvbox, _("Tint Setting"));

#if !defined(USE_GDK_PIXBUF) && !defined(USE_MMX)
	gtk_widget_set_sensitive (vbox, FALSE);
#endif

	wid = gtk_check_button_new_with_label (_("Change in realtime"));
	gtk_object_set_user_data (GTK_OBJECT (wid), sess);

	settings_slider (vbox, _("Red:"), red, wid);
	settings_slider (vbox, _("Green:"), green, wid);
	settings_slider (vbox, _("Blue:"), blue, wid);

	gtk_container_add (GTK_CONTAINER (vbox), wid);
	gtk_widget_show (wid);
}

static void
settings_sortset (GtkWidget * item, int num)
{
	session *sess = gtk_object_get_user_data (GTK_OBJECT (item->parent));
	sess->setup->prefs.userlist_sort = num;
}

/* Functions for each "Page" */

static void
settings_page_interface (struct session *sess, GtkWidget * vbox)
{
	GtkWidget *wid;
	GtkWidget *tab;
	GtkWidget *tog;
	gint row_index;
	static char *ulmenutext[] = 
	{
		N_("A-Z, Ops first"),
		N_("A-Z"),
		N_("Z-A, Ops last"),
		N_("Z-A"),
		N_("Unsorted"),
		NULL
	};

	wid = settings_create_group (vbox, _("Startup and Shutdown"));
	tog = settings_create_toggle (_("No Server List On Startup"), wid,
											prefs.skipserverlist, settings_ptoggle_check,
											&(sess->setup->prefs.skipserverlist));
	add_tip (tog, _("Don't display the server list on X-Chat startup"));
	tog = settings_create_toggle (_("Auto Save URL list"), wid,
											prefs.autosave_url, settings_ptoggle_check,
											&(sess->setup->prefs.autosave_url));
	add_tip (tog, _("Auto save your URL list when exiting from X-Chat"));

	wid = settings_create_group (vbox, _("User List"));
	tog = settings_create_toggle (_ ("Give the User List style"), wid,
                                  prefs.style_namelistgad, settings_ptoggle_check,
                                  &(sess->setup->prefs.style_namelistgad));
	tog = settings_create_toggle (_("Show userhost"), wid,
											prefs.showhostname_in_userlist,
											settings_ptoggle_check,
											&(sess->setup->prefs.showhostname_in_userlist));
	add_tip (tog, _("Show userhost of users in channel userlist"));
	tog = settings_create_toggle (_("Enable Paned Userlist"), wid,
											prefs.paned_userlist, settings_ptoggle_check,
											&(sess->setup->prefs.paned_userlist));
	add_tip (tog, _("Use paned user list instead of a fixed width one"));
	tog = settings_create_toggle (_("Userlist icons"), wid,
											prefs.userlist_icons, settings_ptoggle_check,
											&(sess->setup->prefs.userlist_icons));
	tab = settings_create_table (wid);
	row_index = 0;
	settings_create_entry (_("Double Click Command:"),
								  sizeof (prefs.doubleclickuser) - 1, tab,
								  prefs.doubleclickuser,
								  &sess->setup->entry_doubleclickuser, 0, 0,
								  row_index++);
	settings_create_optmenu (tab, _("Userlist sorted by:"), ulmenutext,
								settings_sortset, sess, row_index, prefs.userlist_sort);
}

static void
settings_page_interface_inout (struct session *sess, GtkWidget * vbox)
{
	GtkWidget *wid;
	GtkWidget *tab;
	GtkWidget *tog;
	GtkWidget *entry;
	GtkWidget *left_box, *right_box;

	wid = settings_create_group (vbox, _("Input Box"));

	settings_create_tworows (wid, &left_box, &right_box);

	tog = settings_create_toggle (_("Nick Name Completion"), left_box,
											prefs.nickcompletion, settings_ptoggle_check,
											&(sess->setup->prefs.nickcompletion));
	add_tip (tog, _("Complete nicknames when a partial one is entered"));
	tog = settings_create_toggle (_("Old-style Nickname Completion"), left_box,
											prefs.old_nickcompletion,
											settings_ptoggle_check,
											&(sess->setup->prefs.old_nickcompletion));
	add_tip (tog,
				_("Nickname completion is old-style (instead of GNU-style)"));
	tog =
		settings_create_toggle (_("Give the Input Box style"), left_box,
										prefs.style_inputbox, settings_ptoggle_check,
										&(sess->setup->prefs.style_inputbox));
	add_tip (tog, _("Input box gets same style as main text area"));
	tog =
		settings_create_toggle (_("Input Box always in focus"), right_box,
										prefs.inputgad_superfocus,
										settings_ptoggle_check,
										&(sess->setup->prefs.inputgad_superfocus));
	tog =
		settings_create_toggle (_("Show nickname"), right_box, prefs.nickgad,
										settings_ptoggle_check,
										&(sess->setup->prefs.nickgad));
	add_tip (tog, _("Show nickname and op/voice icon before the input box"));

	tab = settings_create_table (wid);
	entry = settings_create_entry (_("Nickname Completion Character:"), 1, tab,
											 prefs.nick_suffix,
											 &sess->setup->entry_nick_suffix,
											 0, 0, 0);
	add_tip (entry, _("Character to append to completed nicknames"));

	wid = settings_create_group (vbox, _("Output Box"));
	settings_create_tworows (wid, &left_box, &right_box);
	tog = settings_create_toggle (_("Strip MIRC Color"), right_box,
											prefs.stripcolor, settings_ptoggle_check,
											&(sess->setup->prefs.stripcolor));
	add_tip (tog, _("Strip MIRC color codes from text before displaying"));
	tog = settings_create_toggle (_("Colored Nicks"), left_box,
											prefs.colorednicks, settings_ptoggle_check,
											&(sess->setup->prefs.colorednicks));
	add_tip (tog, _("Output nicknames in different colors"));
	tog = settings_create_toggle (_("Show invites in active window"), left_box,
											prefs.show_invite_in_front_session,
											settings_ptoggle_check,
											&(sess->setup->
											  prefs.show_invite_in_front_session));
	tog = settings_create_toggle (_("Time Stamp All Text"), left_box,
											prefs.timestamp, settings_ptoggle_check,
											&(sess->setup->prefs.timestamp));
	add_tip (tog, _("Prefix all text with the current time stamp"));
	tog = settings_create_toggle (_("Filter out BEEPs"), right_box,
											prefs.filterbeep, settings_ptoggle_check,
											&(sess->setup->prefs.filterbeep));
	add_tip (tog, _("Remove ^G BEEP codes from text before displaying"));

	tab = settings_create_table (wid);
	entry = settings_create_entry (_("Time Stamp Format:"), 63, tab,
											 prefs.stamp_format,
											 &sess->setup->entry_stamp_format,
											 0, 0, 0);

	wid = settings_create_group (vbox, _("Buffer Settings"));
	tab = settings_create_table (wid);
	sess->setup->entry_max_lines =
		settings_create_numberentry (_("Text Buffer Size:"), 0, 10000, 1, tab,
								prefs.max_lines, 0, _("lines (0=Unlimited)."));
#ifdef USE_JCODE
	wid = settings_create_group (vbox, _("Other Settings"));
    tog = settings_create_toggle (_("Kanji locale <=> JIS translation"),
                                  wid,
                                  prefs.kanji_conv, settings_ptoggle_check,
                                  &(sess->setup->prefs.kanji_conv));
        
#endif
}

static void
settings_tabsset (GtkWidget * item, int num)
{
	session *sess = gtk_object_get_user_data (GTK_OBJECT (item->parent));
	sess->setup->prefs.tabs_position = num;
}

static void
settings_lagmeterset (GtkWidget * item, int num)
{
	session *sess = gtk_object_get_user_data (GTK_OBJECT (item->parent));
	sess->setup->prefs.lagometer = num;
}

static void
settings_throttlemeterset (GtkWidget * item, int num)
{
	session *sess = gtk_object_get_user_data (GTK_OBJECT (item->parent));
	sess->setup->prefs.throttlemeter = num;
}

static void
settings_page_interface_layout (struct session *sess, GtkWidget * vbox)
{
	GtkWidget *wid;
	GtkWidget *tog, *tab;
	GtkWidget *left_box, *right_box;
	static char *tabmenutext[] = 
	{
		N_("Bottom"),
		N_("Top"),
		N_("Left"),
		N_("Right"),
		N_("Hidden"),
		NULL
	};
	static char *metermenutext[] = 
	{
		N_("Off"),
		N_("Progress bar"),
		N_("Info text"),
		N_("Both"),
		NULL
	};

	wid = settings_create_group (vbox, _("Buttons"));
	settings_create_tworows (wid, &left_box, &right_box);
	tog = settings_create_toggle (_("Channel Mode Buttons"), right_box,
											prefs.chanmodebuttons,
											settings_ptoggle_check,
											&(sess->setup->prefs.chanmodebuttons));
	add_tip (tog, _("Show the TNSIPMLK buttons"));
	tog = settings_create_toggle (_("User List Buttons"), left_box,
											prefs.userlistbuttons,
											settings_ptoggle_check,
											&(sess->setup->prefs.userlistbuttons));
	add_tip (tog, _("Show the buttons below the user list"));

	wid = settings_create_group (vbox, _("Meters"));
	settings_create_tworows (wid, &left_box, &right_box);
	tab = settings_create_table (left_box);
	settings_create_optmenu (tab, _("Lag meter:"), metermenutext,
								settings_lagmeterset, sess, 0, prefs.lagometer);
	tab = settings_create_table (right_box);
	settings_create_optmenu (tab, _("Throttle meter:"), metermenutext,
								settings_throttlemeterset, sess, 0, prefs.throttlemeter);


	wid = settings_create_group (vbox, _("Tabs"));
	tab = settings_create_table (wid);

	settings_create_optmenu (tab, _("Tabs Located at:"), tabmenutext,
								settings_tabsset, sess, 0, prefs.tabs_position);

	settings_create_tworows (wid, &left_box, &right_box);

	tog = settings_create_toggle (_("Channel Tabs"), right_box,
											prefs.tabchannels, settings_ptoggle_check,
											&(sess->setup->prefs.tabchannels));
	add_tip (tog, _("Use tabs for channels instead of separate windows"));
	tog = settings_create_toggle (_("Limited Tab Highlighting"), left_box,
											prefs.limitedtabhighlight, settings_ptoggle_check,
											&(sess->setup->prefs.limitedtabhighlight));
	add_tip (tog, _("Only highlight channel tabs for channel messages and actions"));
	tog = settings_create_toggle (_("New Tabs to front"), left_box,
											prefs.newtabstofront, settings_ptoggle_check,
											&(sess->setup->prefs.newtabstofront));
	add_tip (tog, _("Bring new query/channel tabs to front"));
	tog = settings_create_toggle (_("Private Message Tabs"), right_box,
											prefs.privmsgtab, settings_ptoggle_check,
											&(sess->setup->prefs.privmsgtab));
	add_tip (tog, _("Use tabs for /query instead of separate windows"));
	tog =
		settings_create_toggle (_
										("Use a separate tab/window for server messages"),
wid, prefs.use_server_tab, settings_ptoggle_check,
&(sess->setup->prefs.use_server_tab));
	tog = settings_create_toggle (_("Use a separate tab/window for notices and server notices"),
		wid, prefs.notices_tabs, settings_ptoggle_check, &(sess->setup->prefs.notices_tabs));
	tog =
		settings_create_toggle (_
										("Use tabs for DCC, Ignore, Notify etc windows."),
wid, prefs.windows_as_tabs, settings_ptoggle_check,
&(sess->setup->prefs.windows_as_tabs));
}

static void
settings_page_interface_mainwindow (struct session *sess, GtkWidget * vbox)
{
	GtkWidget *wid;
	GtkWidget *tab;
	gint row_index;

	wid = settings_create_group (vbox, _("Window Position"));
	gtkutil_label_new (_("If Left and Top are set to zero, X-Chat will use\n"
								"your window manager defaults."), wid);
	tab = settings_create_table (wid);
	row_index = 0;
	sess->setup->entry_mainw_left =
		settings_create_numberentry (_("Left:"), 0, 40000, 1, tab,
								prefs.mainwindow_left, row_index++, 0);
	sess->setup->entry_mainw_top =
		settings_create_numberentry (_("Top:"), 0, 40000, 1, tab,
								prefs.mainwindow_top, row_index++, 0);

	wid = settings_create_group (vbox, _("Window Size"));
	tab = settings_create_table (wid);
	row_index = 0;
	sess->setup->entry_mainw_width =
		settings_create_numberentry (_("Width:"), 106, 40000, 1, tab,
								prefs.mainwindow_width, row_index++, 0);
	sess->setup->entry_mainw_height =
		settings_create_numberentry (_("Height:"), 138, 40000, 1, tab,
								prefs.mainwindow_height, row_index++, 0);
	settings_create_toggle (_("Show Session Tree View"), wid, prefs.treeview,
									settings_ptoggle_check,
									&(sess->setup->prefs.treeview));
}

static void
settings_page_interface_channelwindow (struct session *sess, GtkWidget * vbox)
{
	settings_create_window_page (vbox,
										  &sess->setup->font_normal,
										  prefs.font_normal,
										  &sess->setup->background,
										  prefs.background,
										  prefs.transparent,
										  settings_transparent_check,
										  prefs.tint,
										  &(sess->setup->prefs.tint),
										  &(sess->setup->prefs.show_separator),
										  &(sess->setup->prefs.indent_nicks),
										  &sess->setup->prefs.wordwrap,
										  &sess->setup->check_transparent,
										  &sess->setup->check_tint,
										  &sess->setup->prefs.tint_red,
										  &sess->setup->prefs.tint_green,
										  &sess->setup->prefs.tint_blue,
										  &sess->setup->prefs.auto_indent, sess);
}

static void
settings_page_interface_dialogwindow (struct session *sess, GtkWidget * vbox)
{
	settings_create_window_page (vbox,
										  &sess->setup->dialog_font_normal,
										  prefs.dialog_font_normal,
										  &sess->setup->background_dialog,
										  prefs.background_dialog,
										  prefs.dialog_transparent,
										  settings_transparent_dialog_check,
										  prefs.dialog_tint,
										  &(sess->setup->prefs.dialog_tint),
										  &(sess->setup->prefs.dialog_show_separator),
										  &(sess->setup->prefs.dialog_indent_nicks),
										  &sess->setup->prefs.dialog_wordwrap,
										  &sess->setup->dialog_check_transparent,
										  &sess->setup->dialog_check_tint,
										  &sess->setup->prefs.dialog_tint_red,
										  &sess->setup->prefs.dialog_tint_green,
										  &sess->setup->prefs.dialog_tint_blue,
										  &sess->setup->prefs.auto_indent, sess);
}

#ifdef USE_PANEL
static void
settings_page_interface_panel (struct session *sess, GtkWidget * vbox)
{
	GtkWidget *wid;
	GtkWidget *tog;

	wid = settings_create_group (vbox, _("General"));
	tog = settings_create_toggle (_("Hide Session on Panelize"), wid,
											prefs.panelize_hide, settings_ptoggle_check,
											&(sess->setup->prefs.panelize_hide));
	add_tip (tog, _("Hide X-Chat when window moved to the panel"));

	wid = settings_create_group (vbox, _("Panel Applet"));
	tog = settings_create_toggle (_("Layout For a Vertical Panel"), wid,
											prefs.panel_vbox, settings_ptoggle_check,
											&(sess->setup->prefs.panel_vbox));
	add_tip (tog, _("Layout the X-Chat panel applet for a vertical panel"));
}
#endif

static void
settings_page_irc (struct session *sess, GtkWidget * vbox)
{
	GtkWidget *wid;
	GtkWidget *tab;
	GtkWidget *tog;
	GtkWidget *left_box, *right_box;
	gint row_index;

	wid = settings_create_group (vbox, _("General"));
	settings_create_tworows (wid, &left_box, &right_box);
	tog = settings_create_toggle (_("Raw Mode Display"), left_box,
											prefs.raw_modes, settings_ptoggle_check,
											&(sess->setup->prefs.raw_modes));
	add_tip (tog, _("Display raw mode changes instead of interpretations"));
	tog = settings_create_toggle (_("Beep on Private Messages"), left_box,
											prefs.beepmsg, settings_ptoggle_check,
											&(sess->setup->prefs.beepmsg));
	add_tip (tog, _("Beep when a private message for you is received"));
	tog = settings_create_toggle (_("Beep on Channel Messages"), left_box,
											prefs.beepchans, settings_ptoggle_check,
											&(sess->setup->prefs.beepchans));
	add_tip (tog, _("Beep when a channel message is received"));
	tog = settings_create_toggle (_("Send /who #chan on join."), right_box,
											prefs.userhost, settings_ptoggle_check,
											&(sess->setup->prefs.userhost));
	add_tip (tog, _("Find user information when joining a channel."));
	tog = settings_create_toggle (_("Perform a periodic mail check."), right_box,
											prefs.mail_check, settings_ptoggle_check,
											&(sess->setup->prefs.mail_check));
	wid = settings_create_group (vbox, _("Your irc settings"));
	tab = settings_create_table (wid);
	row_index = 0;

	settings_create_entry (_("Quit Message:"),
								  sizeof (prefs.quitreason) - 1, tab,
								  prefs.quitreason,
								  &sess->setup->entry_quit, 0, 0, row_index++);
	settings_create_entry (_("Part Message:"),
								  sizeof (prefs.partreason) - 1, tab,
								  prefs.partreason,
								  &sess->setup->entry_part, 0, 0, row_index++);
	gtkutil_label_new (_("These can be a filename relative to ~/.xchat/ to be\n"
								"used as a list of random quit or part reasons."), wid);

	wid = settings_create_group (vbox, _("External Programs"));
	tab = settings_create_table (wid);
	row_index = 0;
	settings_create_entry (_("DNS Lookup Program:"),
								  sizeof (prefs.dnsprogram) - 1, tab,
								  prefs.dnsprogram,
								  &sess->setup->entry_dnsprogram, 0, 0, row_index++);

	wid = settings_create_group (vbox, _("Timing"));
	tab = settings_create_table (wid);
	sess->setup->entry_recon_delay =
		settings_create_numberentry (_("Auto ReConnect Delay:"), 0, 10000, 1, tab,
								prefs.recon_delay, 0, _("seconds."));
}

static void
settings_page_irc_ipaddress (struct session *sess, GtkWidget * vbox)
{
	GtkWidget *wid;
	GtkWidget *tab;
	GtkWidget *tog;
	gint row_index;

	wid = settings_create_group (vbox, _("Address"));
	tab = settings_create_table (wid);
	row_index = 0;
	settings_create_entry (_("Hostname / IP Number:"),
								  sizeof (prefs.hostname) - 1, tab, prefs.hostname,
								  &sess->setup->entry_hostname, 0, 0, row_index++);

	gtkutil_label_new (_("Most people should leave this blank, it's only\n"
								"usefull for machines with multiple addresses."),
							 wid);

	wid = settings_create_group (vbox, _("Public IP Address"));
	tab = settings_create_table (wid);
	row_index = 0;

	settings_create_entry (_("DCC IP Address:"),
								  sizeof (prefs.hostname) - 1, tab, prefs.dcc_ip_str,
								  &sess->setup->entry_dcc_ip_str, 0, 0, row_index++);

	gtkutil_label_new (_("You may need to set the DCC IP Address if\n"
			     "you're behind a NAT router or proxy firewall."),
							 wid);

	tog = sess->setup->check_ip = settings_create_toggle
		(_("Get my IP from Server (for use in DCC Send only)"), wid,
		 prefs.ip_from_server, settings_ptoggle_check,
		 &(sess->setup->prefs.ip_from_server));
	add_tip (tog, _("For people using a 10.* or 192.168.* IP number."));
}

static void
settings_proxytypeset (GtkWidget * item, int num)
{
	session *sess = gtk_object_get_user_data (GTK_OBJECT (item->parent));
	sess->setup->prefs.proxy_type = num;
}

static void
settings_page_proxy (struct session *sess, GtkWidget * vbox)
{
	GtkWidget *wid;
	GtkWidget *tab;
	gint row_index;
	static char *proxymenutext[] =
	{
		N_("(Disabled)"),
		N_("Wingate"),
		N_("Socks4"),
		N_("Socks5"),
		N_("HTTP"),
		NULL
	};

	wid = settings_create_group (vbox, _("General"));
	tab = settings_create_table (wid);
	row_index = 0;
	settings_create_entry (_("Proxy Server Hostname:"),
								  sizeof (prefs.proxy_host) - 1, tab, prefs.proxy_host,
								  &sess->setup->entry_proxy_host, 0, 0, row_index++);
	sess->setup->entry_proxy_port =
		settings_create_numberentry (_("Proxy Server Port:"), 0, 65535, 1, tab,
								prefs.proxy_port, row_index++, 0);
	settings_create_optmenu (tab, _("Proxy Type:"), proxymenutext,
								settings_proxytypeset, sess, row_index, prefs.proxy_type);
}

static void
settings_page_irc_away (struct session *sess, GtkWidget * vbox)
{
	GtkWidget *wid;
	GtkWidget *tab;
	GtkWidget *tog;
	gint row_index;

	wid = settings_create_group (vbox, _("General"));
	tog = settings_create_toggle (_("Show away once"), wid,
											prefs.show_away_once, settings_ptoggle_check,
											&(sess->setup->prefs.show_away_once));
	add_tip (tog, _("Only show away messages the first time they're seen"));
	tog = settings_create_toggle (_("Announce away messsages"), wid,
											prefs.show_away_message,
											settings_ptoggle_check,
											&(sess->setup->prefs.show_away_message));
	add_tip (tog,
				_("Announce your away message to the channel(s) you are in"));

	wid = settings_create_group (vbox, _("Your away settings"));
	tab = settings_create_table (wid);
	row_index = 0;
	settings_create_entry (_("Away Reason:"),
								  sizeof (prefs.awayreason) - 1, tab,
								  prefs.awayreason,
								  &sess->setup->entry_away, 0, 0, row_index++);
}

static void
settings_page_irc_highlighting (struct session *sess, GtkWidget * vbox)
{
	GtkWidget *wid;
	GtkWidget *tab;
	gint row_index;

	wid = settings_create_group (vbox, _("General"));
	tab = settings_create_table (wid);
	row_index = 0;
	settings_create_entry (_("Words to Highlight:"),
								  sizeof (prefs.bluestring) - 1, tab,
								  prefs.bluestring,
								  &sess->setup->entry_bluestring,
								  _("(separate with commas)"), 0, row_index++);
}

static void
settings_page_irc_logging (struct session *sess, GtkWidget * vbox)
{
	GtkWidget *wid;
	GtkWidget *tog, *tab;

	gint row_index;

	wid = settings_create_group (vbox, _("General"));
	tab = settings_create_table (wid);
	tog = settings_create_toggle (_("Logging"), wid,
											prefs.logging, settings_ptoggle_check,
											&(sess->setup->prefs.logging));
	add_tip (tog, _("Enable logging conversations to disk"));
	tog = settings_create_toggle (_("Always timestamp logs"), wid,
											prefs.timestamp_logs, settings_ptoggle_check,
											&(sess->setup->prefs.timestamp_logs));

	add_tip (tog, _("Always apply timestamp to disk logs"));
	row_index = 0;
	settings_create_entry (_("Log name mask:"),
								  sizeof (prefs.logmask) - 1, tab,
								  prefs.logmask, &sess->setup->logmask_entry,
								  NULL, NULL, row_index++);
	settings_create_entry (_("Log timestamp format:"),
								63, tab,
								prefs.timestamp_log_format, &sess->setup->logtimestamp_entry,
								NULL, NULL, row_index++);
}

static void
settings_page_irc_notification (struct session *sess, GtkWidget * vbox)
{
	GtkWidget *wid;
	GtkWidget *tab;
	GtkWidget *tog;
	gint row_index;

	wid = settings_create_group (vbox, _("Generic"));
	tog = settings_create_toggle (_("Show notifies in active window"), wid,
											prefs.show_notify_in_front_session,
											settings_ptoggle_check,
											&(sess->setup->
											  prefs.show_notify_in_front_session));
	tog =
		settings_create_toggle (_("Send /whois"), wid,
										prefs.whois_on_notifyonline,
										settings_ptoggle_check,
										&(sess->setup->prefs.whois_on_notifyonline));

	wid = settings_create_group (vbox, _("User List Notify Highlighting"));
	tog = settings_create_toggle (_("Highlight Notifies"), wid,
											prefs.hilitenotify, settings_ptoggle_check,
											&(sess->setup->prefs.hilitenotify));
	add_tip (tog, _("Highlight notified users in the user list"));
	tab = settings_create_table (wid);
	row_index = 0;
	settings_create_color_box (_("Notified User Color:"), tab,
										prefs.nu_color,
										&sess->setup->nu_color,
										settings_nu_color_clicked, sess, row_index++);

	wid = settings_create_group (vbox, _("Notification Timeouts"));
	tab = settings_create_table (wid);
	sess->setup->entry_timeout =
		settings_create_numberentry (_("Notify Check Interval:"), 0, 960, 1, tab,
								prefs.notify_timeout, 0, _("seconds (0=Disable)."));
}

#ifdef USE_TRANS
static void
settings_page_irc_charset (struct session *sess, GtkWidget * vbox)
{
	GtkWidget *wid, *tog, *tab;

	wid = settings_create_group (vbox, _("General"));
	tog = settings_create_toggle (_("Enable Character Translation"), wid,
											prefs.use_trans, settings_ptoggle_check,
											&(sess->setup->prefs.use_trans));
	tab = settings_create_table (wid);

	settings_create_entry (_("Translation File:"),
								  sizeof (prefs.trans_file) - 1, tab,
								  prefs.trans_file,
								  &sess->setup->entry_trans_file, 0,
								  settings_openfiledialog, 1);
	gtkutil_label_new (_("Use a ircII style translation file."), wid);
}
#endif

static void
settings_page_dcc (struct session *sess, GtkWidget * vbox)
{
	GtkWidget *wid;
	GtkWidget *tog;

	wid = settings_create_group (vbox, _("General"));
	tog = settings_create_toggle (_("Auto Open DCC Send Window"), wid,
											prefs.autoopendccsendwindow,
											settings_ptoggle_check,
											&(sess->setup->prefs.
											  autoopendccsendwindow));
	add_tip (tog, _("Automatically open DCC Send Window"));
	tog = settings_create_toggle (_("Auto Open DCC Recv Window"), wid,
											prefs.autoopendccrecvwindow,
											settings_ptoggle_check,
											&(sess->setup->prefs.
											  autoopendccrecvwindow));
	add_tip (tog, _("Automatically open DCC Recv Window"));
	tog = settings_create_toggle (_("Auto Open DCC Chat Window"), wid,
											prefs.autoopendccchatwindow,
											settings_ptoggle_check,
											&(sess->setup->prefs.
											  autoopendccchatwindow));
	add_tip (tog, _("Automatically open DCC Chat Window"));
	tog = settings_create_toggle (_("Resume on Auto Accept"), wid,
											prefs.autoresume,
											settings_ptoggle_check,
											&(sess->setup->prefs.autoresume));
	add_tip (tog, _("When Auto-Accepting DCC, try to resume."));
}

static void
settings_page_dcc_filetransfer (struct session *sess, GtkWidget * vbox)
{
	GtkWidget *wid;
	GtkWidget *tab, *tog;
	GtkWidget *left_box, *right_box;

	gint row_index;
	char buf[127];

	wid = settings_create_group (vbox, _("Timeouts"));
	tab = settings_create_table (wid);
	row_index = 0;

	sess->setup->entry_dcctimeout =
		settings_create_numberentry (_("DCC Offers Timeout:"), 0, 10000, 1, tab,
								prefs.dcctimeout, row_index++, _("seconds."));
	sess->setup->entry_dccstalltimeout =
		settings_create_numberentry (_("DCC Stall Timeout:"), 0, 10000, 1, tab,
								prefs.dccstalltimeout, row_index++, _("seconds."));

	wid = settings_create_group (vbox, _("Received files"));
	tab = settings_create_table (wid);
	row_index = 0;
	sprintf (buf, "%04o", prefs.dccpermissions);
	settings_create_entry (_("File Permissions:"),
								  5, tab, buf,
								  &sess->setup->entry_permissions, _("(octal)"), 0,
								  row_index++);
	settings_create_entry (_("Directory to save to:"),
								  sizeof (prefs.dccdir) - 1, tab, prefs.dccdir,
								  &sess->setup->entry_dccdir, 0, 0, row_index++);
	tog = settings_create_toggle (_("Save file with Nickname"), wid,
											prefs.dccwithnick, settings_ptoggle_check,
											&(sess->setup->prefs.dccwithnick));
	add_tip (tog, _("Put the sender\'s nickname in incoming filenames"));

	wid = settings_create_group (vbox, _("DCC Send Options"));
	tab = settings_create_table (wid);

	settings_create_tworows (wid, &left_box, &right_box);

	tog = settings_create_toggle (_("Fast DCC Send"), left_box,
											prefs.fastdccsend, settings_ptoggle_check,
											&(sess->setup->prefs.fastdccsend));
	add_tip (tog, _("Don\'t wait for ACKs to send more data"));

	tog = settings_create_toggle (_("Fill Spaces"), right_box,
						 					prefs.dcc_send_fillspaces, settings_ptoggle_check,
											&(sess->setup->prefs.dcc_send_fillspaces));
	add_tip (tog, _("File names with spaces will be filled with underscore"));	

	sess->setup->entry_dcc_send_port_first =
		settings_create_numberentry (_("First DCC Send Port:"), 0, 65535, 1, tab,
								prefs.first_dcc_send_port, row_index++, _("(0=Disabled)"));
	sess->setup->entry_dcc_send_port_last =
		settings_create_numberentry (_("Last DCC Send Port:"), 0, 65535, 1, tab,
								prefs.last_dcc_send_port, row_index++, 0);

	sess->setup->entry_dcc_blocksize =
		settings_create_numberentry (_("Send Block Size:"), 0, 10000, 1, tab,
								prefs.dcc_blocksize, row_index++, _("(1024=Normal)"));
}

static void
settings_page_ctcp (struct session *sess, GtkWidget * vbox)
{
	GtkWidget *wid;
	GtkWidget *tab;
	GtkWidget *tog;
	gint row_index;

	wid = settings_create_group (vbox, _("Built-in Replies"));
	tog = settings_create_toggle (_("Hide Version"), wid,
											prefs.hidever, settings_ptoggle_check,
											&(sess->setup->prefs.hidever));
	add_tip (tog, _("Do not reply to CTCP version"));

	wid = settings_create_group (vbox, _("Sound"));
	tab = settings_create_table (wid);
	row_index = 0;
	settings_create_entry (_("Sound Dir:"),
								  sizeof (prefs.sounddir) - 1, tab,
								  prefs.sounddir,
								  &sess->setup->entry_sounddir, 0, 0, row_index++);
	settings_create_entry (_("Play Command:"),
								  sizeof (prefs.soundcmd) - 1, tab,
								  prefs.soundcmd,
								  &sess->setup->entry_soundcmd, 0, 0, row_index++);
}


/* Create Interface */

void
settings_opengui (struct session *sess)
{
	GtkCTreeNode *last_top;
	GtkCTreeNode *last_child;

	GtkWidget *dialog;
	GtkWidget *hbbox;
	GtkWidget *frame;
	GtkWidget *ctree;
	GtkWidget *book;
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *wid;

	gchar *titles[1];
	gint page_index;

	if (sess->setup)
	{
		gdk_window_show (sess->setup->settings_window->window);
		return;
	}
	sess->setup = malloc (sizeof (struct setup));
	memcpy (&sess->setup->prefs, &prefs, sizeof (struct xchatprefs));

	/* prepare the dialog */
	dialog = gtkutil_dialog_new (_("X-Chat: Preferences"), "preferences",
										  settings_closegui, sess);
	sess->setup->settings_window = dialog;

	/* prepare the action area */
	gtk_container_set_border_width
		(GTK_CONTAINER (GTK_DIALOG (dialog)->action_area), 2);
	gtk_box_set_homogeneous (GTK_BOX (GTK_DIALOG (dialog)->action_area),
									 FALSE);

	/* prepare the button box */
	hbbox = gtk_hbutton_box_new ();
	gtk_button_box_set_spacing (GTK_BUTTON_BOX (hbbox), 4);
	gtk_box_pack_end (GTK_BOX (GTK_DIALOG (dialog)->action_area), hbbox,
							FALSE, FALSE, 0);
	gtk_widget_show (hbbox);

	/* i love buttons */
#ifdef USE_GNOME
	wid = gnome_stock_button (GNOME_STOCK_BUTTON_OK);
#else
	wid = gtk_button_new_with_label (_("Ok"));
#endif
	gtk_signal_connect (GTK_OBJECT (wid), "clicked",
							  GTK_SIGNAL_FUNC (settings_ok_clicked), sess);
	gtk_box_pack_start (GTK_BOX (hbbox), wid, FALSE, FALSE, 0);
	gtk_widget_show (wid);

#ifdef USE_GNOME
	wid = gnome_stock_button (GNOME_STOCK_BUTTON_APPLY);
#else
	wid = gtk_button_new_with_label (_("Apply"));
#endif
	gtk_signal_connect (GTK_OBJECT (wid), "clicked",
							  GTK_SIGNAL_FUNC (settings_apply_clicked), sess);
	gtk_box_pack_start (GTK_BOX (hbbox), wid, FALSE, FALSE, 0);
	gtk_widget_show (wid);

#ifdef USE_GNOME
	wid = gnome_stock_button (GNOME_STOCK_BUTTON_CANCEL);
#else
	wid = gtk_button_new_with_label (_("Cancel"));
#endif
	gtk_signal_connect (GTK_OBJECT (wid), "clicked",
							  GTK_SIGNAL_FUNC (gtkutil_destroy),
							  sess->setup->settings_window);
	gtk_box_pack_start (GTK_BOX (hbbox), wid, FALSE, FALSE, 0);
	gtk_widget_show (wid);
	sess->setup->cancel_button = wid;

	/* the main hbox */
	hbox = gtk_hbox_new (FALSE, 6);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
							  hbox, TRUE, TRUE, 0);
	gtk_widget_show (hbox);

	/* the tree */
	titles[0] = _("Categories");
	ctree = gtk_ctree_new_with_titles (1, 0, titles);
	gtk_clist_set_selection_mode (GTK_CLIST (ctree), GTK_SELECTION_BROWSE);
	gtk_ctree_set_indent (GTK_CTREE (ctree), 15);
	gtk_widget_set_usize (ctree, 164, 0);
	gtk_box_pack_start (GTK_BOX (hbox), ctree, FALSE, FALSE, 0);
	gtk_widget_show (ctree);

	/* the preferences frame */
	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
	gtk_widget_show (frame);

	/* the notebook */
	book = gtk_notebook_new ();
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (book), FALSE);
	gtk_notebook_set_show_border (GTK_NOTEBOOK (book), FALSE);
	gtk_container_add (GTK_CONTAINER (frame), book);
	gtk_object_set_user_data (GTK_OBJECT (ctree), book);
	gtk_signal_connect (GTK_OBJECT (ctree), "tree_select_row",
							  GTK_SIGNAL_FUNC (settings_ctree_select), NULL);
	page_index = 0;

	vbox = settings_create_page (book, _("Interface Settings"),
										  ctree, _("Interface"),
										  NULL, &last_top, page_index++,
										  settings_page_interface, sess);
	gtk_ctree_select (GTK_CTREE (ctree), last_top);

	vbox = settings_create_page (book, _("IRC Input/Output Settings"),
										  ctree, _("IRC Input/Output"),
										  last_top, &last_child, page_index++,
										  settings_page_interface_inout, sess);

	vbox = settings_create_page (book, _("Window Layout Settings"),
										  ctree, _("Window Layout"),
										  last_top, &last_child, page_index++,
										  settings_page_interface_layout, sess);

	vbox = settings_create_page (book, _("Main Window Settings"),
										  ctree, _("Main Window"),
										  last_top, &last_child, page_index++,
										  settings_page_interface_mainwindow, sess);

	vbox = settings_create_page (book, _("Channel Window Settings"),
										  ctree, _("Channel Windows"),
										  last_top, &last_child, page_index++,
										  settings_page_interface_channelwindow, sess);

	vbox = settings_create_page (book, _("Dialog Window Settings"),
										  ctree, _("Dialog Windows"),
										  last_top, &last_child, page_index++,
										  settings_page_interface_dialogwindow, sess);

#ifdef USE_PANEL
	vbox = settings_create_page (book, _("Panel Settings"),
										  ctree, _("Panel"),
										  last_top, &last_child, page_index++,
										  settings_page_interface_panel, sess);
#endif

	vbox = settings_create_page (book, _("IRC Settings"),
										  ctree, _("IRC"),
										  NULL, &last_top, page_index++,
										  settings_page_irc, sess);

	vbox = settings_create_page (book, _("IP Address Settings"),
										  ctree, _("IP Address"),
										  last_top, &last_child, page_index++,
										  settings_page_irc_ipaddress, sess);

	vbox = settings_create_page (book, _("Proxy Server"),
										  ctree, _("Proxy Server"),
										  last_top, &last_child, page_index++,
										  settings_page_proxy, sess);

	vbox = settings_create_page (book, _("Away Settings"),
										  ctree, _("Away"),
										  last_top, &last_child, page_index++,
										  settings_page_irc_away, sess);

	vbox = settings_create_page (book, _("Highlighting Settings"),
										  ctree, _("Highlighting"),
										  last_top, &last_child, page_index++,
										  settings_page_irc_highlighting, sess);

	vbox = settings_create_page (book, _("Logging Settings"),
										  ctree, _("Logging"),
										  last_top, &last_child, page_index++,
										  settings_page_irc_logging, sess);

	vbox = settings_create_page (book, _("Notification Settings"),
										  ctree, _("Notification"),
										  last_top, &last_child, page_index++,
										  settings_page_irc_notification, sess);

#ifdef USE_TRANS
	vbox = settings_create_page (book, _("Character Set (Translation Tables)"),
										  ctree, _("Character Set"),
										  last_top, &last_child, page_index++,
										  settings_page_irc_charset, sess);
#endif

	vbox = settings_create_page (book, _("CTCP Settings"),
										  ctree, _("CTCP"),
										  last_top, &last_child, page_index++,
										  settings_page_ctcp, sess);

	vbox = settings_create_page (book, _("DCC Settings"),
										  ctree, _("DCC"),
										  NULL, &last_top, page_index++,
										  settings_page_dcc, sess);

	vbox = settings_create_page (book, _("File Transfer Settings"),
										  ctree, _("File Transfer"),
										  last_top, &last_child, page_index++,
										  settings_page_dcc_filetransfer, sess);

	/* since they all fit in the window, why not expand them all */
	gtk_ctree_expand_recursive ((GtkCTree *) ctree, 0);
	gtk_clist_select_row (GTK_CLIST (ctree), 0, 0);

	gtk_widget_show (book);
	gtk_widget_show (sess->setup->settings_window);
}
