#include "../../config.h"

#ifdef USE_MYGLIB
#include "../fe-text/myglib.h"
#else
#include <glib.h>
#endif
#include <time.h>			/* need time_t */

#include "history.h"

#ifndef HAVE_SNPRINTF
#define snprintf g_snprintf
#endif

#ifdef USE_DEBUG
#define malloc(n) xchat_malloc(n, __FILE__, __LINE__)
#define realloc(n, m) xchat_realloc(n, m, __FILE__, __LINE__)
#define free(n) xchat_free(n, __FILE__, __LINE__)
#define strdup(n) xchat_strdup(n, __FILE__, __LINE__)
void *xchat_malloc (int size, char *file, int line);
void *xchat_strdup (char *str, char *file, int line);
void xchat_free (void *buf, char *file, int line);
void *xchat_realloc (char *old, int len, char *file, int line);
#endif

#ifdef SOCKS
#ifdef __sgi
#include <sys/time.h>
#define INCLUDE_PROTOTYPES 1
#endif
#include <socks.h>
#endif

#ifdef USE_OPENSSL
#include <openssl/ssl.h>		  /* SSL_() */
#endif

#ifdef __EMX__						  /* for o/s 2 */
#define OFLAGS O_BINARY
#define strcasecmp stricmp
#define strncasecmp strnicmp
#define PATH_MAX MAXPATHLEN
#define FILEPATH_LEN_MAX MAXPATHLEN
#else
#ifdef WIN32
#define OFLAGS O_BINARY
#define sleep(t) _sleep(t*1000)
#include <direct.h>
#define	F_OK	0
#define	X_OK	1
#define	W_OK	2
#define	R_OK	4
#ifndef S_ISDIR
#define	S_ISDIR(m)	((m) & _S_IFDIR)
#endif
#define NETWORK_PRIVATE
#else
#define OFLAGS 0
#endif
#endif

#define FONTNAMELEN	127
#define PATHLEN		255
#define NICKLEN		64				/* including the NULL, so 63 really */
#define CHANLEN		202
#define PDIWORDS		32

#define safe_strcpy(dest,src,len)	strncpy(dest,src,len); \
												dest[len-1] = 0;

#if defined(ENABLE_NLS) && !defined(_)
#  include <libintl.h>
#  define _(x) gettext(x)
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
#endif
#if !defined(_)
#  define N_(String) (String)
#  define _(x) (x)
#endif

struct nbexec
{
	int myfd;
	int childpid;
	int tochannel;					  /* making this int keeps the struct 4-byte aligned */
	int iotag;
	char *linebuf;
	int buffill;
	struct session *sess;
};

struct xchatprefs
{
	char nick1[64];
	char nick2[64];
	char nick3[64];
	char realname[127];
	char username[127];
	char nick_suffix[4];			/* Only ever holds a one-character string. */
	char awayreason[256];
	char quitreason[256];
	char partreason[256];
	char font_normal[FONTNAMELEN + 1];
	char dialog_font_normal[FONTNAMELEN + 1];
	char font_shell[FONTNAMELEN + 1];
	char doubleclickuser[256];
	char sounddir[PATHLEN + 1];
	char soundcmd[PATHLEN + 1];
	char background[PATHLEN + 1];
	char background_dialog[PATHLEN + 1];
	char dccdir[PATHLEN + 1];
	char bluestring[300];
	char dnsprogram[72];
	char hostname[127];
	char cmdchar[4];
	char trans_file[256];
	char logmask[256];
	char stamp_format[64];
	char timestamp_log_format[64];

	char proxy_host[64];
	int proxy_port;
	int proxy_type; /* 0=disabled, 1=wingate 2=socks4, 3=socks5, 4=http */

	int first_dcc_send_port;
	int last_dcc_send_port;

	int tint_red;
	int tint_green;
	int tint_blue;

	int dialog_tint_red;
	int dialog_tint_green;
	int dialog_tint_blue;

	int tabs_position;
	int max_auto_indent;
	int indent_pixels;
	int dialog_indent_pixels;
	int dcc_blocksize;
	int max_lines;
	int notify_timeout;
	int dcctimeout;
	int dccstalltimeout;
	int mainwindow_left;
	int mainwindow_top;
	int mainwindow_width;
	int mainwindow_height;
	int dccpermissions;
	int recon_delay;
	int bantype;
	int userlist_sort;
	int nu_color;
	unsigned long local_ip;
	unsigned long dcc_ip;
	char dcc_ip_str[16];

	unsigned int mainwindow_save;
	unsigned int perc_color;
	unsigned int perc_ascii;
	unsigned int hilightnick;
	unsigned int use_trans;
	unsigned int nopanel;
	unsigned int autosave;
	unsigned int autodialog;
	unsigned int autosave_url;
	unsigned int autoreconnect;
	unsigned int autoreconnectonfail;
	unsigned int invisible;
	unsigned int servernotice;
	unsigned int wallops;
	unsigned int skipmotd;
	unsigned int autorejoin;
	unsigned int colorednicks;
	unsigned int chanmodebuttons;
	unsigned int userlistbuttons;
	unsigned int userlist_icons;
	unsigned int showhostname_in_userlist;
	unsigned int nickcompletion;
	unsigned int old_nickcompletion;
	unsigned int tabchannels;
	unsigned int paned_userlist;
	unsigned int autodccchat;
	unsigned int autodccsend;
	unsigned int autoresume;
	unsigned int autoopendccsendwindow;
	unsigned int autoopendccrecvwindow;
	unsigned int autoopendccchatwindow;
	unsigned int transparent;
	unsigned int tint;
	unsigned int dialog_transparent;
	unsigned int dialog_tint;
	unsigned int dialog_width;
	unsigned int dialog_height;
	unsigned int stripcolor;
	unsigned int timestamp;
	unsigned int fastdccsend;
	unsigned int dcc_send_fillspaces;
	unsigned int skipserverlist;
	unsigned int filterbeep;
	unsigned int beepmsg;
	unsigned int beepchans;
	unsigned int privmsgtab;
	unsigned int logging;
	unsigned int timestamp_logs;
	unsigned int newtabstofront;
	unsigned int dccwithnick;
	unsigned int hilitenotify;
	unsigned int hidever;
	unsigned int ip_from_server;
	unsigned int panelize_hide;
	unsigned int panel_vbox;
	unsigned int raw_modes;
	unsigned int show_away_once;
	unsigned int show_away_message;
	unsigned int userhost;
	unsigned int use_server_tab;
	unsigned int notices_tabs;
	unsigned int style_namelistgad;
	unsigned int style_inputbox;
	unsigned int windows_as_tabs;
#ifdef USE_JCODE
	unsigned int kanji_conv;
#endif
	unsigned int use_fontset;
	unsigned int indent_nicks;
	unsigned int show_separator;
	unsigned int thin_separator;
	unsigned int dialog_indent_nicks;
	unsigned int dialog_show_separator;
	unsigned int treeview;
	unsigned int auto_indent;
	unsigned int wordwrap;
	unsigned int dialog_wordwrap;
	unsigned int mail_check;
	unsigned int throttle;
	unsigned int fudgeservernotice;
	unsigned int topicbar;
	unsigned int hideuserlist;
	unsigned int hidemenu;
	unsigned int perlwarnings;
	unsigned int lagometer;
	unsigned int throttlemeter;
	unsigned int hebrew;
	unsigned int limitedtabhighlight;
	unsigned int pingtimeout;
	unsigned int show_invite_in_front_session;
	unsigned int show_notify_in_front_session;
	unsigned int whois_on_notifyonline;
	unsigned int inputgad_superfocus;
	unsigned int nickgad;
	unsigned int channelbox;
	unsigned int persist_chans;

	unsigned int ctcp_number_limit;	/*flood */
	unsigned int ctcp_time_limit;	/*seconds of floods */

	unsigned int msg_number_limit;	/*same deal */
	unsigned int msg_time_limit;
};

/* Session types */
#define SESS_SERVER	1
#define SESS_CHANNEL	2
#define SESS_DIALOG	3
#define SESS_SHELL	4
#define SESS_NOTICES	5
#define SESS_SNOTICES	6

struct session
{
	struct server *server;
	GSList *userlist;
	char channel[CHANLEN];
	char waitchannel[CHANLEN];		  /* waiting to join this channel */
	char willjoinchannel[CHANLEN];	  /* /join done for this channel */
	char channelkey[64];			  /* XXX correct max length? */
	int limit;						  /* channel user limit */
	int logfd;

	char lastnick[NICKLEN];			  /* last nick you /msg'ed */

	struct history history;

	int ops;								/* num. of ops in channel */
	int hops;						  /* num. of half-oped users */
	int voices;							/* num. of voiced people */
	int total;							/* num. of users in channel */

	char *quitreason;
	char *topic;
	char *current_modes;					/* free() me */

	int mode_timeout_tag;

	struct session *lastlog_sess;
	struct setup *setup;
	struct nbexec *running_exec;

	struct session_gui *gui;		/* initialized by fe_new_window */

	int userlisthidden;
	int type;
	int is_tab:1;			/* is this a tab or toplevel window? */
	int new_data:1;		/* new data avail? (red tab) */
	int nick_said:1;		/* your nick mentioned? (blue tab) */
	int ignore_date:1;
	int ignore_mode:1;
	int ignore_names:1;
	int end_of_names:1;
	int doing_who:1;		/* /who sent on this channel */
	int highlight_tab:1;	/* Highlight the channel tab (red-style) */
};

typedef struct session session;

struct server
{
	int port;
	int sok;
	int sok4;
	int sok6;
#ifdef USE_OPENSSL
	SSL *ssl;
	int ssl_do_connect_tag;
	int use_ssl:1;					  /* is server SSL capable? */
	int accept_invalid_cert:1;	  /* ignore result of server's cert. verify */
#endif
	int childread;
	int childwrite;
	int childpid;
	int iotag;
	int bartag;
	int recondelay_tag;				/* reconnect delay timeout */
	char hostname[128];				/* real ip number */
	char servername[128];			/* what the server says is its name */
	char password[86];
	char nick[NICKLEN];
	char linebuf[522];				/* RFC says 512 including \r\n */
	char *last_away_reason;
	int pos;								/* current position in linebuf */
	int nickcount;

	char *chantypes;					/* for 005 numeric - free me */
	char *chanmodes;					/* for 005 numeric - free me */
	char *nick_prefixes;				/* e.g. "*@%+" */
	char *nick_modes;					/* e.g. "aohv" */
	char *bad_nick_prefixes;		/* for ircd that doesn't give the modes */

	char *eom_cmd;						/* end-of-motd command, free it! */

	GSList *outbound_queue;
	int next_send;						/* cptr->since in ircu */
	int sendq_len;						/* queue size */

	struct session *front_session;

	struct server_gui *gui;		  /* initialized by fe_new_server */

	unsigned int ctcp_counter;	  /*flood */
	time_t ctcp_last_time;

	unsigned int msg_counter;	  /*counts the msg tab opened in a certain time */
	time_t msg_last_time;

	/*time_t connect_time;*/				/* when did it connect? */
	time_t lag_sent;
	time_t ping_recv;					/* when we last got a ping reply */
	time_t away_time;					/* when we were marked away */

	int motd_skipped:1;
	int connected:1;
	int connecting:1;
	int no_login:1;
	int skip_next_who:1;			  /* used for "get my ip from server" */
	int inside_whois:1;
	int doing_who:1;				  /* /dns has been done */
	int end_of_motd:1;			  /* end of motd reached (logged in) */
	int sent_quit:1;				  /* sent a QUIT already? */
	int is_newtype:1;		/* undernet and dalnet need /list >0,<10000 */
	int six_modes:1;		/* undernet allows 6 modes per line */
	int is_away:1;
	int reconnect_away:1;		/* whether to reconnect in is_away state */
	int dont_use_proxy:1;		/* to proxy or not to proxy */
	int supports_watch:1;		/* supports the WATCH command */
	int bad_prefix:1;				/* gave us a bad PREFIX= 005 number */
};

typedef struct server server;

typedef int (*cmd_callback) (struct session * sess, char *tbuf, char *word[],
									  char *word_eol[]);

struct commands
{
	char *name;
	cmd_callback callback;
	char needserver;
	char needchannel;
	char *help;
};

struct away_msg
{
	struct server *server;
	char nick[NICKLEN];
	char *message;
};

/* not just for popups, but used for usercommands, ctcp replies,
   userlist buttons etc */

struct popup
{
	char *cmd;
	char *name;
};
