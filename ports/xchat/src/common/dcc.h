/* dcc.h */

#include <time.h>						/* for time_t */

#define STAT_QUEUED 0
#define STAT_ACTIVE 1
#define STAT_FAILED 2
#define STAT_DONE 3
#define STAT_CONNECTING 4
#define STAT_ABORTED 5

#define TYPE_SEND 0
#define TYPE_RECV 1
#define TYPE_CHATRECV 2
#define TYPE_CHATSEND 3

struct DCC
{
	struct server *serv;
	struct dcc_chat *dccchat;
	unsigned long addr;			/* the 32bit IP number, host byte order */
	int fp;							/* file pointer */
	int sok;
	int iotag;
	int wiotag;
	int port;
	int cps;
	int size;
	int resumable;
	int ack;
	int oldack;
	int pos;
	int oldpos;
	time_t starttime;
	time_t offertime;
	time_t lasttime;
	char *file;
	char *destfile;
	char *nick;
	char type;				  /* 0 = SEND  1 = RECV  2 = CHAT */
	char dccstat;			  /* 0 = QUEUED  1 = ACTIVE  2 = FAILED  3 = DONE */
	unsigned int fastsend:1;
	unsigned int ackoffset:1;	/* is reciever sending acks as an offset from */
};										/* the resume point? */

struct dcc_chat
{
	char linebuf[1024];
	int pos;
};

struct dccstat_info
{
	char *name;						  /* Display name */
	int color;						  /* Display color (index into colors[] ) */
};

extern struct dccstat_info dccstat[];
extern char *dcctypes[];

void dcc_get (struct DCC *dcc);
void dcc_resume (struct DCC *dcc);
void dcc_check_timeouts (void);
void dcc_change_nick (server *serv, char *oldnick, char *newnick);
void dcc_send_filereq (session *sess, char *nick);
void dcc_close (struct DCC *dcc, int stat, int destroy);
void dcc_notify_kill (struct server *serv);
struct DCC *dcc_write_chat (char *nick, char *text);
void dcc_send (struct session *sess, char *tbuf, char *to, char *file);
struct DCC *find_dcc (char *nick, char *file, int type);
void dcc_get_nick (struct session *sess, char *nick);
void dcc_chat (session *sess, char *nick);
void handle_dcc (session *sess, char *outbuf, char *nick, char *word[],
					  char *word_eol[]);
void dcc_show_list (session *sess, char *outbuf);
void open_dcc_recv_window (void);
void open_dcc_send_window (void);
void open_dcc_chat_window (void);
