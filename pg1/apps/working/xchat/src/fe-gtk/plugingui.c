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

/* plugin.c by Adam Langley */

#define	PLUGIN_C

#include <string.h>
#include <stdio.h>

#include "fe-gtk.h"
#include "../common/xchat.h"
#include "../common/plugin.h"
#include "../common/text.h"
#include "../common/util.h"
#include "gtkutil.h"

#ifdef USE_PLUGIN


static GtkWidget *modlist = 0, *modclist;

/* ************** GUI STUFF ***************** */

static void
module_glist_close (GtkWidget * wid, gpointer * a)
{
	gtk_widget_destroy (modlist);
	modlist = NULL;
}

void
fe_pluginlist_update (void)
{
	struct module *mod;
	gchar *entry[2];

	if (!modlist)
		return;

	mod = modules;
	gtk_clist_clear (GTK_CLIST (modclist));
	for (;;)
	{
		if (mod == NULL)
			break;
		entry[0] = mod->name;
		entry[1] = mod->desc;
		gtk_clist_append (GTK_CLIST (modclist), entry);

		mod = mod->next;
	}
}

static void
module_glist_unload (GtkWidget * wid, struct session *sess)
{
	int row;
	char *modname;

	row = gtkutil_clist_selection (modclist);
	if (row == -1)
		return;
	gtk_clist_get_text (GTK_CLIST (modclist), row, 0, &modname);
	module_unload (modname, sess);
}

void
module_glist (struct session *sess)
{
	gchar *titles[] = { _("Name"), _("Description") };
	GtkWidget *okb, *ulb;

	if (modlist)
		return;

	modlist = gtk_dialog_new ();
	gtk_signal_connect (GTK_OBJECT (modlist), "delete_event",
							  GTK_SIGNAL_FUNC (module_glist_close), 0);
	gtk_widget_set_usize (modlist, 350, 200);
	gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (modlist)->vbox),
											  4);
	gtk_window_set_position (GTK_WINDOW (modlist), GTK_WIN_POS_CENTER);
	gtk_window_set_title (GTK_WINDOW (modlist), _("X-Chat Plugins"));
	gtk_window_set_wmclass (GTK_WINDOW (modlist), "plugins", "X-Chat");

	modclist = gtk_clist_new_with_titles (2, titles);
	gtk_clist_set_selection_mode (GTK_CLIST (modclist), GTK_SELECTION_BROWSE);
	gtk_clist_column_titles_passive (GTK_CLIST (modclist));
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (modlist)->vbox), modclist, 1, 1,
							  10);
	gtk_widget_show (modclist);

	gtk_clist_set_column_width (GTK_CLIST (modclist), 0, 40);

#ifdef	USE_GNOME
	okb = gnome_stock_button (GNOME_STOCK_BUTTON_OK);
#else
	okb = gtk_button_new_with_label (_("Ok"));
#endif
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (modlist)->action_area), okb, 1, 1,
							  10);
	gtk_signal_connect (GTK_OBJECT (okb), "clicked",
							  GTK_SIGNAL_FUNC (module_glist_close),
							  (gpointer) modlist);
	gtk_widget_show (okb);

	ulb = gtk_button_new_with_label (_("Unload"));
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (modlist)->action_area), ulb, 1, 1,
							  10);
	gtk_signal_connect (GTK_OBJECT (ulb), "clicked",
							  GTK_SIGNAL_FUNC (module_glist_unload),
							  (gpointer) sess);
	gtk_widget_show (ulb);

	fe_pluginlist_update ();

	gtk_widget_show (modlist);
}
#endif
