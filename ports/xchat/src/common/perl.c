/* X-Chat
 * Copyright (C) 1998 Peter Zelezny.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

/* perl.c by Erik Scrafford <eriks@chilisoft.com>. */

#include "../../config.h"
#undef PACKAGE

#include <EXTERN.h>
#ifndef _SEM_SEMUN_UNDEFINED
#define HAS_UNION_SEMUN
#endif
#define __G_WIN32_H__
#define WIN32IOP_H
#include <perl.h>
#include <XSUB.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#undef PACKAGE
#include <stdlib.h>
#include <stdio.h>
#include "xchat.h"
#include "cfgfiles.h"
#include "util.h"
#include "ignore.h"
#include "notify.h"
#include "fe.h"
#include "text.h"
#include "outbound.h"
#include "xchatc.h"
#include "perlc.h"


struct _perl_timeout_handlers
{
	char *handler_name;
	gint iotag;
};

struct _perl_inbound_handlers
{
	char *message_type;
	char *handler_name;
};

struct _perl_command_handlers
{
	char *command_name;
	char *handler_name;
};

struct _perl_print_handlers
{
	char *print_name;
	char *handler_name;
};

struct perlscript
{
	char *name;
	char *version;
	char *shutdowncallback;
};

/* external values needed to access preferences via IRC::get_prefs*/
extern struct prefs vars[];	  /*from cfgfiles.c */

static PerlInterpreter *my_perl = NULL;
static session *perl_sess = NULL;

/* these must be initialized to 0 incase cmd_scpinfo is
   called before perl is initialized */
static GSList *perl_timeout_handlers = 0;
static GSList *perl_inbound_handlers = 0;
static GSList *perl_command_handlers = 0;
static GSList *perl_print_handlers = 0;
static GSList *perl_list = 0;

static XS (XS_IRC_register);
static XS (XS_IRC_add_message_handler);
static XS (XS_IRC_add_command_handler);
static XS (XS_IRC_add_print_handler);
static XS (XS_IRC_add_timeout_handler);
static XS (XS_IRC_print);
static XS (XS_IRC_print_with_channel);
static XS (XS_IRC_send_raw);
static XS (XS_IRC_command);
static XS (XS_IRC_command_with_server);
static XS (XS_IRC_channel_list);
static XS (XS_IRC_server_list);
static XS (XS_IRC_add_user_list);
static XS (XS_IRC_sub_user_list);
static XS (XS_IRC_clear_user_list);
static XS (XS_IRC_user_list);
static XS (XS_IRC_user_info);
static XS (XS_IRC_ignore_list);
static XS (XS_IRC_notify_list);
static XS (XS_IRC_dcc_list);
static XS (XS_IRC_get_info);
static XS (XS_IRC_get_prefs);
static XS (XS_IRC_user_list_short);
static XS (XS_IRC_perl_script_list);

#ifdef OLD_PERL
extern void boot_DynaLoader _((CV * cv));
#else
extern void boot_DynaLoader (pTHX_ CV* cv);
#endif

/* xs_init is the second argument perl_pars. As the name hints, it
   initializes XS subroutines (see the perlembed manpage) */
static void
#ifdef OLD_PERL
xs_init ()
#else
xs_init (pTHX)
#endif
{
	char *file = __FILE__;

	/* This one allows dynamic loading of perl modules in perl
	   scripts by the 'use perlmod;' construction*/
	newXS ("DynaLoader::boot_DynaLoader", boot_DynaLoader, file);
	/* load up all the custom IRC perl functions */
	/* deplaced here from perl_init function (TheHobbit)*/
	newXS ("IRC::register", XS_IRC_register, "IRC");
	newXS ("IRC::add_message_handler", XS_IRC_add_message_handler, "IRC");
	newXS ("IRC::add_command_handler", XS_IRC_add_command_handler, "IRC");
	newXS ("IRC::add_print_handler", XS_IRC_add_print_handler, "IRC");
	newXS ("IRC::add_timeout_handler", XS_IRC_add_timeout_handler, "IRC");
	newXS ("IRC::print", XS_IRC_print, "IRC");
	newXS ("IRC::print_with_channel", XS_IRC_print_with_channel, "IRC");
	newXS ("IRC::send_raw", XS_IRC_send_raw, "IRC");
	newXS ("IRC::command", XS_IRC_command, "IRC");
	newXS ("IRC::command_with_server", XS_IRC_command_with_server, "IRC");
	newXS ("IRC::channel_list", XS_IRC_channel_list, "IRC");
	newXS ("IRC::server_list", XS_IRC_server_list, "IRC");
	newXS ("IRC::add_user_list", XS_IRC_add_user_list, "IRC");
	newXS ("IRC::sub_user_list", XS_IRC_sub_user_list, "IRC");
	newXS ("IRC::clear_user_list", XS_IRC_clear_user_list, "IRC");
	newXS ("IRC::user_list", XS_IRC_user_list, "IRC");
	newXS ("IRC::user_info", XS_IRC_user_info, "IRC");
	newXS ("IRC::ignore_list", XS_IRC_ignore_list, "IRC");
	newXS ("IRC::notify_list", XS_IRC_notify_list, "IRC");
	newXS ("IRC::dcc_list", XS_IRC_dcc_list, "IRC");
	newXS ("IRC::get_info", XS_IRC_get_info, "IRC");
	newXS ("IRC::get_prefs", XS_IRC_get_prefs, "IRC");
	newXS ("IRC::user_list_short", XS_IRC_user_list_short, "IRC");
	newXS ("IRC::perl_script_list", XS_IRC_perl_script_list, "IRC");
}

/* a session is being killed, does it affect us? */

void
perl_notify_kill (session * sess)
{
	struct session *s;
	GSList *list = sess_list;

	if (perl_sess == sess)		  /* need to find a new perl_sess, this one's closing */
	{
		while (list)
		{
			s = (struct session *) list->data;
			if (s->server == perl_sess->server && s != perl_sess)
			{
				perl_sess = s;
				break;
			}
			list = list->next;
		}
		if (perl_sess == sess)
			perl_sess = 0;
	}
}

/* list some script information (handlers etc) */

int
cmd_scpinfo (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	GSList *handler;

	PrintText (sess, _("Registered Scripts:\n"));
	handler = perl_list;
	while (handler)
	{
		struct perlscript *scp = handler->data;
		sprintf (tbuf, "  %s %s\n", scp->name, scp->version);
		PrintText (sess, tbuf);
		handler = handler->next;
	}

	PrintText (sess, _("Inbound Handlers:\n"));
	for (handler = perl_inbound_handlers; handler != NULL;
		  handler = handler->next)
	{
		struct _perl_inbound_handlers *data = handler->data;
		sprintf (tbuf, "  %s\n", data->message_type);
		PrintText (sess, tbuf);
	}

	PrintText (sess, _("Command Handlers:\n"));
	for (handler = perl_command_handlers; handler != NULL;
		  handler = handler->next)
	{
		struct _perl_command_handlers *data = handler->data;
		sprintf (tbuf, "  %s\n", data->command_name);
		PrintText (sess, tbuf);
	}

	PrintText (sess, _("Print Handlers:\n"));
	for (handler = perl_print_handlers; handler != NULL;
		  handler = handler->next)
	{
		struct _perl_print_handlers *data = handler->data;
		sprintf (tbuf, "  %s\n", data->print_name);
		PrintText (sess, tbuf);
	}

	return TRUE;
}

/* 
   execute_perl is modified in order to avoid crashing of xchat when a
   perl error occours. The embedded interpreter will instead print the
   error message using IRC::print and return 1 to stop futher
   processing of the event.

   patch by TheHobbit <thehobbit@altern.org>

*/

/*
  2001/06/14: execute_perl replaced by Martin Persson <mep@passagen.se>
	      previous use of perl_eval leaked memory, replaced with
	      a version that uses perl_call instead
*/
static int
execute_perl (char *function, char *args)
{
	char *perl_args[2] = { args, NULL }, buf[512];
	int count, ret_value = 1;
	SV *sv;

	dSP;
	ENTER;
	SAVETMPS;
	PUSHMARK(sp);
	count = perl_call_argv(function, G_EVAL | G_SCALAR, perl_args);
	SPAGAIN;

	sv = GvSV(gv_fetchpv("@", TRUE, SVt_PV));
	if (SvTRUE(sv)) {
		snprintf(buf, 512, "Perl error: %s\n", SvPV(sv, count));
		PrintText(perl_sess, buf);
		POPs;
	} else if (count != 1) {
		snprintf(buf, 512, "Perl error: expected 1 value from %s, "
			"got: %d\n", function, count);
		PrintText(perl_sess, buf);
	} else {
		ret_value = POPi;
	}

	PUTBACK;
	FREETMPS;
	LEAVE;

	return ret_value;
}

static void
perl_init (void)
{
	/*changed the name of the variable from load_file to
	  perl_definitions since now it does much more than defining
	  the load_file sub. Moreover, deplaced the initialisation to
	  the xs_init function. (TheHobbit)*/
	char *perl_args[] = { "", "-e", "0", "-w" };
	char perl_definitions[] =
	{
	  /* We use to function one to load a file the other to
	     execute the string obtained from the first and holding
	     the file conents. This allows to have a realy local $/
	     without introducing temp variables to hold the old
	     value. Just a question of style:) 
	     We also redefine the $SIG{__WARN__} handler to have XChat
	     printing warnings in the main window. (TheHobbit)*/
	  "sub load_file{"
	    "my $file_name=shift;"
	    "local $/=undef;"
	    "open FH,$file_name or return \"__FAILED__\";"
	    "$_=<FH>;"
	    "close FH;"
	    "return $_;"
	  "}"
	  "sub load_n_eval{"
	    "my $file_name=shift;"
	    "my $strin=load_file($file_name);"
	    "return 2 if($strin eq \"__FAILED__\");"
	    "eval $strin;"
	    "if($@){"
	    /*"  #something went wrong\n"*/
	      "IRC::print \"Errors loading file $file_name:\\n\";"
	      "IRC::print \"$@\\n\";"
	      "return 1;"
	    "}"
	    "return 0;"
	  "}"
	  "$SIG{__WARN__}=sub{IRC::print\"$_[0]\n\";};"
	};
#ifdef ENABLE_NLS

	/* Problem is, dynamicaly loaded modules check out the $]
	   var. It appears that in the embedded interpreter we get
	   5,00503 as soon as the LC_NUMERIC locale calls for a comma
	   instead of a point in separating integer and decimal
	   parts. I realy can't understant why... The following
	   appears to be an awful workaround... But it'll do until I
	   (or someone else :)) found the "right way" to solve this
	   nasty problem. (TheHobbit <thehobbit@altern.org>)*/
	
 	setlocale(LC_NUMERIC,"C"); 
	
#endif

	my_perl = perl_alloc ();
	perl_construct (my_perl);
	if (prefs.perlwarnings)
		perl_parse (my_perl, xs_init, 4, perl_args, NULL);
	else
		perl_parse (my_perl, xs_init, 3, perl_args, NULL);
	/*
	  Now initialising the perl interpreter by loading the
	  perl_definition array.
	*/
#ifdef HAVE_EVAL_PV
	eval_pv (perl_definitions, TRUE);
#else
	perl_eval_pv (perl_definitions, TRUE);	/* deprecated */
#endif
	/*perl_timeout_handlers = 0;
	perl_inbound_handlers = 0;
	perl_command_handlers = 0;
	perl_print_handlers = 0;
	perl_list = 0;*/
}

/*
  To avoid problems, the load_n_eval sub must be executed directly
  without going into a supplementary eval.

  TheHobbit <thehobbit@altern.org>
*/
int
perl_load_file (char *script_name)
{
	if (my_perl == NULL)
		perl_init ();

	return execute_perl ("load_n_eval", script_name);
}

static void
perl_autoload_file (char *script_name)
{
	perl_load_file (script_name);
}

void
perl_auto_load (session *sess)
{
	perl_sess = sess;
	for_files (get_xdir (), "*.pl", perl_autoload_file);
}

int
cmd_unloadall (session *sess, char *tbuf, char *word[], char *word_eol[])
{
	perl_sess = sess;
	perl_end ();
	return TRUE;
}

void
perl_end (void)
{
	struct perlscript *scp;
	struct _perl_command_handlers *chand;
	struct _perl_inbound_handlers *ihand;
	struct _perl_print_handlers *phand;
	struct _perl_timeout_handlers *thand;

	while (perl_list)
	{
		scp = perl_list->data;
		perl_list = g_slist_remove (perl_list, scp);
		if (scp->shutdowncallback[0])
			execute_perl (scp->shutdowncallback, "");
		free (scp->name);
		free (scp->version);
		free (scp->shutdowncallback);
		free (scp);
	}

	while (perl_command_handlers)
	{
		chand = perl_command_handlers->data;
		perl_command_handlers = g_slist_remove (perl_command_handlers, chand);
		free (chand->command_name);
		free (chand->handler_name);
		free (chand);
	}

	while (perl_print_handlers)
	{
		phand = perl_print_handlers->data;
		perl_print_handlers = g_slist_remove (perl_print_handlers, phand);
		free (phand->print_name);
		free (phand->handler_name);
		free (phand);
	}

	while (perl_inbound_handlers)
	{
		ihand = perl_inbound_handlers->data;
		perl_inbound_handlers = g_slist_remove (perl_inbound_handlers, ihand);
		free (ihand->message_type);
		free (ihand->handler_name);
		free (ihand);
	}

	while (perl_timeout_handlers)
	{
		thand = perl_timeout_handlers->data;
		perl_timeout_handlers = g_slist_remove (perl_timeout_handlers, thand);
		fe_timeout_remove (thand->iotag);
		free (thand->handler_name);
		free (thand);
	}

	if (my_perl != NULL)
	{
		perl_destruct (my_perl);
		perl_free (my_perl);
		my_perl = NULL;
	}
}

int
perl_inbound (struct session *sess, struct server *serv, char *buf, char *msg_type)
{
	GSList *handler;
	struct _perl_inbound_handlers *data;
	int handler_return;

	perl_sess = sess;

	for (handler = perl_inbound_handlers; handler != NULL;
		  handler = handler->next)
	{
		data = handler->data;
		if (!strcmp (msg_type, data->message_type)
			 || !strcmp ("INBOUND", data->message_type))
		{
			handler_return = execute_perl (data->handler_name, buf);
			if (handler_return)
			{
				return handler_return;
			}
		}
	}
	return 0;
}

int
perl_dcc_chat (struct session *sess, struct server *serv, char *buf)
{
	GSList *handler;
	struct _perl_inbound_handlers *data;
	int handler_return;

	if (!buf)
		return 0;
	perl_sess = sess;

	for (handler = perl_inbound_handlers; handler != NULL;
		  handler = handler->next)
	{
		data = handler->data;
		if (!strcmp ("DCC", data->message_type)
			 || !strcmp ("INBOUND", data->message_type))
		{
			handler_return = execute_perl (data->handler_name, buf);
			if (handler_return)
			{
				return handler_return;
			}
		}
	}
	return 0;
}

int
perl_command (char *cmd, struct session *sess)
{
	GSList *handler;
	struct _perl_command_handlers *data;
	char *command_name;
	char *tmp;
	char *args;
	char nullargs[] = "";
	int handler_return;
	int command = FALSE;

	args = NULL;
	perl_sess = sess;
	if (*cmd == '/')
	{
		cmd++;
		command = TRUE;
	}
	command_name = strdup (cmd);
	tmp = strchr (command_name, ' ');
	if (tmp)
	{
		*tmp = 0;
		args = ++tmp;
	}
	if (!args)
		args = nullargs;

	for (handler = perl_command_handlers; handler != NULL;
		  handler = handler->next)
	{
		data = handler->data;
		if (
			 ((!strcasecmp (command_name, data->command_name)) && command)
			 || (!command && data->command_name[0] == 0))
		{
			if (!command)
				handler_return = execute_perl (data->handler_name, cmd);
			else
				handler_return = execute_perl (data->handler_name, args);
			if (handler_return)
			{
				free (command_name);
				return handler_return;
			}
		}
	}

	free (command_name);
	return 0;
}

int
perl_print (char *cmd, struct session *sess, char *b, char *c, char *d,
				char *e)
{
	GSList *handler;
	struct _perl_print_handlers *data;
	char *args;
	int handler_return;

	if (!perl_print_handlers)
		return 0;

	args = malloc (1);
	*args = 0;
	perl_sess = sess;

	if (b)
	{
		args = realloc (args, strlen (args) + strlen (b) + 2);
		strcat (args, " ");
		strcat (args, b);
	}
	if (c)
	{
		args = realloc (args, strlen (args) + strlen (c) + 2);
		strcat (args, " ");
		strcat (args, c);
	}
	if (d)
	{
		args = realloc (args, strlen (args) + strlen (d) + 2);
		strcat (args, " ");
		strcat (args, d);
	}
	if (e)
	{
		args = realloc (args, strlen (args) + strlen (e) + 2);
		strcat (args, " ");
		strcat (args, e);
	}

	for (handler = perl_print_handlers; handler != NULL;
		  handler = handler->next)
	{
		data = handler->data;
		if (!strcasecmp (cmd, data->print_name))
		{
			handler_return = execute_perl (data->handler_name, args);
			if (handler_return)
			{
				free (args);
				return handler_return;
			}
		}
	}

	free (args);
	return 0;
}

static int
perl_timeout (struct _perl_timeout_handlers *handler)
{
	if (perl_sess && !is_session (perl_sess))	/* sanity check */
		perl_sess = menu_sess;

	execute_perl (handler->handler_name, "");
	perl_timeout_handlers = g_slist_remove (perl_timeout_handlers, handler);
	free (handler->handler_name);
	free (handler);

	return 0;						  /* returning zero removes the timeout handler */
}

/* custom IRC perl functions for scripting */

/* IRC::register (scriptname, version, shutdowncallback, unused)

 *  all scripts should call this at startup
 *
 */

static XS (XS_IRC_register)
{
	char *name, *ver, *callback, *unused;
	int junk;
	struct perlscript *scp;
	dXSARGS;

	name = SvPV (ST (0), junk);
	ver = SvPV (ST (1), junk);
	callback = SvPV (ST (2), junk);
	unused = SvPV (ST (3), junk);

	scp = malloc (sizeof (struct perlscript));
	scp->name = strdup (name);
	scp->version = strdup (ver);
	scp->shutdowncallback = strdup (callback);
	perl_list = g_slist_prepend (perl_list, scp);

	XST_mPV (0, VERSION);
	XSRETURN (1);
}


/* print to main window */
/* IRC::main_print(output) */
static XS (XS_IRC_print)
{
	int junk;
	int i;
	char *output;
	dXSARGS;

	/*if (perl_sess)
	   { */
	for (i = 0; i < items; ++i)
	{
		output = SvPV (ST (i), junk);
		PrintText (perl_sess, output);
	}
	/*} */

	XSRETURN_EMPTY;
}

/*
 * IRC::print_with_channel( text, channelname, servername )
 *    
 *   The servername is optional, channelname is required.
 *   Returns 1 on success, 0 on fail.
 */

static XS (XS_IRC_print_with_channel)
{
	int junk;
	char *output;
	struct session *sess;
	GSList *list = sess_list;
	char *channel, *server;
	dXSARGS;

	output = SvPV (ST (0), junk);
	channel = SvPV (ST (1), junk);
	server = SvPV (ST (2), junk);

	while (list)
	{
		sess = (struct session *) list->data;
		if (!server || !server[0]
			 || !strcasecmp (server, sess->server->servername))
		{
			if (sess->channel[0])
			{
				if (!strcasecmp (sess->channel, channel))
				{
					PrintText (sess, output);
					XSRETURN_YES;
				}
			}
		}
		list = list->next;
	}

	XSRETURN_NO;
}

static XS (XS_IRC_get_info)
{
	dXSARGS;

	if (!perl_sess)
	{
		XST_mPV (0, "Error1");
		XSRETURN (1);
	}
	switch (SvIV (ST (0)))
	{
	case 0:
		XST_mPV (0, VERSION);
		break;

	case 1:
		XST_mPV (0, perl_sess->server->nick);
		break;

	case 2:
		XST_mPV (0, perl_sess->channel);
		break;

	case 3:
		XST_mPV (0, perl_sess->server->servername);
		break;

	case 4:
		XST_mPV (0, get_xdir ());
		break;

	case 5:
		XST_mIV (0, perl_sess->server->is_away);
		break;

	default:
		XST_mPV (0, "Error2");
	}

	XSRETURN (1);
}

/* Added by TheHobbit <thehobbit@altern.org>*/
/* IRC::get_prefs(var) */
static XS (XS_IRC_get_prefs)
{
	int junk;
	char *var;
	int i = 0;
	dXSARGS;

	var = SvPV (ST (0), junk);

	do
	{
		if (!strcasecmp (var, vars[i].name))
		{
			switch (vars[i].type)
			{
			case TYPE_STR:
				XST_mPV (0, (char *) &prefs + vars[i].offset);
				break;
			case TYPE_INT:
				XST_mIV (0, *((int *) &prefs + vars[i].offset));
				break;
			default:
			/*case TYPE_BOOL:*/
				if (*((int *) &prefs + vars[i].offset))
				{
					XST_mYES (0);
				} else
				{
					XST_mNO (0);
				}
				break;
			}
			XSRETURN (1);
		}
		i++;
	}
	while (vars[i].type != 0);
	XST_mPV (0, "Unknown variable");
	XSRETURN (1);
}

/* add handler for messages with message_type(ie PRIVMSG, 400, etc) */
/* IRC::add_message_handler(message_type, handler_name) */
static XS (XS_IRC_add_message_handler)
{
	int junk;
	struct _perl_inbound_handlers *handler;
	dXSARGS;

	handler = malloc (sizeof (struct _perl_inbound_handlers));
	handler->message_type = strdup (SvPV (ST (0), junk));
	handler->handler_name = strdup (SvPV (ST (1), junk));
	perl_inbound_handlers = g_slist_prepend (perl_inbound_handlers, handler);
	XSRETURN_EMPTY;
}

/* add handler for commands with command_name */
/* IRC::add_command_handler(command_name, handler_name) */
static XS (XS_IRC_add_command_handler)
{
	int junk;
	struct _perl_command_handlers *handler;
	dXSARGS;

	handler = malloc (sizeof (struct _perl_command_handlers));
	handler->command_name = strdup (SvPV (ST (0), junk));
	handler->handler_name = strdup (SvPV (ST (1), junk));
	perl_command_handlers = g_slist_prepend (perl_command_handlers, handler);
	XSRETURN_EMPTY;
}

/* add handler for commands with print_name */
/* IRC::add_print_handler(print_name, handler_name) */
static XS (XS_IRC_add_print_handler)
{
	int junk;
	struct _perl_print_handlers *handler;
	dXSARGS;

	handler = malloc (sizeof (struct _perl_print_handlers));
	handler->print_name = strdup (SvPV (ST (0), junk));
	handler->handler_name = strdup (SvPV (ST (1), junk));
	perl_print_handlers = g_slist_prepend (perl_print_handlers, handler);
	XSRETURN_EMPTY;
}

static XS (XS_IRC_add_timeout_handler)
{
	int junk;
	struct _perl_timeout_handlers *handler;
	dXSARGS;

	handler = malloc (sizeof (struct _perl_timeout_handlers));
	handler->handler_name = strdup (SvPV (ST (1), junk));
	perl_timeout_handlers = g_slist_prepend (perl_timeout_handlers, handler);
	handler->iotag = fe_timeout_add (SvIV (ST (0)), perl_timeout, handler);
	XSRETURN_EMPTY;
}

/* send raw data to server */
/* IRC::send_raw(data) */
static XS (XS_IRC_send_raw)
{
	char *data;
	int junk;
	dXSARGS;

	if (perl_sess)
	{
		data = strdup (SvPV (ST (0), junk));
		tcp_send (perl_sess->server, data);
		free (data);
	}
	XSRETURN_EMPTY;
}

static XS (XS_IRC_channel_list)
{
	struct session *sess;
	GSList *list = sess_list;
	int i = 0;
	dXSARGS;

	while (list)
	{
		sess = (struct session *) list->data;
		if (sess->channel[0])
		{
			XST_mPV (i, sess->channel);
			i++;
			XST_mPV (i, sess->server->servername);
			i++;
			XST_mPV (i, sess->server->nick);
			i++;
		}
		list = list->next;
	}

	XSRETURN (i);
}

static XS (XS_IRC_server_list)
{
	server *serv;
	GSList *list = serv_list;
	int i = 0;
	dXSARGS;

	while (list)
	{
		serv = list->data;
		if (serv->connected && serv->end_of_motd)
		{
			XST_mPV (i, serv->servername);
			i++;
		}
		list = list->next;
	}

	XSRETURN (i);
}

static XS (XS_IRC_ignore_list)
{
	struct ignore *ig;
	GSList *list = ignore_list;
	int i = 0;
	dXSARGS;

	while (list)
	{
		ig = (struct ignore *) list->data;

		XST_mPV (i, ig->mask);
		i++;
		XST_mIV (i, ig->priv);
		i++;
		XST_mIV (i, ig->chan);
		i++;
		XST_mIV (i, ig->ctcp);
		i++;
		XST_mIV (i, ig->noti);
		i++;
		XST_mIV (i, ig->invi);
		i++;
		XST_mIV (i, ig->unignore);
		i++;
		XST_mPV (i, ":");
		i++;

		list = list->next;
	}
	XSRETURN (i);
}

static XS (XS_IRC_notify_list)
{
	struct notify *not;
	struct notify_per_server *notserv;
	GSList *list = notify_list;
	GSList *notslist;
	int i = 0;
	dXSARGS;

	while (list)
	{
		not = (struct notify *) list->data;
		notslist = not->server_list;

		XST_mPV (i, not->name);
		i++;
		while (notslist)
		{
			notserv = (struct notify_per_server *)notslist->data;

			XST_mPV (i, notserv->server->servername);
			i++;
			XST_mIV (i, notserv->laston);
			i++;
			XST_mIV (i, notserv->lastseen);
			i++;
			XST_mIV (i, notserv->lastoff);
			i++;
			if (notserv->ison)
				XST_mYES (i);
			else
				XST_mNO (i);
			i++;
			XST_mPV (i, "::");
			i++;
			
			notslist = notslist->next;
		}
		XST_mPV (i, ":");
		i++;

		list = list->next;
	}
	
	XSRETURN (i);
}


/*

   IRC::user_info( nickname )

 */

static XS (XS_IRC_user_info)
{
	int junk;
	struct User *user;
	char *nick;
	dXSARGS;

	if (perl_sess)
	{
		nick = SvPV (ST (0), junk);
		if (nick[0] == 0)
			nick = perl_sess->server->nick;
		user = find_name (perl_sess, nick);
		if (user)
		{
			XST_mPV (0, user->nick);
			if (user->hostname)
				XST_mPV (1, user->hostname);
			else
				XST_mPV (1, "FETCHING");
			XST_mIV (2, user->op);
			XST_mIV (3, user->voice);
			XSRETURN (4);
		}
	}
	XSRETURN (0);
}

/*
 * IRC::add_user_list(ul_channel, ul_server, nick, user_host,
 * 		      realname, server)
 */
static XS (XS_IRC_add_user_list)
{
	int junk;
	char *ul_channel;
	char *ul_server;
	char *nick;
	char *user_host;	/* add_name() wants user and host merged */
	char *realname;
	char *server;
	struct session *sess;
	GSList *list = sess_list;
	dXSARGS;

	ul_channel = SvPV(ST(0), junk);
	ul_server  = SvPV(ST(1), junk);
	nick       = SvPV(ST(2), junk);
	user_host  = SvPV(ST(3), junk);
	realname   = SvPV(ST(4), junk);
	server     = SvPV(ST(5), junk);

	while (list) {
		sess = (struct session *) list->data;
		if (!server[0] || !strcasecmp(sess->server->servername, ul_server)) {
			if (!strcasecmp(sess->channel, ul_channel)) {
				add_name(sess, nick, NULL);
				userlist_add_hostname(sess, nick, user_host,
						      realname, server);
				XSRETURN_YES;
			}
		}
		list = list->next;
	}

	XSRETURN_NO;
}

/*
 * IRC::sub_user_list(ul_channel, ul_server, nick)
 */
static XS (XS_IRC_sub_user_list)
{
	int junk;
	char *channel;
	char *server;
	char *nick;
	struct session *sess;
	GSList *list = sess_list;
	dXSARGS;

	channel = SvPV(ST(0), junk);
	server  = SvPV(ST(1), junk);
	nick    = SvPV(ST(2), junk);

	while (list) {
		sess = (struct session *) list->data;
		if (!server[0] || !strcasecmp(sess->server->servername, server)) {
			if (!strcasecmp(sess->channel, channel)) {
				sub_name(sess, nick);
				XSRETURN_YES;
			}
		}
		list = list->next;
	}

	XSRETURN_NO;
}

/*
 * IRC::clear_user_list(channel, server)
 */
static XS (XS_IRC_clear_user_list)
{
	int junk;
	char *channel, *server;
	struct session *sess;
	GSList *list = sess_list;
	dXSARGS;

	channel = SvPV(ST(0), junk);
	server  = SvPV(ST(1), junk);

	while (list) {
		sess = (struct session *) list->data;
		if (!server[0] || !strcasecmp(sess->server->servername, server)) {
			if (!strcasecmp(sess->channel, channel)) {
				clear_user_list(sess);
				XSRETURN_YES;
			}
		}
		list = list->next;
	}

	XSRETURN_NO;
};

/*

   IRC::user_list( channel, server )

 */

static XS (XS_IRC_user_list)
{
	struct User *user;
	struct session *sess;
	char *channel, *server;
	GSList *list = sess_list;
	int i = 0, junk;
	int MinStk = 10;
	dXSARGS;

	EXTEND (SP, MinStk);

	channel = SvPV (ST (0), junk);
	server = SvPV (ST (1), junk);

	while (list)
	{
		sess = (struct session *) list->data;
		if (!server[0] || !strcasecmp (sess->server->servername, server))
		{
			if (!strcasecmp (sess->channel, channel) && sess->type == SESS_CHANNEL)
			{
				list = sess->userlist;
				while (list)
				{
					user = (struct User *) list->data;
					XST_mPV (i, user->nick);
					i++;
					if (user->hostname)
						XST_mPV (i, user->hostname);
					else
						XST_mPV (i, "FETCHING");
					i++;
					XST_mIV (i, user->op);
					i++;
					XST_mIV (i, user->voice);
					i++;
					XST_mPV (i, ":");
					i++;
					list = list->next;
					/* Make sure there is room on the stack */
					MinStk = i + 10;
					EXTEND(SP, MinStk);
				}
				XSRETURN (i);
			}
		}
		list = list->next;
	}
	XSRETURN (i);
}

static XS (XS_IRC_dcc_list)
{
	struct DCC *dcc;
	GSList *list = dcc_list;
	int i = 0;
	dXSARGS;

	while (list)
	{
		dcc = (struct DCC *) list->data;
		XST_mPV (i, dcc->nick);
		i++;
		if (dcc->file)
			XST_mPV (i, dcc->file);
		else
			XST_mPV (i, "");
		i++;
		XST_mIV (i, dcc->type);
		i++;
		XST_mIV (i, dcc->dccstat);
		i++;
		XST_mIV (i, dcc->cps);
		i++;
		XST_mIV (i, dcc->size);
		i++;
		XST_mIV (i, dcc->resumable);
		i++;
		XST_mIV (i, dcc->addr);
		i++;
		if (dcc->destfile)
			XST_mPV (i, dcc->destfile);
		else
			XST_mPV (i, "");
		i++;
		list = list->next;
	}

	XSRETURN (i);
}

/* run internal xchat command */
/* IRC::command(command) */
static XS (XS_IRC_command)
{
	char *command;
	int junk;
	dXSARGS;

	if (perl_sess)
	{
		command = strdup (SvPV (ST (0), junk));
		handle_command (command, perl_sess, FALSE, FALSE);
		free (command);
	}
	XSRETURN_EMPTY;
}

static XS (XS_IRC_command_with_server)
{
	GSList *list = serv_list;
	struct server *serv;
	char *server, *command;
	int junk;
	dXSARGS;

	server = strdup (SvPV (ST (1), junk));

	while (list)
	{
		serv = (struct server *) list->data;
		if (!strcmp (serv->servername, server))
		{
			command = strdup (SvPV (ST (0), junk));
			if (!serv->front_session)
			{
				struct session *sess;
				GSList *list = sess_list;
				/*fprintf(stderr, "*** Perl Error: no front_session\n"); */
				while (list)
				{
					sess = (struct session *) list->data;
					if (sess->server == serv)
					{
						/*fprintf(stderr, "*** Using %lx instead\n", (unsigned long)sess); */
						handle_command (command, sess, FALSE, FALSE);
						break;
					}
					list = list->next;
				}
			} else
				handle_command (command, serv->front_session, FALSE, FALSE);
			free (command);
			free (server);
			XSRETURN_EMPTY;
		}
		list = list->next;
	}

	free (server);
	XSRETURN_EMPTY;
}

/* MAG030600: BEGIN IRC::user_list_short */
/*

   IRC::user_list_short( channel, server )
   returns a shorter user list consisting of pairs of nick & user@host
   suitable for assigning to a hash.  modified IRC::user_list()
   
 */
static XS (XS_IRC_user_list_short)
{
	struct User *user;
	struct session *sess;
	char *channel, *server;
	GSList *list = sess_list;
	int i = 0, junk;
	int MinStk = 10;
	dXSARGS;

	EXTEND (SP, MinStk);

	channel = SvPV (ST (0), junk);
	server = SvPV (ST (1), junk);

	while (list)
	{
		sess = (struct session *) list->data;
		if (!server[0] || !strcasecmp (sess->server->servername, server))
		{
			if (!strcasecmp (sess->channel, channel) && sess->type == SESS_CHANNEL)
			{
				list = sess->userlist;
				while (list)
				{
					user = (struct User *) list->data;
					XST_mPV (i, user->nick);
					i++;
					if (user->hostname)
						XST_mPV (i, user->hostname);
					else
						XST_mPV (i, "FETCHING");
					i++;
					list = list->next;
					/* Make sure there is room on the stack */
					MinStk = i + 10;
					EXTEND(SP, MinStk);
				}
				XSRETURN (i);
			}
		}
		list = list->next;
	}
	XSRETURN (i);
}
/* MAG030600: END */


/* MAG030600 BEGIN IRC::perl_script_list() */
/* return a list of currently loaded perl script names and versions */
static XS (XS_IRC_perl_script_list)
{
	int i = 0;
	GSList *handler;
	dXSARGS;

	handler = perl_list;
	while (handler)
	{
		struct perlscript *scp = handler->data;
		XST_mPV (i, scp->name);
		i++;
		XST_mPV (i, scp->version);
		i++;
		handler = handler->next;
	}
	XSRETURN(i);
}
/* MAG030600 END */
