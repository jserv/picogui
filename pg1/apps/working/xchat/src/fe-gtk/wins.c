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

/* xchat's window/tab functions */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "fe-gtk.h"
#include "../common/xchat.h"
#include "../common/fe.h"
#include "pixmaps.h"
#include "gtkutil.h"
#include "maingui.h"
#include "panel.h"
#include "wins.h"

#ifdef USE_XLIB
#include <gdk/gdkx.h>
#endif

typedef struct xchatwin
{
	GtkWidget *child;	/* GtkWindow or hbox for notebooktab */
	GtkWidget *vbox;	/* main contents */
	GtkWidget *notebooklabel;
	GtkWidget *button_box;
	GtkWidget *button_box_box;
	GtkWidget **child_ret;	/* give the client changes (when linking) */
	void (*close_callback) (void *userdata);
	void *userdata;
	void (*link_callback) (GtkWidget *win, void *userdata);	/* called when de/relinked */
	void *link_userdata;
	char *title;
	char *name;
	unsigned int is_toplevel:1;
	unsigned int is_nonirc:1;
} xchatwin;


/* private functions - callbacks */

static void
wins_free (xchatwin *xw)
{
	free (xw->title);
	free (xw->name);
	free (xw);
}

static gint
wins_destroy_cb (GtkWidget *win, xchatwin *xw)
{
	if (xw->close_callback != NULL)
		xw->close_callback (xw->userdata);
	wins_free (xw);

	return TRUE;
}

static void
wins_X_cb (GtkWidget *button, xchatwin *xw)
{
	gtk_widget_destroy (xw->child);
}

/* delink a tab OR relink a window */

static void
wins_link_cb (GtkWidget *button, xchatwin *xw)
{
	GtkWidget *new_win;
	GtkWidget *old_win;
	xchatwin *new_xw;
#ifdef USE_GNOME
	GtkWidget *wid;
#endif

	old_win = xw->child;

	/* make sure wins_destroy_cb isn't called */
	gtk_signal_disconnect_by_data (GTK_OBJECT (old_win), xw);

	/* do we need to create a main window? */
	if (main_window == NULL && xw->is_toplevel)
	{
		gui_make_tab_window (NULL, old_win);
		gtk_widget_show (main_window);
	}

	/* create a new tab/window */
	new_win = wins_new (xw->name, xw->title, xw->close_callback,
								xw->userdata, xw->link_callback,
								xw->link_userdata, xw->is_toplevel, xw->is_nonirc,
								xw->child_ret);
	new_xw = gtk_object_get_user_data (GTK_OBJECT (new_win));

	/* give the client the new window pointer */
	if (xw->child_ret != NULL)
		*xw->child_ret = new_win;

	/* use the old vbox instead */
	gtk_widget_destroy (new_xw->vbox);
	new_xw->vbox = xw->vbox;

#ifdef USE_GNOME
	if (xw->is_toplevel)
	{
		gtk_widget_reparent (xw->vbox, new_win);
	} else
	{
		wid = gtk_hbox_new (0, 0);
		gtk_widget_reparent (xw->vbox, wid);
		gnome_app_set_contents (GNOME_APP (new_win), wid);
	}
#else
	gtk_widget_reparent (xw->vbox, new_win);
#endif

	if (xw->button_box != NULL)
	{
		/* buttons need to be re-created so that the parameters to the
			callbacks are updated. */
		gtk_widget_destroy (xw->button_box);
		wins_create_linkbuttons (new_win, xw->button_box_box);
	}

	gtk_widget_destroy (old_win);
	gtk_widget_show (new_win);
	wins_free (xw);

	/* this will call link_cb() in maingui.c */
	if (new_xw->link_callback != NULL)
		new_xw->link_callback (new_win, new_xw->link_userdata);

	/* did we just remove the last page? if so, destroy the main window */
	if (gtk_notebook_get_nth_page (GTK_NOTEBOOK (main_book), 0) == NULL)
		gtk_widget_destroy (main_window);
}

static void
wins_moveright_cb (GtkWidget *button, xchatwin *xw)
{
	wins_move_leftorright (xw->child, FALSE);
}

static void
wins_moveleft_cb (GtkWidget *button, xchatwin *xw)
{
	wins_move_leftorright (xw->child, TRUE);
}

#ifdef USE_PANEL
static void
wins_panelize_cb (GtkWidget *button, xchatwin *xw)
{
	maingui_panelize (xw->child);
}
#endif

/* private functions */

static xchatwin *
wins_add_window (GtkWidget *win, char *title, char *name)
{
	xchatwin *xw;

	xw = malloc (sizeof (struct xchatwin));
	memset (xw, 0, sizeof (struct xchatwin));
	xw->title = strdup (title);
	xw->name = strdup (name);
	xw->child = win;

	gtk_signal_connect (GTK_OBJECT (win), "destroy",
								GTK_SIGNAL_FUNC (wins_destroy_cb), xw);

	gtk_object_set_user_data (GTK_OBJECT (win), xw);

	return xw;
}

static void
wins_addtabto_notebook (GtkWidget *tab, xchatwin *xw, char *name, int nonirc)
{
	char *buf;
	GtkWidget *label;

	if (nonirc)
	{
		buf = malloc (strlen (name) + 3);
		sprintf (buf, "(%s)", name);
		label = gtk_label_new (buf);
		free (buf);
	} else
	{
		label = gtk_label_new (name);
	}
	gtk_widget_show (label);
	xw->notebooklabel = label;

	gtk_notebook_append_page (GTK_NOTEBOOK (main_book), tab, label);
}

static GtkWidget *
wins_create_button (GtkWidget *box, char *stock, char *label, char *tip,
							void *callback, void *userdata)
{
	GtkWidget *wid;

#ifdef USE_GNOME
	wid = gtkutil_button (box, stock, 0, callback, userdata, 0);
	gtk_box_pack_start (GTK_BOX (box), wid, 0, 0, 0);
#else
	wid = gtk_button_new_with_label (label);
	gtk_box_pack_start (GTK_BOX (box), wid, 0, 0, 0);
	gtk_signal_connect (GTK_OBJECT (wid), "clicked",
							  GTK_SIGNAL_FUNC (callback), userdata);
#endif
	show_and_unfocus (wid);
	add_tip (wid, tip);

	return wid;
}

static GtkWidget *
wins_create_tab (char *name, char *title, int nonirc, xchatwin **xw_ret)
{
	GtkWidget *tab;
	GtkWidget *vbox;
	xchatwin *xw;

	tab = gtk_hbox_new (0, 0);

	vbox = gtk_vbox_new (0, 0);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 0);
	gtk_container_add (GTK_CONTAINER (tab), vbox);
	gtk_widget_show (vbox);

	xw = wins_add_window (tab, title, name);

	xw->is_toplevel = FALSE;
	xw->vbox = vbox;

	wins_addtabto_notebook (tab, xw, name, nonirc);

	*xw_ret = xw;

	return tab;
}

static GtkWidget *
wins_create_win (char *name, char *title, xchatwin **xw_ret)
{
	GtkWidget *win;
	GtkWidget *vbox;
	xchatwin *xw;

#ifdef USE_GNOME
	win = gnome_app_new ("X-Chat", title);
#else
	win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (win), title);
#endif
	gtk_window_set_wmclass (GTK_WINDOW (win), name, "X-Chat");

	wins_set_icon (win);
	gtk_window_set_policy (GTK_WINDOW (win), TRUE, TRUE, FALSE);

	vbox = gtk_vbox_new (0, 0);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 0);
#ifdef USE_GNOME
	gnome_app_set_contents (GNOME_APP (win), vbox);
#else
	gtk_container_add (GTK_CONTAINER (win), vbox);
#endif
	gtk_widget_show (vbox);

	xw = wins_add_window (win, title, name);

	xw->is_toplevel = TRUE;
	xw->vbox = vbox;

	*xw_ret = xw;

	return win;
}

/* public functions */

GtkWidget *
wins_get_vbox (GtkWidget *win)
{
	xchatwin *xw;

	xw = (xchatwin *) gtk_object_get_user_data (GTK_OBJECT (win));
	return xw->vbox;
}

GtkWidget *
wins_get_notebooklabel (GtkWidget *win)
{
	xchatwin *xw;

	xw = (xchatwin *) gtk_object_get_user_data (GTK_OBJECT (win));
	return xw->notebooklabel;
}

void
wins_create_linkbuttons (GtkWidget *win, GtkWidget *box)
{
	GtkWidget *hbox;
	xchatwin *xw;

	xw = gtk_object_get_user_data (GTK_OBJECT (win));

	hbox = gtk_hbox_new (0, 0);
	gtk_box_pack_start (GTK_BOX (box), hbox, 0, 0, 0);
	gtk_widget_show (hbox);

	gtk_box_reorder_child (GTK_BOX (box), hbox, 0);

	xw->button_box = hbox;
	xw->button_box_box = box;

	wins_create_button (hbox, GNOME_STOCK_BUTTON_CANCEL, "X",
								_("Close this tab/window"), wins_X_cb, xw);

	wins_create_button (hbox, GNOME_STOCK_PIXMAP_REMOVE, "^",
								_("Link/DeLink this tab"), wins_link_cb, xw);

#ifdef USE_PANEL
	if (!nopanel)
	{
		GtkWidget *wid = gtkutil_button (win, GNOME_STOCK_PIXMAP_BOTTOM,
									 0, wins_panelize_cb, xw, 0);
		gtk_box_pack_start (GTK_BOX (hbox), wid, 0, 0, 0);
		add_tip (wid, _("Panelize"));
	}
#endif

	if (!xw->is_toplevel)
	{
		wins_create_button (hbox, GNOME_STOCK_PIXMAP_BACK, "<",
								_("Move tab left"), wins_moveleft_cb, xw);

		wins_create_button (hbox, GNOME_STOCK_PIXMAP_FORWARD, ">",
								_("Move tab right"), wins_moveright_cb, xw);
	}
}

GtkWidget *
wins_new (char *name, char *title, void *close_callback, void *userdata,
				void *link_callback, void *link_userdata,
				int tab, int nonirc, GtkWidget **child_ret)
{
	GtkWidget *win;
	xchatwin *xw;

	if (tab)
		win = wins_create_tab (name, title, nonirc, &xw);
	else
		win = wins_create_win (name, title, &xw);

	xw->close_callback = close_callback;
	xw->userdata = userdata;
	xw->link_callback = link_callback;
	xw->link_userdata = link_userdata;
	xw->is_nonirc = nonirc;
	xw->child_ret = child_ret;

	return win;
}

void
wins_update_notebooktitle (GtkWidget *fronttab)
{
	xchatwin *xw;

	xw = (xchatwin *) gtk_object_get_user_data (GTK_OBJECT (fronttab));
	gtk_window_set_title (GTK_WINDOW (main_window), xw->title);
}

void
wins_set_name (GtkWidget *widget, char *new_name)
{
	xchatwin *xw;

	xw = (xchatwin *) gtk_object_get_user_data (GTK_OBJECT (widget));
	free (xw->name);
	xw->name = strdup (new_name);
}

void
wins_set_title (GtkWidget *widget, char *new_title)
{
	xchatwin *xw;
	int fpage;

	xw = (xchatwin *) gtk_object_get_user_data (GTK_OBJECT (widget));

	free (xw->title);
	xw->title = strdup (new_title);

	if (xw->is_toplevel)
	{
		gtk_window_set_title (GTK_WINDOW (xw->child), new_title);
	} else
	{
		if (main_window)
		{
			/* set WINDOW title only if tab is the front one */
			fpage = gtk_notebook_get_current_page (GTK_NOTEBOOK (main_book));
			if (fpage == gtk_notebook_page_num (GTK_NOTEBOOK (main_book), widget))
			{
				gtk_window_set_title (GTK_WINDOW (main_window), new_title);
			}
		}
	}
}

void
wins_bring_tofront (GtkWidget *widget)
{
	xchatwin *xw;
	int page;

	xw = (xchatwin *) gtk_object_get_user_data (GTK_OBJECT (widget));

	if (xw->is_toplevel)
	{
		gdk_window_show (widget->window);
	} else
	{
		page = gtk_notebook_page_num (GTK_NOTEBOOK (main_book), widget);
		gtk_notebook_set_page (GTK_NOTEBOOK (main_book), page);
	}
}

void
wins_move_leftorright (GtkWidget *win, int left)
{
	int pos;
	xchatwin *xw;

	xw = (xchatwin *) gtk_object_get_user_data (GTK_OBJECT (win));

	pos = gtk_notebook_get_current_page (GTK_NOTEBOOK (main_book));

	if (left)
	{
		if (pos > 0)
			gtk_notebook_reorder_child (GTK_NOTEBOOK (main_book), xw->child, pos - 1);
	} else
	{
		gtk_notebook_reorder_child (GTK_NOTEBOOK (main_book), xw->child, pos + 1);
	}
}

void
wins_set_icon (GtkWidget *win)
{
#ifdef USE_XLIB
	GdkAtom icon_atom;
	glong data[2];

	gtk_widget_realize (win);

	data[0] = GDK_WINDOW_XWINDOW (pix_xchat_mini);
	data[1] = GDK_WINDOW_XWINDOW (mask_xchat_mini);
	icon_atom = gdk_atom_intern ("KWM_WIN_ICON", FALSE);
	gdk_property_change (win->window, icon_atom, icon_atom,
								32, GDK_PROP_MODE_REPLACE, (guchar *) data, 2);

	gdk_window_set_icon (win->window, NULL, pix_xchat, mask_xchat);
	gdk_window_set_icon_name (win->window, PACKAGE);
#endif
}

