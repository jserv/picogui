GtkWidget * wins_get_vbox (GtkWidget *win);
GtkWidget * wins_get_notebooklabel (GtkWidget *win);
void wins_create_linkbuttons (GtkWidget *win, GtkWidget *box);
GtkWidget *
wins_new (char *name, char *title, void *close_callback, void *userdata,
				void *link_callback, void *link_userdata,
				int tab, int nonirc, GtkWidget **child_ret);
void wins_update_notebooktitle (GtkWidget *fronttab);
void wins_set_name (GtkWidget *widget, char *new_name);
void wins_set_title (GtkWidget *widget, char *new_title);
void wins_bring_tofront (GtkWidget *widget);
void wins_move_leftorright (GtkWidget *win, int left);
void wins_set_icon (GtkWidget *win);

GtkWidget *
maingui_window (char *name, char *title, int force_toplevel,
						int link_buttons,
						void *close_callback, void *userdata,
						int width, int height, GtkWidget **child_ret);

