extern GSList *ignore_list;

extern int ignored_ctcp;
extern int ignored_priv;
extern int ignored_chan;
extern int ignored_noti;
extern int ignored_invi;

struct ignore
{
	unsigned char *mask;
	unsigned int priv:1;
	unsigned int chan:1;
	unsigned int ctcp:1;
	unsigned int noti:1;
	unsigned int invi:1;
	unsigned int unignore:1;
	unsigned int no_save:1;
};

int ignore_add (char *mask, int priv, int noti, int chan, int ctcp,
							  int mode, int unignore, int no_save);
void ignore_showlist (session *sess);
int ignore_del (char *mask, struct ignore *ig);
int ignore_check (char *mask, int priv, int noti, int chan, int ctcp,
								 int mode);
void ignore_load (void);
void ignore_save (void);
void ignore_gui_open (void);
void ignore_gui_update (int level);
int flood_check (char *nick, char *ip, server *serv, session *sess, int what);
