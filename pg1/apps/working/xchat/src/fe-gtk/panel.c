#include <stdio.h>
#include "fe-gtk.h"
#include "../common/xchat.h"
#include "../common/xchatc.h"
#include "gtkutil.h"
#include "wins.h"
#include "maingui.h"
#include "about.h"
#include <applet-widget.h>

static GtkWidget *panel_applet = NULL, *panel_box;
static GtkWidget *panel_popup = NULL;
int nopanel = FALSE;


static void
panel_invalidate (GtkWidget * wid, gpointer * arg2)
{
	panel_applet = NULL;
}

void
panel_cleanup (void)
{
	/* When we want to exit we wipe the panel applet */
	if (panel_applet != NULL) {
		 applet_widget_remove (APPLET_WIDGET (panel_applet));
		 panel_applet = NULL;
	}
}

static void
create_panel_widget ()
{
	panel_applet = applet_widget_new ("xchat_applet");
	gtk_widget_realize (panel_applet);

	if (prefs.panel_vbox)
		panel_box = gtk_vbox_new (0, 0);
	else
		panel_box = gtk_hbox_new (0, 0);
	applet_widget_add (APPLET_WIDGET (panel_applet), panel_box);
	gtk_widget_show (panel_box);

	gtkutil_label_new ("X-Chat:", panel_box);

	applet_widget_register_stock_callback (APPLET_WIDGET (panel_applet),
														"about",
														GNOME_STOCK_MENU_ABOUT,
														_("About..."),
														GTK_SIGNAL_FUNC (menu_about), NULL);

	gtk_signal_connect (GTK_OBJECT (panel_applet), "destroy", panel_invalidate,
							  NULL);

	/* Never save session on the panel, it wouldn't work anyhow */
	gtk_signal_connect (GTK_OBJECT (panel_applet), "save_session",
			    GTK_SIGNAL_FUNC (gtk_true), NULL);

	gtk_widget_show (panel_applet);
}

static void
gui_panel_destroy_popup (GtkWidget * wid, GtkWidget * popup)
{
	gtk_widget_destroy (panel_popup);
	panel_popup = NULL;
}

static void
gui_panel_remove_clicked (GtkWidget * button, struct session *sess)
{
	gtk_widget_show (sess->gui->window);
	gtk_widget_destroy (sess->gui->panel_button);
	sess->gui->panel_button = 0;
	if (main_window)
	{
		wins_bring_tofront (sess->gui->window);
	}
	if (panel_popup)
		gui_panel_destroy_popup (NULL, NULL);
}

static void
gui_panel_hide_clicked (GtkWidget * button, struct session *sess)
{
	gtk_widget_hide (sess->gui->window);
}

static void
gui_panel_show_clicked (GtkWidget * button, struct session *sess)
{
	gtk_widget_show (sess->gui->window);
}

static void
gui_panel_here_clicked (GtkWidget * button, struct session *sess)
{
	if (sess->is_tab)
	{
		if (main_window)
		{
			gtk_widget_hide (main_window);
			gtk_window_set_position (GTK_WINDOW (main_window),
											 GTK_WIN_POS_MOUSE);
			gtk_widget_show (main_window);
		}
	} else
	{
		gtk_widget_hide (sess->gui->window);
		gtk_window_set_position (GTK_WINDOW (sess->gui->window),
										 GTK_WIN_POS_MOUSE);
		gtk_widget_show (sess->gui->window);
	}
}

static void
gui_panel_destroy_view_popup (GtkWidget * popup, /* Event */ gpointer * arg2,
										struct session *sess)
{
	/* BODGE ALERT !! BODGE ALERT !! --AGL */
	gtk_widget_reparent (sess->gui->textgad, sess->gui->leftpane);
	gtk_box_reorder_child (GTK_BOX (sess->gui->leftpane), sess->gui->textgad,
								  0);

	gtk_widget_destroy (popup);
}

static void
gui_panel_view_clicked (GtkWidget * button, struct session *sess)
{
	GtkWidget *view_popup;

	view_popup = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_widget_show_all (view_popup);
	gtk_widget_reparent (sess->gui->textgad, view_popup);
	gtk_signal_connect (GTK_OBJECT (view_popup), "leave_notify_event",
							  gui_panel_destroy_view_popup, sess);
}

static gint
gui_panel_button_event (GtkWidget * button, GdkEvent * event,
								struct session *sess)
{
	if (event->type == GDK_BUTTON_PRESS && event->button.button == 3)
	{
		GtkWidget *vbox, *wid;

		panel_popup = gtk_window_new (GTK_WINDOW_POPUP);
		gtk_window_set_position (GTK_WINDOW (panel_popup), GTK_WIN_POS_MOUSE);
		vbox = gtk_vbox_new (0, 0);
		gtk_container_add (GTK_CONTAINER (panel_popup), vbox);

		wid = gtk_label_new ("");
		if (sess->channel[0])
			gtk_label_set_text (GTK_LABEL (wid), sess->channel);
		else
			gtk_label_set_text (GTK_LABEL (wid), _("No Channel"));
		gtk_box_pack_start (GTK_BOX (vbox), wid, 0, 0, 0);

		wid = gtk_label_new ("");
		if (sess->server->hostname[0])
			gtk_label_set_text (GTK_LABEL (wid), sess->server->servername);
		else
			gtk_label_set_text (GTK_LABEL (wid), _("No Server"));
		gtk_box_pack_start (GTK_BOX (vbox), wid, 0, 0, 0);

		wid = gtk_label_new ("");
		if (sess->is_tab)
			gtk_label_set_text (GTK_LABEL (wid), _("Is Tab"));
		else
			gtk_label_set_text (GTK_LABEL (wid), _("Is Not Tab"));
		gtk_box_pack_start (GTK_BOX (vbox), wid, 0, 0, 0);

		wid = gtk_button_new_with_label (_("Close"));
		gtk_signal_connect (GTK_OBJECT (wid), "clicked",
								  GTK_SIGNAL_FUNC (gtkutil_destroy),
								  sess->gui->window);
		gtk_box_pack_start (GTK_BOX (vbox), wid, 0, 0, 0);
		wid = gtk_button_new_with_label (_("Remove"));
		gtk_signal_connect (GTK_OBJECT (wid), "clicked",
								  GTK_SIGNAL_FUNC (gui_panel_remove_clicked), sess);
		gtk_box_pack_start (GTK_BOX (vbox), wid, 0, 0, 0);
		wid = gtk_button_new_with_label (_("Hide"));
		gtk_signal_connect (GTK_OBJECT (wid), "clicked",
								  GTK_SIGNAL_FUNC (gui_panel_hide_clicked), sess);
		gtk_box_pack_start (GTK_BOX (vbox), wid, 0, 0, 0);
		wid = gtk_button_new_with_label (_("Show"));
		gtk_signal_connect (GTK_OBJECT (wid), "clicked",
								  GTK_SIGNAL_FUNC (gui_panel_show_clicked), sess);
		gtk_box_pack_start (GTK_BOX (vbox), wid, 0, 0, 0);
/*		wid = gtk_button_new_with_label (_("De/Link"));
		gtk_signal_connect (GTK_OBJECT (wid), "clicked",
								  GTK_SIGNAL_FUNC (relink_window), sess);
		gtk_box_pack_start (GTK_BOX (vbox), wid, 0, 0, 0);*/
		wid = gtk_button_new_with_label (_("Move Here"));
		gtk_signal_connect (GTK_OBJECT (wid), "clicked",
								  GTK_SIGNAL_FUNC (gui_panel_here_clicked), sess);
		gtk_box_pack_start (GTK_BOX (vbox), wid, 0, 0, 0);
		wid = gtk_button_new_with_label (_("View"));
		gtk_signal_connect (GTK_OBJECT (wid), "clicked",
								  GTK_SIGNAL_FUNC (gui_panel_view_clicked), sess);
		gtk_box_pack_start (GTK_BOX (vbox), wid, 0, 0, 0);

		gtk_signal_connect (GTK_OBJECT (panel_popup), "leave_notify_event",
								  gui_panel_destroy_popup, panel_popup);
		gtk_widget_show_all (panel_popup);

	}
	return 0;
}

static void
maingui_unpanelize (GtkWidget * button, struct session *sess)
{
	if (!sess->is_tab)
		gtk_widget_set_rc_style (GTK_BIN (sess->gui->panel_button)->child);
	if (prefs.panelize_hide)
	{
		gtk_container_remove (GTK_CONTAINER (button->parent), button);
		gtk_widget_destroy (button);
	}
	gtk_widget_show (sess->gui->window);
	if (prefs.panelize_hide)
		sess->gui->panel_button = 0;
	if (main_window)				  /* this fixes a little refresh glitch */
	{
		wins_bring_tofront (sess->gui->window);
	}
}

void
maingui_panelize (GtkWidget *win)
{
	char tbuf[128];
	session *sess;
	GSList *list;
	GtkWidget *button;

	/* YUK! FIXME */
	list = sess_list;
	while (list)
	{
		sess = list->data;
		if (sess->gui->window == win)
			break;
		list = list->next;
	}

	if (!panel_applet)
		create_panel_widget ();

	if (sess->gui->panel_button != NULL)
		return;

	if (prefs.panelize_hide)
		gtk_widget_hide (sess->gui->window);

	if (sess->channel[0] == 0)
		button = gtk_button_new_with_label (_("<none>"));
	else
		button = gtk_button_new_with_label (sess->channel);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
							  GTK_SIGNAL_FUNC (maingui_unpanelize), sess);
	gtk_container_add (GTK_CONTAINER (panel_box), button);
	gtk_signal_connect (GTK_OBJECT (button), "button_press_event",
							  GTK_SIGNAL_FUNC (gui_panel_button_event), sess);
	gtk_widget_show (button);
	sess->gui->panel_button = button;

	/* NULL the "panel_button" member when it gets destroyed */
	gtk_signal_connect (GTK_OBJECT (button), "destroy",
			    GTK_SIGNAL_FUNC (gtk_widget_destroyed),
			    &sess->gui->panel_button);

	if (sess->channel[0])
	{
		snprintf (tbuf, sizeof tbuf, "%s: %s", sess->server->servername,
					 sess->channel);
		add_tip (button, tbuf);
	}
}
