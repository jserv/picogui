int pys_load (session *sess, char *tbuf, char *word[], char *word_eol[]);
int pys_cmd_handle (char *cmd, session *sess, char **word);
int pys_pkill (session *sess, char *tbuf, char *word[], char *word_eol[]);
int pys_plist (session *sess, char *tbuf, char *word[], char *word_eol[]);
void pys_kill (void);
void pys_init (void);
