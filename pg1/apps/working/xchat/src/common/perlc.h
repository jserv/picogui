void perl_notify_kill (session * sess);
int cmd_unloadall (session *sess, char *tbuf, char *word[], char *word_eol[]);
int perl_load_file (char *script_name);
void perl_auto_load (session *sess);
void perl_end (void);
int perl_inbound (session *sess, server *serv, char *buf, char *msg_type);
int perl_dcc_chat (session *sess, server *serv, char *buf);
int perl_command (char *cmd, session *sess);
int perl_print (char *cmd, session *sess, char *b, char *c, char *d, char *e);
