extern struct xchatprefs prefs;

extern int auto_connect;
extern int xchat_is_quitting;

extern session *menu_sess;
extern session *current_tab;

extern GSList *popup_list;
extern GSList *button_list;
extern GSList *dlgbutton_list;
extern GSList *command_list;
extern GSList *ctcp_list;
extern GSList *replace_list;
extern GSList *sess_list;
extern GSList *serv_list;
extern GSList *dcc_list;
extern GSList *ignore_list;
extern GSList *usermenu_list;
extern GSList *urlhandler_list;

int tcp_send_len (server *serv, char *buf, int len);
int tcp_send (server *serv, char *buf);
session * find_session_from_channel (char *chan, server *serv);
session * find_dialog (server *serv, char *nick);
session * new_ircwindow (server *serv, char *name, int type);
void set_server_defaults (server *serv);
struct away_msg *find_away_message (struct server *serv, char *nick);
void save_away_message (server *serv, char *nick, char *msg);
int is_server (server * serv);
int is_session (session * sess);
void lag_check (void);
void kill_session_callback (session * killsess);
void xchat_exit (void);
void xchat_exec (char *cmd);
