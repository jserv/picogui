void userlist_dnd_drop (GtkWidget * widget, GdkDragContext * context,
						 gint x, gint y,
						 GtkSelectionData * selection_data,
						 guint info, guint32 time, session *sess);
int userlist_dnd_motion (GtkWidget * widget, GdkDragContext * context, gint x,
							gint y, guint ttime);
int userlist_dnd_leave (GtkWidget * widget, GdkDragContext * context, guint ttime);
