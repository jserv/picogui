int is_channel (server *serv, char *chan);
char get_nick_prefix (server *serv, unsigned int access);
unsigned int nick_access (server *serv, char *nick, int *modechars);
int mode_access (server *serv, char mode, char *prefix);
void handle_005 (server *serv, char *word[]);
void handle_mode (server *serv, char *outbuf, char *word[], char *word_eol[], char *nick, int numeric_324);
