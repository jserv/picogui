extern GtkWidget *main_window;
extern GtkWidget *main_book;
extern GtkWidget *main_menu_bar;

extern GtkStyle *redtab_style;
extern GtkStyle *bluetab_style;
extern GtkStyle *inputgad_style;

void tree_blue_style (session *);
void tree_red_style (session *);
void create_window (session *);
void userlist_hide (GtkWidget *igad, session *sess);
void maingui_showhide_topic (session * sess);
void userlist_hide (GtkWidget * igad, session *sess);
int maingui_word_check (GtkWidget * xtext, char *word);
void maingui_word_clicked (GtkWidget *xtext, char *word, GdkEventButton *even, session *sess);
void link_cb (GtkWidget *win, session *sess);
void focus_in (GtkWindow * win, GtkWidget * wid, session *sess);
void maingui_configure (session *sess);
void gui_create_toolbox (session *sess, GtkWidget *box);
void gui_make_tab_window (session *sess, GtkWidget *win);
void handle_inputgad (GtkWidget * igad, session *sess);
void maingui_set_tab_pos (int pos);
