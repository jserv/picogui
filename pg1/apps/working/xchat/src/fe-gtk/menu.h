typedef void (*menucallback) (void *wid, void *data);

#define M_MENU 0
#define M_NEWMENURIGHT 6
#define M_MENUD 5
#define M_NEWMENU 1
#define M_END 2
#define M_SEP 3
#define M_MENUTOG 4

struct mymenu
{
	int type;
	char *text;
	menucallback callback;
	int state;
	int activate;
};

GtkWidget * createmenus (void *app, session *sess, int bar);
void menu_showhide (void);
void menu_newshell_set_palette (session *sess);
void menu_urlmenu (GdkEventButton * event, char *url);
void menu_chanmenu (session *sess, GdkEventButton * event, char *chan);
void menu_nickmenu (session *sess, GdkEventButton * event, char *nick);
void menu_middlemenu (session *sess, GdkEventButton *event);
void menu_quick_item_with_callback (void *callback, char *label, GtkWidget * menu, void *arg);
void userlist_button_cb (GtkWidget * button, char *cmd);
void goto_url (char *url);
void nick_command_parse (session *sess, char *cmd, char *nick, char *allnick);
void menu_create (GtkWidget *menu, GSList *list, char *target);
