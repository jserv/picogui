/* 
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

/* **************EXPERIMENTAL*********************/

/* Python support by Adam Langley (agl@linuxpower.org) */

/* Welcome to the all singing, all dancing <crash> python support

   currently this is as stable as an elephant balenced on a bobby pin...

   current objects:
     XChat
     Server
     Session
     Data

   The first thing a script MUST do is import XChat. All the interface stems from this module.
   To start the ball rolling set XChat.XChat() to some variable. This is your interface.

   XChat:
     get_servers: returns a tuple of server objects
     get_sessions: returns a tuple of session object
     get_current_session: returns a session object
     hook_command(name, func): hooks the command @name and each this it is run it calls @func passing (String name, XChat.Session session, String Tuple args)
     register(name, desc): sets the script's name and description
     hook_signal(name, func): hooks the signal @name (e.g. "XP_TE_UJOIN") and calls @func everytime that signal fires passing (String name, Int flags, XChat.Data Tuple args)
       if the called function returns a non-zero integer, the signal won't be processed any further by xchat
     hook_timeout(msecs, func): one-shot timer, calls @func after @msecs milliseconds have passed
     info: returns a dict (see .keys())

   Server:
     send(data): writes @data raw to the server
     info: returns a dict (see .keys())
     set(host, port, nick): sets the server that the object acts on to the one matching @host, @port and @nick

   Session:
     print_text(text): Displays @text on the session
     handle_cmd(cmd): acts as if @cmd had been typed in, but without putting @cmd in the session history
     get_users: returns a dict in the form nick:(host, isop, isvoice) (just call it in interactive mode - you'll see what I mean)
     info: returns a dict (see .keys())
     set(host, port, nick, channel): sets the session the object refereres to, to the session matching @host, @port, @nick and @channel
     get_server: returns the server object of the session

   Data:
     get_string: returns a string from the data
     get_server: returns a server from the data
     get_session: returns a session from the data
     get_number: returns a int *of* the data


   That should be resonably basic, the only odd thing is the Data class. Why the hell is it there? Well when the python signal handler gets called it gets 5 void *'s and a char. The void *'s could point to anything, it changes from signal to signal. Rather than try to code in every signal's meta data (and make an ugly mess) it wraps the void *'s up into Data objects which are passed to your python signal handler in a tuple (3rd arg). It's then upto you what you do with the data. For example a possible signal handler for XP_INBOUND:

   def signalhandler(name, flags, args):
          session = args[0].get_session()
	  server = args[1].get_server()
	  message = args[2].get_string()

	  print "Message to " + session.info()['channel'] + " from " + server.info()['hostname'] + ": " + message

  It doesn't do a lot ;) but I hope you get the idea.

  The other wierd thing is the .set() calls to the Server and Session classes. Well XChat is free to kill a server or session struct anytime it likes. So if you keep a Server or Session object between calls then the actual object it acts on could be free()'ed. If you then call it XChat will go down harder than a 6 ton whale from 30,000 feet. So rather then keep the actual object just keep the host, port, nick (for the Server) and channel (Session only) and for each call do something like:

  sess = XChat.Session()
  try:
          sess.set (stored_host, stored_port, stored_nick, stored_channel)
  except LookupError:
          print "You Git! Give me back my session,  *NOW*!\n"

  you can get the data from:
    XChat.Sever.info()[x] with x = 'hostname', 'port' and 'nick'
    OR
    XChat.Session.info()[x] with x = 'servername', 'serverport', 'servernick' and 'channel'


    The function handlers follow this naming system:
      pysH_(E|D|S|)<function name>

      E = sess obj
      S = server obj
      D = data obj

      so a function to handle a session call of some_func() would be
      pysH_Esome_func
  Have fun!

  --AGL
*/

#include <stdio.h>

#include "xchat.h"
#include "plugin.h"
#include "stdlib.h"
#include "Python.h"
#include "pys_signals.h"
#include "fe.h"
#include "pythonc.h"
#include "text.h"
#include "util.h"
#include "cfgfiles.h"
#include "outbound.h"
#include "xchatc.h"

#define CHECK_SESS(x) if (self->sess == NULL) {\
PyErr_SetString (PyExc_AssertionError, "access attempt on NULL session object");\
return NULL;\
}

#define CHECK_SERVER(x) if (self->serv == NULL) {\
PyErr_SetString (PyExc_AssertionError, "access attempt on NULL server object");\
return NULL;\
}

typedef struct
{
	PyObject_HEAD PyThreadState * state;
	gchar *name;
	gchar *desc;
	GSList *cmd_hooks;
	GSList *sig_hooks;
	GSList *timeout_hooks;
}
xchat_obj;

typedef struct
{
	PyObject_HEAD struct server *serv;
}
server_obj;

typedef struct
{
	PyObject_HEAD struct session *sess;
}
sess_obj;

typedef struct
{
	PyObject_HEAD void *data;
}
data_obj;

typedef struct
{
	gchar *name;
	PyObject *cback;
	xchat_obj *xchat;
}
cmd_hook;

typedef struct
{
	gint iotag;
	PyObject *cback;
	xchat_obj *xchat;
}
timeout_hook;

typedef struct
{
	struct xp_signal *sig;
	xchat_obj *xchat;
	PyObject *cback;
}
sig_hook;

static void _xchat_dealloc (xchat_obj * self);
static PyObject *_xchat_getaddr (xchat_obj * self, char *name);
static void _server_dealloc (xchat_obj * self);
static PyObject *_server_getaddr (server_obj * self, char *name);
static void _sess_dealloc (xchat_obj * self);
static PyObject *_sess_getaddr (sess_obj * self, char *name);
static void _data_dealloc (xchat_obj * self);
static PyObject *_data_getaddr (sess_obj * self, char *name);

static GSList *pys_list;
static PyThreadState *pys_master = NULL;

static PyTypeObject XChat_Type = {
	PyObject_HEAD_INIT (NULL) 0, /* ob_size */
	"XChat",							  /* tp_name */
	sizeof (xchat_obj),			  /* tp_size */
	0,									  /* tp_itemsize */
	/* methods */
	(destructor) _xchat_dealloc, /* te_dealloc */
	0,
	(getattrfunc) _xchat_getaddr,
	0
};

static PyTypeObject Server_Type = {
	PyObject_HEAD_INIT (NULL) 0, /* ob_size */
	"Server",						  /* tp_name */
	sizeof (server_obj),			  /* tp_size */
	0,									  /* tp_itemsize */
	/* methods */
	(destructor) _server_dealloc,	/* te_dealloc */
	0,
	(getattrfunc) _server_getaddr,
	0
};

static PyTypeObject Sess_Type = {
	PyObject_HEAD_INIT (NULL) 0, /* ob_size */
	"Session",						  /* tp_name */
	sizeof (sess_obj),			  /* tp_size */
	0,									  /* tp_itemsize */
	/* methods */
	(destructor) _sess_dealloc,  /* te_dealloc */
	0,
	(getattrfunc) _sess_getaddr,
	0
};

static PyTypeObject Data_Type = {
	PyObject_HEAD_INIT (NULL) 0, /* ob_size */
	"Data",							  /* tp_name */
	sizeof (data_obj),			  /* tp_size */
	0,									  /* tp_itemsize */
	/* methods */
	(destructor) _data_dealloc,  /* te_dealloc */
	0,
	(getattrfunc) _data_getaddr,
	0
};

static PyObject *
_xchat_new ()
{
	xchat_obj *self;

	self = PyObject_NEW (xchat_obj, &XChat_Type);
	if (self == NULL)
		return NULL;

	self->state = PyThreadState_Get ();
	self->name = NULL;
	self->cmd_hooks = NULL;
	self->sig_hooks = NULL;
	self->timeout_hooks = NULL;

	pys_list = g_slist_prepend (pys_list, self);

	return (PyObject *) self;
}

static PyObject *
_server_new ()
{
	server_obj *self;

	self = PyObject_NEW (server_obj, &Server_Type);
	if (self == NULL)
		return NULL;


	self->serv = NULL;

	return (PyObject *) self;
}

static PyObject *
_sess_new ()
{
	sess_obj *self;

	self = PyObject_NEW (sess_obj, &Sess_Type);
	if (self == NULL)
		return NULL;

	self->sess = NULL;

	return (PyObject *) self;
}

static PyObject *
_data_new ()
{
	data_obj *self;

	self = PyObject_NEW (data_obj, &Data_Type);
	if (self == NULL)
		return NULL;

	self->data = NULL;

	return (PyObject *) self;
}

static void
_xchat_dealloc (xchat_obj * self)
{
	GSList *cur;
	cmd_hook *cmd;
	sig_hook *sh;
	timeout_hook *timeout;

	if (self->name)
		g_free (self->name);
	if (self->desc)
		g_free (self->desc);
	pys_list = g_slist_remove (pys_list, self);

	cur = self->cmd_hooks;
	while (cur)
	{
		cmd = cur->data;
		g_free (cmd->name);
		Py_XDECREF (cmd->cback);
		cur = g_slist_remove (cur, cmd);
		g_free (cmd);
	}

	cur = self->timeout_hooks;
	while (cur)
	{
		timeout = cur->data;
		fe_timeout_remove(timeout->iotag);
		Py_XDECREF (timeout->cback);
		cur = g_slist_remove (cur, timeout);
		g_free (timeout);
	}

	cur = self->sig_hooks;
	while (cur)
	{
		sh = cur->data;
		Py_XDECREF (sh->cback);
		unhook_signal (sh->sig);
		g_free (sh->sig);
		cur = g_slist_remove (cur, sh);
		g_free (sh);
	}

	PyMem_DEL (self);
}

static void
_server_dealloc (xchat_obj * self)
{
	PyMem_DEL (self);
}

static void
_sess_dealloc (xchat_obj * self)
{
	PyMem_DEL (self);
}

static void
_data_dealloc (xchat_obj * self)
{
	PyMem_DEL (self);
}

static PyObject *
_xchat_obj_op (PyObject * self, PyObject * args)
{
	if (!PyArg_NoArgs (args))
		return NULL;

	return _xchat_new ();
}

static PyObject *
_server_obj_op (PyObject * self, PyObject * args)
{
	if (!PyArg_NoArgs (args))
		return NULL;

	return _server_new ();
}

static PyObject *
_sess_obj_op (PyObject * self, PyObject * args)
{
	if (!PyArg_NoArgs (args))
		return NULL;

	return _sess_new ();
}

static PyObject *
_data_obj_op (PyObject * self, PyObject * args)
{
	PyErr_SetString (PyExc_StandardError, "attempt to make NULL Data object");
	return NULL;
}

int
find_signal_from_name (char *name)
{
	int c = 0;

	while (signal_mapping[c].name)
	{
		if (strcmp (name, signal_mapping[c].name) == 0)
			return signal_mapping[c].num;
		c++;
	}

	return -1;
}

char *
find_name_from_signal (int num)
{
	int c = 0;

	while (signal_mapping[c].name)
	{
		if (signal_mapping[c].num == num)
			return signal_mapping[c].name;
		c++;
	}

	return NULL;
}

int
pys_stock_signal_handler (void *a, void *b, void *c, void *d, void *e, char f)
{
	sig_hook *hook;
	PyObject *tuple, *data, *arglist, *retobj;
	char *name = find_name_from_signal (current_signal);
	int retval = 0;

	hook = signal_data;
	g_assert (hook);
	g_assert (name);

	tuple = PyTuple_New (5);

#define F(x,y) data = _data_new ();\
((data_obj *) data)->data = x;\
PyTuple_SetItem (tuple, y, data);
	F (a, 0);
	F (b, 1);
	F (c, 2);
	F (d, 3);
	F (e, 4);
#undef F

	arglist = Py_BuildValue ("(s,i,O)", name, f, tuple);

	PyThreadState_Swap (hook->xchat->state);
	retobj = PyEval_CallObject (hook->cback, arglist);
	if (PyErr_Occurred ())
		PyErr_Print ();

	Py_DECREF (tuple);
	Py_XDECREF (arglist);
	
	if (retobj) {
		if (retobj != Py_None && PyInt_Check(retobj) && PyInt_AsLong(retobj))
			retval = 1;
		Py_DECREF(retobj);
	}

	return retval;
}

static PyObject *
pysH_get_servers (xchat_obj * self, PyObject * args)
{
/* This returns a tuple of server classes,
   it takes 2 parses of the serv_list, one to count the number of
   servers and a second to fill out the tuple object --AGL */
	int c = 0;
	GSList *cur;
	PyObject *tuple;
	PyObject *server;

	cur = serv_list;
	while (cur)
	{
		c++;
		cur = cur->next;
	}
	tuple = PyTuple_New (c);

	c = 0;
	cur = serv_list;
	while (cur)
	{
		server = _server_new ();
		((server_obj *) server)->serv = cur->data;
		PyTuple_SetItem (tuple, c, server);
		cur = cur->next;
		c++;
	}

	return tuple;
}

static PyObject *
pysH_get_sessions (xchat_obj * self, PyObject * args)
{
/* This returns a tuple of session classes,
   it takes 2 parses of the sess_list, one to count the number of
   servers and a second to fill out the tuple object --AGL */
	int c = 0;
	GSList *cur;
	PyObject *tuple;
	PyObject *sess;

	cur = sess_list;
	while (cur)
	{
		c++;
		cur = cur->next;
	}
	tuple = PyTuple_New (c);

	c = 0;
	cur = sess_list;
	while (cur)
	{
		sess = _sess_new ();
		((server_obj *) sess)->serv = cur->data;
		PyTuple_SetItem (tuple, c, sess);
		cur = cur->next;
		c++;
	}

	return tuple;
}

cmd_hook *
find_hook (GSList * list, char *name)
{
	cmd_hook *hook;
	GSList *cur;

	cur = list;
	while (cur)
	{
		hook = cur->data;
		if (strcasecmp (name, hook->name) == 0)
			return hook;
		cur = cur->next;
	}

	return NULL;
}

static PyObject *
pysH_get_current_session (xchat_obj * self, PyObject * args)
{
	PyObject *obj;

	g_assert (current_tab);
	if (!PyArg_ParseTuple (args, ""))
		return NULL;

	obj = _sess_new ();
	((sess_obj *) obj)->sess = current_tab;

	return obj;
}

static PyObject *
pysH_hook_command (xchat_obj * self, PyObject * args)
{
	char *name;
	PyObject *cback;
	cmd_hook *hook;

	if (!PyArg_ParseTuple (args, "sO", &name, &cback))
		return NULL;

	if (!PyCallable_Check (cback))
	{
		PyErr_SetString (PyExc_TypeError, "parameter must be callable");
		return NULL;
	}

	Py_XINCREF (cback);
	hook = find_hook (self->cmd_hooks, name);
	if (hook)
	{
		g_free (hook->name);
		Py_XDECREF (hook->cback);
		g_free (hook);
		self->cmd_hooks = g_slist_remove (self->cmd_hooks, hook);
	}

	hook = g_new (cmd_hook, 1);
	hook->name = g_strdup (name);
	hook->cback = cback;
	hook->xchat = self;
	self->cmd_hooks = g_slist_prepend (self->cmd_hooks, hook);

	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *
pysH_Eprint_text (sess_obj * self, PyObject * args)
{
	char *out;

	CHECK_SESS (self);
	if (!PyArg_ParseTuple (args, "s", &out))
		return NULL;
	g_assert (self->sess);

	PrintText (self->sess, out);
	Py_XINCREF (Py_None);
	return Py_None;
}

static PyObject *
pysH_Ehandle_cmd (sess_obj * self, PyObject * args)
{
	char *out;

	CHECK_SESS (self);
	if (!PyArg_ParseTuple (args, "s", &out))
		return NULL;
	g_assert (self->sess);

	handle_command (out, self->sess, 0, 0);

	Py_XINCREF (Py_None);
	return Py_None;
}

static PyObject *
pysH_Ssend (server_obj * self, PyObject * args)
{
	char *out;

	CHECK_SERVER (self);
	if (!PyArg_ParseTuple (args, "s", &out))
		return NULL;
	g_assert (self->serv);

	tcp_send_len (self->serv, out, strlen (out));

	Py_XINCREF (Py_None);
	return Py_None;
}

static PyObject *
pysH_Sinfo (server_obj * self, PyObject * args)
{
	PyObject *map;

	CHECK_SERVER (self);
	if (!PyArg_ParseTuple (args, ""))
		return NULL;

	map = PyDict_New ();

#define F(x,y) PyDict_SetItemString (map, x, PyString_FromString(self->serv->y))
	F ("name", servername);
	F ("hostname", hostname);
	F ("password", password);
	F ("nick", nick);
#undef F
#define F(x, y) PyDict_SetItemString (map, x, PyInt_FromLong(self->serv->y))
	F ("port", port);
#undef F
	return map;
}

static PyObject *
pysH_Einfo (sess_obj * self, PyObject * args)
{
	PyObject *map;

	CHECK_SESS (self);
	if (!PyArg_ParseTuple (args, ""))
		return NULL;

	map = PyDict_New ();

#define F(x, y) PyDict_SetItemString (map, x, PyString_FromString(self->sess->y))
	F ("channel", channel);
	F ("waitchannel", waitchannel);
	F ("nick", server->nick);
	F ("willjoinchannel", willjoinchannel);
	F ("channelkey", channelkey);
#undef F
	/* Now for the numbers ... --AGL */
#define F(x, y) PyDict_SetItemString (map, x, PyInt_FromLong(self->sess->y))
	F ("limit", limit);
	F ("ops", ops);
	F ("total", total);
	F ("type", type);
	F ("is_shell", type == SESS_SHELL);
	F ("is_tab", is_tab);
	F ("is_dialog", type == SESS_DIALOG);
	F ("is_server", type == SESS_SERVER);
#undef F

	PyDict_SetItemString (map, "servername",
								 PyString_FromString (self->sess->server->hostname));
	PyDict_SetItemString (map, "serverport",
								 PyInt_FromLong (self->sess->server->port));
	PyDict_SetItemString (map, "servernick",
								 PyString_FromString (self->sess->server->nick));

	return map;
}

static PyObject *
pysH_Eget_users (sess_obj * self, PyObject * args)
{
	PyObject *map, *tuple;
	struct User *cur;
	GSList *list;

	g_assert (self);

	CHECK_SESS (self);
	if (!PyArg_ParseTuple (args, ""))
		return NULL;

	map = PyDict_New ();

	list = self->sess->userlist;
	while (list)
	{
		cur = (struct User *) list->data;
		tuple = PyTuple_New (3);
		if (cur->hostname)
			PyTuple_SetItem (tuple, 0, PyString_FromString (cur->hostname));
		else
			PyTuple_SetItem (tuple, 0, PyString_FromString ("FETCHING"));
		PyTuple_SetItem (tuple, 1, PyInt_FromLong (cur->op));
		PyTuple_SetItem (tuple, 2, PyInt_FromLong (cur->voice));
		PyDict_SetItemString (map, cur->nick, tuple);

		list = list->next;
	}

	return map;
}

static PyObject *
pysH_register (xchat_obj * self, PyObject * args)
{
	char *name;
	char *desc;

	if (!PyArg_ParseTuple (args, "ss", &name, &desc))
		return NULL;

	g_assert (name);
	g_assert (desc);
	self->name = g_strdup (name);
	self->desc = g_strdup (desc);

	Py_XINCREF (Py_None);
	return Py_None;
}

static int
python_timeout (timeout_hook* hook)
{

	PyObject *arglist, *retobj;
	arglist = Py_BuildValue ("()");

	PyThreadState_Swap (hook->xchat->state);
	retobj = PyEval_CallObject (hook->cback, arglist);
	if (PyErr_Occurred ())
		PyErr_Print ();

	Py_XDECREF (arglist);
	Py_XDECREF (retobj);

	Py_XDECREF (hook->cback);
	hook->xchat->timeout_hooks = g_slist_remove (hook->xchat->timeout_hooks, hook);
	g_free(hook);

	return 0;
}

static PyObject *
pysH_hook_timeout (xchat_obj * self, PyObject * args)
{
	PyObject *cback;
	long timeout;
	timeout_hook *hook;

	if (!PyArg_ParseTuple (args, "lO", &timeout, &cback))
		return NULL;

	if (!PyCallable_Check (cback))
	{
		PyErr_SetString (PyExc_TypeError, "parameter must be callable");
		return NULL;
	}

	Py_XINCREF (cback);

	hook = g_new0 (timeout_hook, 1);
	hook->xchat = self;
	hook->cback = cback;
	hook->iotag = fe_timeout_add(timeout, python_timeout, hook);
	self->timeout_hooks = g_slist_prepend (self->timeout_hooks, hook);

	Py_XINCREF (Py_None);
	return Py_None;
}

static PyObject *
pysH_hook_signal (xchat_obj * self, PyObject * args)
{
	char *name;
	PyObject *cback;
	int signum;
	sig_hook *sh;

	if (!PyArg_ParseTuple (args, "sO", &name, &cback))
		return NULL;

	if (!PyCallable_Check (cback))
	{
		PyErr_SetString (PyExc_TypeError, "parameter must be callable");
		return NULL;
	}

	signum = find_signal_from_name (name);
	if (signum == -1)
	{
		PyErr_SetString (PyExc_LookupError, "unknown signal name");
		return NULL;
	}

	Py_XINCREF (cback);

	sh = g_new0 (sig_hook, 1);
	sh->xchat = self;
	sh->cback = cback;
	sh->sig = g_new0 (struct xp_signal, 1);
	sh->sig->signal = signum;
	sh->sig->data = sh;
	sh->sig->callback = pys_stock_signal_handler;
	hook_signal (sh->sig);
	self->sig_hooks = g_slist_prepend (self->sig_hooks, sh);

	Py_XINCREF (Py_None);
	return Py_None;
}

static PyObject *
pysH_Dget_string (data_obj * self, PyObject * args)
{
	if (!PyArg_ParseTuple (args, ""))
		return NULL;

	return PyString_FromString ((char *) self->data);
}

static PyObject *
pysH_Dget_session (data_obj * self, PyObject * args)
{
	PyObject *sess;

	if (!PyArg_ParseTuple (args, ""))
		return NULL;

	sess = _sess_new ();
	((sess_obj *) sess)->sess = self->data;

	return sess;
}

static PyObject *
pysH_Dget_server (data_obj * self, PyObject * args)
{
	PyObject *serv;

	if (!PyArg_ParseTuple (args, ""))
		return NULL;

	serv = _server_new ();
	((server_obj *) serv)->serv = self->data;

	return serv;
}

static PyObject *
pysH_Dget_number (data_obj * self, PyObject * args)
{
	int num;

	g_assert (sizeof (int) == sizeof (void *));

	if (!PyArg_ParseTuple (args, ""))
		return NULL;

	memcpy (&num, &self->data, sizeof (int));
	return PyInt_FromLong (num);
}

PyObject *
pysH_Eset (sess_obj * self, PyObject * args)
{
	char *host, *chan, *nick;
	int port;
	GSList *cur;
	struct session *sess;

	if (!PyArg_ParseTuple (args, "siss", &host, &port, &nick, &chan))
		return NULL;

	cur = sess_list;
	while (cur)
	{
		sess = cur->data;
		if (sess->server->port == port &&
			 strcasecmp (sess->server->hostname, host) == 0 &&
			 strcasecmp (sess->server->nick, nick) == 0 &&
			 strcasecmp (sess->channel, chan) == 0)
		{
			self->sess = sess;
			break;
		}
		cur = cur->next;
	}

	if (!cur)
	{
		PyErr_SetString (PyExc_LookupError, "unknown session");
		return NULL;
	}

	Py_XINCREF (Py_None);
	return Py_None;
}

static PyObject *
pysH_Sset (server_obj * self, PyObject * args)
{
	char *host, *nick;
	int port;
	GSList *cur;
	struct server *serv;

	if (!PyArg_ParseTuple (args, "sis", &host, &port, &nick))
		return NULL;

	cur = serv_list;
	while (cur)
	{
		serv = cur->data;

		if (serv->port == port &&
			 strcasecmp (serv->hostname, host) == 0 &&
			 strcasecmp (serv->nick, nick) == 0)
		{
			self->serv = serv;
			break;
		}

		cur = cur->next;
	}
	if (!cur)
	{
		PyErr_SetString (PyExc_LookupError, "unknown server");
		return NULL;
	}

	Py_XINCREF (Py_None);
	return Py_None;
}

static PyObject *
pysH_Eget_server (sess_obj * self, PyObject * args)
{
	PyObject *serv;

	CHECK_SESS (self);
	if (!PyArg_ParseTuple (args, ""))
		return NULL;

	serv = _server_new ();
	((server_obj *) serv)->serv = self->sess->server;

	return serv;
}

static PyObject *
pysH_info (xchat_obj * self, PyObject * args)
{
	PyObject *map;

	if (!PyArg_ParseTuple (args, ""))
		return NULL;

	map = PyDict_New ();

#define F(x,y) PyDict_SetItemString (map, x, PyString_FromString(y))
	F ("version", VERSION);
#undef F

#ifdef USE_GDKPIXBUF
	PyDict_SetItemString (map, "Gdk-Pixbuf", PyString_FromString ("yes"));
#else
	PyDict_SetItemString (map, "Gdk-Pixbuf", PyString_FromString ("no"));
#endif

#ifdef USE_GNOME
	PyDict_SetItemString (map, "GNOME", PyString_FromString ("yes"));
#else
	PyDict_SetItemString (map, "GNOME", PyString_FromString ("no"));
#endif

#ifdef USE_PERL
	PyDict_SetItemString (map, "Perl", PyString_FromString ("yes"));
#else
	PyDict_SetItemString (map, "Perl", PyString_FromString ("no"));
#endif

#ifdef USE_PLUGIN
	PyDict_SetItemString (map, "Plugin", PyString_FromString ("yes"));
#else
	PyDict_SetItemString (map, "Plugin", PyString_FromString ("no"));
#endif

#ifdef ENABLE_NLS
	PyDict_SetItemString (map, "NLS", PyString_FromString ("yes"));
#else
	PyDict_SetItemString (map, "NLS", PyString_FromString ("no"));
#endif

#ifdef SOCKS
	PyDict_SetItemString (map, "Socks", PyString_FromString ("yes"));
#else
	PyDict_SetItemString (map, "Socks", PyString_FromString ("no"));
#endif

	return map;
}

static PyMethodDef _xchat_methods[] = {
	{"get_servers", (PyCFunction) pysH_get_servers, METH_VARARGS},
	{"get_sessions", (PyCFunction) pysH_get_sessions, METH_VARARGS},
	
		{"get_current_session", (PyCFunction) pysH_get_current_session,
	 METH_VARARGS},
	{"hook_command", (PyCFunction) pysH_hook_command, METH_VARARGS},
	{"register", (PyCFunction) pysH_register, METH_VARARGS},
	{"hook_signal", (PyCFunction) pysH_hook_signal, METH_VARARGS},
	{"hook_timeout", (PyCFunction) pysH_hook_timeout, METH_VARARGS},
	{"info", (PyCFunction) pysH_info, METH_VARARGS},
	{NULL, NULL}
};

static PyMethodDef _server_methods[] = {
	{"send", (PyCFunction) pysH_Ssend, METH_VARARGS},
	{"info", (PyCFunction) pysH_Sinfo, METH_VARARGS},
	{"set", (PyCFunction) pysH_Sset, METH_VARARGS},
	{NULL, NULL}
};

static PyMethodDef _sess_methods[] = {
	{"print_text", (PyCFunction) pysH_Eprint_text, METH_VARARGS},
	{"handle_cmd", (PyCFunction) pysH_Ehandle_cmd, METH_VARARGS},
	{"get_users", (PyCFunction) pysH_Eget_users, METH_VARARGS},
	{"info", (PyCFunction) pysH_Einfo, METH_VARARGS},
	{"set", (PyCFunction) pysH_Eset, METH_VARARGS},
	{"get_server", (PyCFunction) pysH_Eget_server, METH_VARARGS},
	{NULL, NULL}
};

static PyMethodDef _data_methods[] = {
	{"get_string", (PyCFunction) pysH_Dget_string, METH_VARARGS},
	{"get_server", (PyCFunction) pysH_Dget_server, METH_VARARGS},
	{"get_session", (PyCFunction) pysH_Dget_session, METH_VARARGS},
	{"get_number", (PyCFunction) pysH_Dget_number, METH_VARARGS},
	{NULL, NULL}
};

static PyObject *
_server_getaddr (server_obj * self, char *name)
{
	return Py_FindMethod (_server_methods, (PyObject *) self, name);
}

static PyObject *
_xchat_getaddr (xchat_obj * self, char *name)
{
	return Py_FindMethod (_xchat_methods, (PyObject *) self, name);
}

static PyObject *
_sess_getaddr (sess_obj * self, char *name)
{
	return Py_FindMethod (_sess_methods, (PyObject *) self, name);
}

static PyObject *
_data_getaddr (sess_obj * self, char *name)
{
	return Py_FindMethod (_data_methods, (PyObject *) self, name);
}

static PyMethodDef _classes[] = {
	{"XChat", _xchat_obj_op, 0},
	{"Server", _server_obj_op, 0},
	{"Session", _sess_obj_op, 0},
	{"Data", _data_obj_op, 0},
	{NULL, NULL}
};

static void
pys_autoloadfile (char *file)
{
	char *words[3] = {"", "", file};
	pys_load(NULL, NULL, words, NULL);
}

void
pys_init ()
{
	pys_list = NULL;

	Py_SetProgramName ("xchat");
	Py_Initialize ();

	XChat_Type.ob_type = &PyType_Type;
	Server_Type.ob_type = &PyType_Type;

	Py_InitModule ("XChat", _classes);
	if (PyErr_Occurred ())
		g_assert_not_reached ();

	pys_master = Py_NewInterpreter ();
	Py_InitModule ("XChat", _classes);

	for_files (get_xdir (), "*.py", pys_autoloadfile);
}

int
pys_pkill (struct session *sess, char *tbuf, char **word, char **word_eol)
{
	GSList *cur;
	xchat_obj *obj;

	if (!word[2] || word[2][0] == 0)
		return FALSE;

	cur = pys_list;
	while (cur)
	{
		obj = cur->data;
		if (obj->name)
		{
			if (strcmp (obj->name, word[2]) == 0)
				break;
		}
		cur = cur->next;
	}
	if (!cur)
	{
		PrintText (sess, "No such python script\n");
		return FALSE;
	}

	PyThreadState_Swap (obj->state);
	Py_EndInterpreter (obj->state);
	PyThreadState_Swap (pys_master);

	return TRUE;
}

int
pys_plist (struct session *sess, char *tbuf, char **word, char **word_eol)
{
	GSList *cur;
	xchat_obj *obj;
	gchar *buf;

	cur = pys_list;
	while (cur)
	{
		obj = cur->data;
		buf = g_strdup_printf ("%s: %s\n", obj->name, obj->desc);
		PrintText (sess, buf);
		g_free (buf);

		cur = cur->next;
	}

	return TRUE;
}

int
pys_cmd_handle (char *cmd, struct session *sess, char **word)
{
	GSList *cur;
	xchat_obj *xo;
	cmd_hook *hook = NULL;
	sess_obj *sess_o;
	PyObject *tuple, *arglist, *retobj;
	int c, x, y;

	cur = pys_list;
	while (cur)
	{
		xo = cur->data;
		hook = find_hook (xo->cmd_hooks, cmd);
		if (hook)
			break;
		cur = cur->next;
	}
	if (!hook)
		return 0;

	sess_o = (sess_obj *) _sess_new ();
	sess_o->sess = sess;

	/* Count the number of args */
	c = 1;
	while (word[c] && word[c][0] != 0)
		c++;
	c--;

	tuple = PyTuple_New (c);
	c++;
	for (x = 1, y = 0; x < c; x++, y++)
		PyTuple_SetItem (tuple, y, PyString_FromString (word[x]));

	arglist = Py_BuildValue ("(s,O,O)", cmd, sess_o, tuple);

	PyThreadState_Swap(hook->xchat->state);
	retobj = PyEval_CallObject (hook->cback, arglist);
	if (PyErr_Occurred ())
		PyErr_Print ();

	Py_XDECREF (arglist);
	Py_XDECREF (sess_o);
	Py_XDECREF (tuple);
	Py_XDECREF (retobj);

	return TRUE;
}

/* Note: this is also called from pys_autoloadfile() with funky
   arguments, be careful */
int
pys_load (struct session *sess, char *tbuf, char **word, char **word_eol)
{
	FILE *in;
	PyThreadState *tstate;

	if (!word[2] || word[2][0] == 0)
		return FALSE;
	if (strcmp (word[2], "DEBUG") == 0)
	{
		printf ("*** Python code - running interactive loop...\n");
		in = fopen ("test.py", "r");
		g_assert (in);
		PyRun_InteractiveLoop (in, "test.py");
		return TRUE;
	}

	in = fopen (word[2], "r");
	if (!in)
	{
		PrintText (current_tab,
					  "\017Python script file not found or could not be opened\n");
		return FALSE;
	}
	if ((tstate = Py_NewInterpreter ()) == NULL)
	{
		PrintText (current_tab, "\017Python interpreter could not be created\n");
		fclose(in);
		return TRUE;
	}
	Py_InitModule ("XChat", _classes);
	if (PyErr_Occurred ())
	{
		PrintText (current_tab,
					  "\017Python interpreter failed to import XChat\n");
		fclose(in);
		return TRUE;
	}
	PyRun_SimpleFile (in, word[2]);
	if (PyErr_Occurred ())
	{
		PrintText (current_tab,
					  "\017Python interpreter failed to load script\n");
		Py_EndInterpreter (tstate);
		fclose(in);
		return TRUE;
	}

	fclose(in);
	return TRUE;
}

void
pys_kill ()
{
	PyThreadState_Swap (pys_master);
	Py_Finalize ();
}
