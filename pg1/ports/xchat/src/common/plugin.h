
#ifndef PLUGIN_H
#define PLUGIN_H

#define MODULE_IFACE_VER	2
#define XP_CALLBACK(x)	( (int (*) (void *, void *, void *, void *, void *, char) ) x )

enum
{ XP_USERCOMMAND =
		0, XP_PRIVMSG, XP_CHANACTION, XP_CHANMSG, XP_CHANGENICK, XP_JOIN,
		XP_CHANSETKEY, XP_CHANSETLIMIT, XP_CHANHOP, XP_CHANOP, XP_CHANVOICE,
		XP_CHANBAN, XP_CHANRMKEY, XP_CHANRMLIMIT, XP_CHANDEHOP, XP_CHANDEOP,
		XP_CHANDEVOICE, XP_CHANUNBAN, XP_CHANEXEMPT, XP_CHANRMEXEMPT,
		XP_CHANINVITE, XP_CHANRMINVITE, XP_INBOUND, XP_HIGHLIGHT,
		XP_TE_JOIN, XP_TE_CHANACTION, XP_TE_HCHANACTION, XP_TE_CHANMSG,
		XP_TE_HCHANMSG, XP_TE_PRIVMSG, XP_TE_CHANGENICK,
		XP_TE_NEWTOPIC, XP_TE_TOPIC, XP_TE_KICK, XP_TE_PART, XP_TE_CHANDATE,
		XP_TE_TOPICDATE, XP_TE_QUIT, XP_TE_PINGREP, XP_TE_NOTICE, XP_TE_UJOIN,
		XP_TE_UCHANMSG, XP_TE_DPRIVMSG, XP_TE_UCHANGENICK, XP_TE_UKICK,
		XP_TE_UPART, XP_TE_CTCPSND, XP_TE_CTCPGEN, XP_TE_CTCPGENC,
		XP_TE_CHANSETKEY, XP_TE_CHANSETLIMIT, XP_TE_CHANHOP, XP_TE_CHANOP,
		XP_TE_CHANVOICE, XP_TE_CHANBAN, XP_TE_CHANRMKEY, XP_TE_CHANRMLIMIT,
		XP_TE_CHANDEHOP, XP_TE_CHANDEOP, XP_TE_CHANDEVOICE, XP_TE_CHANUNBAN,
		XP_TE_CHANEXEMPT, XP_TE_CHANRMEXEMPT, XP_TE_CHANINVITE,
		XP_TE_CHANRMINVITE, XP_TE_CHANMODEGEN, XP_TE_WHOIS1, XP_TE_WHOIS2,
		XP_TE_WHOIS3, XP_TE_WHOIS4, XP_TE_WHOIS4T, XP_TE_WHOIS5, XP_TE_WHOIS6,
		XP_TE_USERLIMIT, XP_TE_BANNED, XP_TE_INVITE, XP_TE_KEYWORD,
		XP_TE_MOTDSKIP, XP_TE_SERVTEXT, XP_TE_INVITED, XP_TE_USERSONCHAN,
		XP_TE_NICKCLASH, XP_TE_NICKFAIL, XP_TE_UKNHOST, XP_TE_CONNFAIL,
		XP_TE_CONNECT, XP_TE_CONNECTED, XP_TE_SCONNECT, XP_TE_DISCON,
		XP_TE_NODCC, XP_TE_DELNOTIFY, XP_TE_ADDNOTIFY,
		XP_TE_CHANMODES, XP_TE_RAWMODES, XP_TE_KILL, XP_TE_DCCSTALL,
		XP_TE_DCCTOUT, XP_TE_DCCCHATF, XP_TE_DCCFILEERR, XP_TE_DCCRECVERR,
		XP_TE_DCCRECVCOMP, XP_TE_DCCCONFAIL, XP_TE_DCCCON, XP_TE_DCCSENDFAIL,
		XP_TE_DCCSENDCOMP, XP_TE_DCCOFFER, XP_TE_DCCABORT, XP_TE_DCCIVAL,
		XP_TE_DCCCHATREOFFER, XP_TE_DCCCHATOFFERING,
		XP_TE_DCCCHATOFFER, XP_TE_DCCRESUMEREQUEST, XP_TE_DCCSENDOFFER,
		XP_TE_DCCGENERICOFFER, XP_TE_NOTIFYONLINE, XP_TE_NOTIFYNUMBER,
		XP_TE_NOTIFYEMPTY, XP_TE_NOCHILD, XP_TE_ALREADYPROCESS,
		XP_TE_SERVERLOOKUP, XP_TE_SERVERCONNECTED, XP_TE_SERVERERROR,
		XP_TE_SERVERGENMESSAGE, XP_TE_FOUNDIP, XP_TE_DCCRENAME, XP_TE_CTCPSEND,
		XP_TE_MSGSEND, XP_TE_NOTICESEND, XP_TE_WALLOPS,
		XP_TE_IGNOREHEADER, XP_TE_IGNORELIST, XP_TE_IGNOREFOOTER,
		XP_TE_IGNOREADD, XP_TE_IGNOREREMOVE, XP_TE_RESOLVINGUSER,
		XP_TE_IGNOREEMPTY, XP_TE_IGNORECHANGE, XP_TE_NOTIFYOFFLINE,
		XP_TE_MALFORMED_FROM, XP_TE_MALFORMED_PACKET, XP_TE_PARTREASON,
		XP_TE_UPARTREASON, XP_TE_NEWMAIL, XP_TE_MOTD, XP_TE_PINGTIMEOUT,
		XP_TE_UINVITE, XP_TE_BANLIST, XP_TE_CHANLISTHEAD, XP_TE_NOTIFYHEAD,
		XP_TE_DCCHEAD,
		XP_IF_SEND, XP_IF_RECV, XP_TE_CHANNOTICE, 
		NUM_XP
};

#define	EMIT_SIGNAL(s, a, b, c, d, e, f) (fire_signal(s, a, b, c, d, e, f))
/* #define XP_CALLNEXT(s, a, b, c, d, e, f)  if (s != NULL) return s(a, b, c, d, e, f); return 0; */
/* #define XP_CALLNEXT_ANDSET(s, a, b, c, d, e, f) if (s != NULL) s(a, b, c, d, e, f); return 1; */

#define XP_CALLNEXT(s, a, b, c, d, e, f) return 0;
#define XP_CALLNEXT_ANDSET(s, a, b, c, d, e, f) return 1;


#ifdef USE_PLUGIN

struct module
{
	void *handle;
	char *name, *desc;
	struct module *next, *last;
};

struct module_cmd_set
{
	struct module *mod;
	struct commands *cmds;
	struct module_cmd_set *next, *last;
};

#endif

struct xp_signal
{
	int signal;
	int (**naddr) (void *, void *, void *, void *, void *, char);
	int (*callback) (void *, void *, void *, void *, void *, char);
	/* These aren't used, but needed to keep compatibility --AGL */
	void *next, *last;
	void *data;
#ifdef USE_PLUGIN
	struct module *mod;
#else
	void *padding;
#endif
};

struct pevt_stage1
{
	int len;
	char *data;
	struct pevt_stage1 *next;
};

#ifndef PLUGIN_C
int fire_signal (int, void *, void *, void *, void *, void *, char);
#endif

extern int current_signal;
extern void *signal_data;
extern GSList *sigroots[NUM_XP];
extern struct module *modules;

int module_command (char *cmd, struct session *sess, char *tbuf,
									char *word[], char *word_eol[]);
int module_load (char *name, struct session *sess);
int module_list (struct session *sess, char *tbuf, char *word[],
								char *word_eol[]);
int module_unload (char *name, struct session *sess);
void module_setup (void);
void signal_setup (void);
void unhook_signal (struct xp_signal *);
int hook_signal (struct xp_signal *);

#endif /* PLUGIN_H */
