#define USE_PLUGIN

#include <stdio.h>
#include "../../src/common/xchat.h"
#include "../../src/common/plugin.h"

extern	struct module *module_find (char *name);

int sample_cmd (struct session *sess, char *tbuf, char *word[], char *word_eol[]);
int sample_privmsg (struct server *serv, char *from, char *ip, char *text, void *a, char c);
int sample_test (struct session *sess, char *a, char *b, char *c, char *d, char e);
int sample_test2 (struct session *sess, char *a, char *b, char *c, char *d,  char e);

char	*name = "Sample";
char	*desc = "This is a sample module of no use!";

struct	commands	sample_cmds[] = {
	{"SAMPLE",	sample_cmd,	0, 0, "/SAMPLE <msg>"},
	{0, 0, 0, 0, 0}
};

struct	xp_signal	privmsg_sig, test_sig, test2_sig;
int	(*test_next)	(void *, void *, void *, void *, void *, char);
int	(*test2_next)	(void *, void *, void *, void *, void *, char);
int	(*privmsg_next)	(void *, void *, void *, void *, void *, char);

struct	module_cmd_set	sample_cmd_set;

int	module_init (int ver, struct module *mod, struct session *sess)
{
	/* This check *MUST* be done first */
	if (ver != MODULE_IFACE_VER)
		return 1;
	
	if (module_find (name) != NULL) {
		/* We are already loaded */
		PrintText(sess, "Module sample already loaded\n");
		return 1;
	}
	PrintText(sess, "Loaded module sample\n");
	mod->name = name;
	mod->desc = desc;
		
	privmsg_sig.signal = XP_PRIVMSG;
	privmsg_sig.callback = XP_CALLBACK(sample_privmsg);
	privmsg_sig.naddr = &privmsg_next;
	privmsg_sig.mod = mod;
	
	test_sig.signal = XP_USERCOMMAND;
	test_sig.callback = XP_CALLBACK(sample_test);
	test_sig.naddr = &test_next;
	test_sig.mod = mod;
	
	test2_sig.signal = XP_USERCOMMAND;
	test2_sig.callback = XP_CALLBACK(sample_test2);
	test2_sig.naddr = &test2_next;
	test2_sig.mod = mod;
	
	hook_signal(&test_sig);
	hook_signal(&test2_sig);
	hook_signal(&privmsg_sig);

	sample_cmd_set.mod = mod;
	sample_cmd_set.cmds = sample_cmds;
	
	module_add_cmds (&sample_cmd_set);
	
	return 0;
}

void	module_cleanup (struct module *mod, struct session *sess)
{
	PrintText(sess, "Sample module unloading\n");
}

int	sample_privmsg (struct server *serv, char *from, char *ip, char *text, void *a, char c)
{
	struct	session	*sess;
	char	buf[512];
	
	sess = serv->front_session;
	snprintf(buf, 510, "PRIVMSG from: %s, ip: %s, text: %s\n", from, ip, text);
	PrintText(sess, buf);
	
	XP_CALLNEXT(privmsg_next, serv, from, ip, text, a, c);
}

int	sample_test (struct session *sess, char *a, char *b, char *c, char *d, char e)
{
	printf("CMD: %s\n", a);
	
	XP_CALLNEXT(test_next, sess, a, b, c, d, e);
}

int	sample_test2 (struct session *sess, char *a, char *b, char *c, char *d,  char e)
{
	printf("CMD2: %s\n", a);
	
	XP_CALLNEXT(test2_next, sess, a, b, c, d, e);
}

int	sample_cmd (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	PrintText(sess, "Sample cmd!\n");
	return 0;
}
