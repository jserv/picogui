typedef void (*filereqcallback) (void *, void *, char *file);

void gtkutil_file_req (char *title, void *callback, void *userdata,
							  void *userdata2, int write);
GtkWidget *gtkutil_dialog_new (char *title, char *wmclass, void *callback,
										 gpointer userdata);
void gtkutil_destroy (GtkWidget * igad, GtkWidget * dgad);
void gtkutil_destroy_window (GtkWidget * igad, struct session *sess);
GtkWidget *gtkutil_simpledialog (char *msg);
GtkWidget *gtkutil_button (GtkWidget * win, char *stock, char *labeltext,
									void *callback, gpointer userdata,
									GtkWidget * box);
void gtkutil_label_new (char *text, GtkWidget * box);
GtkWidget *gtkutil_entry_new (int max, GtkWidget * box, void *callback,
										gpointer userdata);
GtkWidget *gtkutil_clist_new (int columns, char *titles[], GtkWidget * box,
										int policy, void *select_callback,
										gpointer select_userdata,
										void *unselect_callback,
										gpointer unselect_userdata, int selection_mode);
int gtkutil_clist_selection (GtkWidget * clist);
void add_tip (GtkWidget * wid, char *text);
void show_and_unfocus (GtkWidget * wid);
