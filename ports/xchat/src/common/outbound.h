extern struct commands xc_cmds[];

void user2serv (unsigned char *s);
void serv2user (unsigned char *s);
int load_trans_table (char *full_path);
int auto_insert (char *dest, char *src, char *word[], char *word_eol[],
				 char *a, char *c, char *d, char *h, char *n, char *s);
int handle_command (char *cmd, session *sess, int history, int nocommand);
void process_data_init (unsigned char *buf, char *cmd, char *word[],
						 char *word_eol[], int handle_quotes);
void handle_multiline (session *sess, char *cmd, int history, int nocommand);
void check_special_chars (char *cmd, int do_ascii);
void notc_msg (session *sess);
void notj_msg (session *sess);
void server_sendpart (server * serv, char *channel, char *reason);
void server_sendquit (session * sess);
void send_channel_modes (session *sess, char *tbuf,
						  char *word[], int start, int end, char sign, char mode);
