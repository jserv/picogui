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

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "xchat.h"
#include "cfgfiles.h"
#include "util.h"
#include "fe.h"
#include "text.h"
#include "xchatc.h"
#ifdef USE_JCODE
#include <locale.h>
#endif

#define DEF_FONT "-b&h-lucidatypewriter-medium-r-normal-*-*-120-*-*-m-*-*-*"
#define DEF_FONT_JCODE "-*-fixed-medium-r-normal-*-14-*-*-*-c-*-*-*"
#define DEF_FONT_WIN32 "-*-Fixedsys-normal-r-normal-*-15-*-*-*-m-*-iso8859-1"

void
list_addentry (GSList ** list, char *cmd, char *name)
{
	struct popup *pop;
	int cmd_len = 1, name_len;

	if (cmd)
		cmd_len = strlen (cmd) + 1;
	name_len = strlen (name) + 1;

	pop = malloc (sizeof (struct popup) + cmd_len + name_len);
	pop->name = (char *) pop + sizeof (struct popup);
	pop->cmd = pop->name + name_len;

	memcpy (pop->name, name, name_len);
	if (cmd)
		memcpy (pop->cmd, cmd, cmd_len);
	else
		pop->cmd[0] = 0;

	*list = g_slist_append (*list, pop);
}

void
list_loadconf (char *file, GSList ** list, char *defaultconf)
{
	char filebuf[256];
	char cmd[256];
	char name[82];
	char *buf, *ibuf;
	int fh, pnt = 0;
	struct stat st;

	snprintf (filebuf, sizeof (filebuf), "%s/%s", get_xdir (), file);
	fh = open (filebuf, O_RDONLY | OFLAGS);
	if (fh == -1)
	{
		fh = open (filebuf, O_TRUNC | O_WRONLY | O_CREAT | OFLAGS, 0600);
		if (fh != -1)
		{
			write (fh, defaultconf, strlen (defaultconf));
			close (fh);
			list_loadconf (file, list, defaultconf);
		}
		return;
	}
	if (fstat (fh, &st) != 0)
	{
		perror ("fstat");
		abort ();
	}

	/*fstat (fh, &st);*/
	ibuf = malloc (st.st_size);
	read (fh, ibuf, st.st_size);
	close (fh);

	cmd[0] = 0;
	name[0] = 0;

	while (buf_get_line (ibuf, &buf, &pnt, st.st_size))
	{
		if (*buf != '#')
		{
			if (!strncasecmp (buf, "NAME ", 5))
			{
				safe_strcpy (name, buf + 5, sizeof (name));
			}
			if (!strncasecmp (buf, "CMD ", 4))
			{
				safe_strcpy (cmd, buf + 4, sizeof (cmd));
				if (*name)
				{
					list_addentry (list, cmd, name);
					cmd[0] = 0;
					name[0] = 0;
				}
			}
		}
	}
	free (ibuf);
}

void
list_free (GSList ** list)
{
	void *data;
	while (*list)
	{
		data = (void *) (*list)->data;
		free (data);
		*list = g_slist_remove (*list, data);
	}
}

int
list_delentry (GSList ** list, char *name)
{
	struct popup *pop;
	GSList *alist = *list;

	while (alist)
	{
		pop = (struct popup *) alist->data;
		if (!strcasecmp (name, pop->name))
		{
			*list = g_slist_remove (*list, pop);
			free (pop);
			return 1;
		}
		alist = alist->next;
	}
	return 0;
}

char *
cfg_get_str (char *cfg, char *var, char *dest)
{
	while (1)
	{
		if (!strncasecmp (var, cfg, strlen (var)))
		{
			char *value, t;
			cfg += strlen (var);
			while (*cfg == ' ')
				cfg++;
			if (*cfg == '=')
				cfg++;
			while (*cfg == ' ')
				cfg++;
			/*while (*cfg == ' ' || *cfg == '=')
			   cfg++; */
			value = cfg;
			while (*cfg != 0 && *cfg != '\n')
				cfg++;
			t = *cfg;
			*cfg = 0;
			strcpy (dest, value);
			*cfg = t;
			return cfg;
		}
		while (*cfg != 0 && *cfg != '\n')
			cfg++;
		if (*cfg == 0)
			return 0;
		cfg++;
		if (*cfg == 0)
			return 0;
	}
}

static void
cfg_put_str (int fh, char *var, char *value)
{
	char buf[512];

	snprintf (buf, sizeof buf, "%s = %s\n", var, value);
	write (fh, buf, strlen (buf));
}

void
cfg_put_int (int fh, int value, char *var)
{
	char buf[400];

	if (value == -1)
		value = 1;

	snprintf (buf, sizeof buf, "%s = %d\n", var, value);
	write (fh, buf, strlen (buf));
}

int
cfg_get_int_with_result (char *cfg, char *var, int *result)
{
	char str[128];

	if (!cfg_get_str (cfg, var, str))
	{
		*result = 0;
		return 0;
	}

	*result = 1;
	return atoi (str);
}

int
cfg_get_int (char *cfg, char *var)
{
	char str[128];

	if (!cfg_get_str (cfg, var, str))
		return 0;

	return atoi (str);
}

char *xdir = 0;

char *
get_xdir (void)
{
#ifndef WIN32
	if (!xdir)
	{
		xdir = malloc (strlen (g_get_home_dir ()) + 8);
		sprintf (xdir, "%s/.xchat", g_get_home_dir ());
	}
	return xdir;
#else
	return "./data"; /* FIXME */
#endif
}

void
check_prefs_dir (void)
{
	char *xdir = get_xdir ();
	if (access (xdir, F_OK) != 0)
	{
#ifdef WIN32
		if (mkdir (xdir) != 0)
#else
		if (mkdir (xdir, S_IRUSR | S_IWUSR | S_IXUSR) != 0)
#endif
			fe_message (_("Cannot create ~/.xchat"), FALSE);
	}
}

char *
default_file (void)
{
	static char *dfile = 0;

	if (!dfile)
	{
		dfile = malloc (strlen (get_xdir ()) + 12);
		sprintf (dfile, "%s/xchat.conf", get_xdir ());
	}
	return dfile;
}

/* Keep these sorted!! */

#ifdef USE_PERL
struct prefs vars[] = {
#else
static struct prefs vars[] = {
#endif
	{"auto_indent", PREFS_OFFINT (auto_indent), TYPE_BOOL},
	{"auto_resume", PREFS_OFFINT (autoresume), TYPE_BOOL},
	{"autodccchat", PREFS_OFFINT (autodccchat), TYPE_BOOL},
	{"autodccsend", PREFS_OFFINT (autodccsend), TYPE_BOOL},
	{"autodialog", PREFS_OFFINT (autodialog), TYPE_BOOL},
	{"autoopendccchatwindow", PREFS_OFFINT (autoopendccchatwindow), TYPE_BOOL},
	{"autoopendccrecvwindow", PREFS_OFFINT (autoopendccrecvwindow), TYPE_BOOL},
	{"autoopendccsendwindow", PREFS_OFFINT (autoopendccsendwindow), TYPE_BOOL},
	{"autoreconnect", PREFS_OFFINT (autoreconnect), TYPE_BOOL},
	{"autoreconnectonfail", PREFS_OFFINT (autoreconnectonfail), TYPE_BOOL},
	{"autorejoin", PREFS_OFFINT (autorejoin), TYPE_BOOL},
	{"autosave", PREFS_OFFINT (autosave), TYPE_BOOL},
	{"autosaveurl", PREFS_OFFINT (autosave_url), TYPE_BOOL},
	{"awayreason", PREFS_OFFSET (awayreason), TYPE_STR},

	{"background_dialog_pic", PREFS_OFFSET (background_dialog), TYPE_STR},
	{"background_pic", PREFS_OFFSET (background), TYPE_STR},
	{"ban_type", PREFS_OFFINT (bantype), TYPE_INT},
	{"beep_msg", PREFS_OFFINT (beepmsg), TYPE_BOOL},
	{"beep_chans", PREFS_OFFINT (beepchans), TYPE_BOOL},
	{"bluestring", PREFS_OFFSET (bluestring), TYPE_STR},

	{"chanmodebuttons", PREFS_OFFINT (chanmodebuttons), TYPE_BOOL},
	{"channelbox", PREFS_OFFINT (channelbox), TYPE_INT},
	{"cmdchar", PREFS_OFFSET (cmdchar), TYPE_STR},
	{"colorednicks", PREFS_OFFINT (colorednicks), TYPE_BOOL},
	{"ctcp_number_limit", PREFS_OFFINT (ctcp_number_limit), TYPE_INT},
	{"ctcp_time_limit", PREFS_OFFINT (ctcp_time_limit), TYPE_INT},

	{"dcc_blocksize", PREFS_OFFINT (dcc_blocksize), TYPE_INT},
	{"dcc_ip", PREFS_OFFSET (dcc_ip_str), TYPE_STR},
	{"dcc_permissions", PREFS_OFFINT (dccpermissions), TYPE_INT},
	{"dcc_send_fillspaces", PREFS_OFFINT (dcc_send_fillspaces), TYPE_BOOL},
	{"dcc_stall_timeout", PREFS_OFFINT (dccstalltimeout), TYPE_INT},
	{"dcc_timeout", PREFS_OFFINT (dcctimeout), TYPE_INT},
	{"dccdir", PREFS_OFFSET (dccdir), TYPE_STR},
	{"dccwithnick", PREFS_OFFINT (dccwithnick), TYPE_BOOL},
	{"dialog_height", PREFS_OFFINT (dialog_height), TYPE_INT},
	{"dialog_indent_nicks", PREFS_OFFINT (dialog_indent_nicks), TYPE_BOOL},
	{"dialog_indent_pixels", PREFS_OFFINT (dialog_indent_pixels), TYPE_INT},
	{"dialog_show_separator", PREFS_OFFINT (dialog_show_separator), TYPE_BOOL},
	{"dialog_tint", PREFS_OFFINT (dialog_tint), TYPE_BOOL},
	{"dialog_tint_blue", PREFS_OFFINT (dialog_tint_blue), TYPE_INT},
	{"dialog_tint_green", PREFS_OFFINT (dialog_tint_green), TYPE_INT},
	{"dialog_tint_red", PREFS_OFFINT (dialog_tint_red), TYPE_INT},
	{"dialog_transparent", PREFS_OFFINT (dialog_transparent), TYPE_BOOL},
	{"dialog_width", PREFS_OFFINT (dialog_width), TYPE_INT},
	{"dialog_wordwrap", PREFS_OFFINT (dialog_wordwrap), TYPE_BOOL},
	{"dnsprogram", PREFS_OFFSET (dnsprogram), TYPE_STR},
	{"doubleclickuser", PREFS_OFFSET (doubleclickuser), TYPE_STR},

	{"fastdccsend", PREFS_OFFINT (fastdccsend), TYPE_BOOL},
	{"filterbeep", PREFS_OFFINT (filterbeep), TYPE_BOOL},
	{"first_dcc_send_port", PREFS_OFFINT (first_dcc_send_port), TYPE_INT},
	{"font_dialog_normal", PREFS_OFFSET (dialog_font_normal), TYPE_STR},
	{"font_normal", PREFS_OFFSET (font_normal), TYPE_STR},
#ifdef USE_ZVT
	{"font_shell", PREFS_OFFSET (font_shell), TYPE_STR},
#endif
	{"fudgeservernotice", PREFS_OFFINT (fudgeservernotice), TYPE_BOOL},

#ifdef USE_HEBREW
	{"hebrew", PREFS_OFFINT (hebrew), TYPE_BOOL},
#endif
	{"hide_version", PREFS_OFFINT (hidever), TYPE_BOOL},
	{"hidemenu", PREFS_OFFINT (hidemenu), TYPE_BOOL},
	{"hideuserlist", PREFS_OFFINT (hideuserlist), TYPE_BOOL},
	{"hilight_notify", PREFS_OFFINT (hilitenotify), TYPE_BOOL},
	{"hilightnick", PREFS_OFFINT (hilightnick), TYPE_BOOL},
	{"host_in_userlist", PREFS_OFFINT(showhostname_in_userlist), TYPE_BOOL},
	{"hostname", PREFS_OFFSET (hostname), TYPE_STR},

	{"indent_nicks", PREFS_OFFINT (indent_nicks), TYPE_BOOL},
	{"indent_pixels", PREFS_OFFINT (indent_pixels), TYPE_INT},
	{"inputgad_superfocus", PREFS_OFFINT (inputgad_superfocus), TYPE_BOOL},
	{"invisible", PREFS_OFFINT (invisible), TYPE_BOOL},
	{"ip_from_server", PREFS_OFFINT (ip_from_server), TYPE_BOOL},

	{"lagometer", PREFS_OFFINT (lagometer), TYPE_INT},
	{"last_dcc_send_port", PREFS_OFFINT (last_dcc_send_port), TYPE_INT},
	{"limitedtabhighlight", PREFS_OFFINT (limitedtabhighlight), TYPE_BOOL},
	{"logging", PREFS_OFFINT (logging), TYPE_BOOL},
	{"logmask", PREFS_OFFSET (logmask), TYPE_STR},

	{"mail_check", PREFS_OFFINT (mail_check), TYPE_BOOL},
	{"mainwindow_height", PREFS_OFFINT (mainwindow_height), TYPE_INT},
	{"mainwindow_left", PREFS_OFFINT (mainwindow_left), TYPE_INT},
	{"mainwindow_save", PREFS_OFFINT (mainwindow_save), TYPE_BOOL},
	{"mainwindow_top", PREFS_OFFINT (mainwindow_top), TYPE_INT},
	{"mainwindow_width", PREFS_OFFINT (mainwindow_width), TYPE_INT},
	{"max_auto_indent", PREFS_OFFINT (max_auto_indent), TYPE_INT},
	{"max_lines", PREFS_OFFINT (max_lines), TYPE_INT},
	{"msg_number_limit", PREFS_OFFINT (msg_number_limit), TYPE_INT},
	{"msg_time_limit", PREFS_OFFINT (msg_time_limit), TYPE_INT},

	{"newtabs_to_front", PREFS_OFFINT (newtabstofront), TYPE_BOOL},
	{"nick_suffix", PREFS_OFFSET (nick_suffix), TYPE_STR},
	{"nickcompletion", PREFS_OFFINT (nickcompletion), TYPE_BOOL},
	{"nickgad", PREFS_OFFINT (nickgad), TYPE_BOOL},
	{"nickname1", PREFS_OFFSET (nick1), TYPE_STR},
	{"nickname2", PREFS_OFFSET (nick2), TYPE_STR},
	{"nickname3", PREFS_OFFSET (nick3), TYPE_STR},
	{"notices_tabs", PREFS_OFFINT (notices_tabs), TYPE_BOOL},
	{"notify_timeout", PREFS_OFFINT (notify_timeout), TYPE_INT},
	{"nu_color", PREFS_OFFINT (nu_color), TYPE_INT},

	{"old_nickcompletion", PREFS_OFFINT (old_nickcompletion), TYPE_BOOL},

	{"paned_userlist", PREFS_OFFINT (paned_userlist), TYPE_BOOL},
#ifdef USE_PANEL
	{"panel_vbox", PREFS_OFFINT (panel_vbox), TYPE_BOOL},
	{"panelize_hide", PREFS_OFFINT (panelize_hide), TYPE_BOOL},
#endif
	{"partreason", PREFS_OFFSET (partreason), TYPE_STR},
	{"percascii", PREFS_OFFINT (perc_ascii), TYPE_BOOL},
	{"perccolor", PREFS_OFFINT (perc_color), TYPE_BOOL},
#ifdef USE_PERL
	{"perlwarnings", PREFS_OFFINT (perlwarnings), TYPE_BOOL},
#endif
	{"persist_chans", PREFS_OFFINT (persist_chans), TYPE_BOOL},
	{"pingtimeout", PREFS_OFFINT (pingtimeout), TYPE_INT},
	{"priv_msg_tabs", PREFS_OFFINT (privmsgtab), TYPE_BOOL},
	{"proxy_host", PREFS_OFFSET (proxy_host), TYPE_STR},
	{"proxy_port", PREFS_OFFINT (proxy_port), TYPE_INT},
	{"proxy_type", PREFS_OFFINT (proxy_type), TYPE_INT},

	{"quitreason", PREFS_OFFSET (quitreason), TYPE_STR},

	{"raw_modes", PREFS_OFFINT (raw_modes), TYPE_BOOL},
	{"realname", PREFS_OFFSET (realname), TYPE_STR},
	{"reconnect_delay", PREFS_OFFINT (recon_delay), TYPE_INT},

	{"servernotice", PREFS_OFFINT (servernotice), TYPE_BOOL},
	{"show_away_message", PREFS_OFFINT (show_away_message), TYPE_BOOL},
	{"show_away_once", PREFS_OFFINT (show_away_once), TYPE_BOOL},
	{"show_invite_in_front_session",
	 PREFS_OFFINT (show_invite_in_front_session), TYPE_BOOL},
	{"show_notify_in_front_session",
	 PREFS_OFFINT (show_notify_in_front_session), TYPE_BOOL},
	{"show_separator", PREFS_OFFINT (show_separator), TYPE_BOOL},
	{"skipmotd", PREFS_OFFINT (skipmotd), TYPE_BOOL},
	{"skipserverlist", PREFS_OFFINT (skipserverlist), TYPE_BOOL},
	{"soundcmd", PREFS_OFFSET (soundcmd), TYPE_STR},
	{"sounddir", PREFS_OFFSET (sounddir), TYPE_STR},
	{"stamp_format", PREFS_OFFSET (stamp_format), TYPE_STR},
	{"stripcolor", PREFS_OFFINT (stripcolor), TYPE_BOOL},
	{"style_inputbox", PREFS_OFFINT (style_inputbox), TYPE_BOOL},
	{"style_namelistgad", PREFS_OFFINT (style_namelistgad), TYPE_BOOL},

	{"tabchannels", PREFS_OFFINT (tabchannels), TYPE_BOOL},
	{"tabs_position", PREFS_OFFINT (tabs_position), TYPE_INT},
	{"thin_separator", PREFS_OFFINT (thin_separator), TYPE_BOOL},
	{"throttle", PREFS_OFFINT (throttle), TYPE_BOOL},
	{"throttlemeter", PREFS_OFFINT (throttlemeter), TYPE_INT},
	{"timestamp", PREFS_OFFINT (timestamp), TYPE_BOOL},
	{"timestamp_logs", PREFS_OFFINT (timestamp_logs), TYPE_BOOL},
	{"timestamp_log_format", PREFS_OFFSET (timestamp_log_format), TYPE_STR},
	{"tint", PREFS_OFFINT (tint), TYPE_BOOL},
	{"tint_blue", PREFS_OFFINT (tint_blue), TYPE_INT},
	{"tint_green", PREFS_OFFINT (tint_green), TYPE_INT},
	{"tint_red", PREFS_OFFINT (tint_red), TYPE_INT},
	{"topicbar", PREFS_OFFINT (topicbar), TYPE_BOOL},
	{"transparent", PREFS_OFFINT (transparent), TYPE_BOOL},
#ifdef USE_TRANS
	{"trans_file", PREFS_OFFSET (trans_file), TYPE_STR},
#endif
	{"treeview", PREFS_OFFINT (treeview), TYPE_BOOL},

	{"use_fontset", PREFS_OFFINT (use_fontset), TYPE_BOOL},
	{"use_server_tab", PREFS_OFFINT (use_server_tab), TYPE_BOOL},
#ifdef USE_TRANS
	{"use_trans", PREFS_OFFINT (use_trans), TYPE_BOOL},
#endif
	{"userhost", PREFS_OFFINT (userhost), TYPE_BOOL},
	{"userlist_sort", PREFS_OFFINT (userlist_sort), TYPE_INT},
	{"userlist_icons", PREFS_OFFINT (userlist_icons), TYPE_BOOL},
	{"userlistbuttons", PREFS_OFFINT (userlistbuttons), TYPE_BOOL},
	{"username", PREFS_OFFSET (username), TYPE_STR},

	{"wallops", PREFS_OFFINT (wallops), TYPE_BOOL},
	{"whois_on_notifyonline", PREFS_OFFINT (whois_on_notifyonline), TYPE_BOOL},
	{"windows_as_tabs", PREFS_OFFINT (windows_as_tabs), TYPE_BOOL},
	{"wordwrap", PREFS_OFFINT (wordwrap), TYPE_BOOL},

	{0, 0, 0},
};

void
load_config (void)
{
	struct stat st;
	char *cfg, *username;
	int res, val, i, fh;
#ifdef	USE_JCODE
	gchar *locale;
#endif

	memset (&prefs, 0, sizeof (struct xchatprefs));
	/* Just for ppl upgrading --AGL */
	strcpy (prefs.logmask, "%s,%c.xchatlog");
	strcpy (prefs.nick_suffix, ":");
	strcpy (prefs.cmdchar, "/");
	prefs.auto_indent = 1;
	prefs.max_auto_indent = 256;
	prefs.mail_check = 1;
	prefs.show_separator = 1;
	prefs.dialog_show_separator = 1;
	prefs.dcc_blocksize = 1024;
	prefs.throttle = 1;
	 /*FIXME*/ prefs.msg_time_limit = 30;
	prefs.msg_number_limit = 5;
	prefs.ctcp_time_limit = 30;
	prefs.ctcp_number_limit = 5;
	prefs.topicbar = 1;
	prefs.lagometer = 1;
	prefs.throttlemeter = 1;
	prefs.nickgad = 1;
	prefs.autoopendccrecvwindow = 1;
	prefs.autoopendccsendwindow = 1;
	prefs.autoopendccchatwindow = 1;
	prefs.chanmodebuttons = 1;
	prefs.userhost = 1;
	prefs.userlistbuttons = 1;
	prefs.hilightnick = 1;
	prefs.persist_chans = 1;
	prefs.userlist_icons = 1;
	prefs.perc_color = 1;
	prefs.dialog_width = 300;
	prefs.dialog_height = 100;
	prefs.dcc_send_fillspaces = 1;
	prefs.mainwindow_save = 1;
	strcpy (prefs.stamp_format, "[%H:%M:%S] ");
	strcpy (prefs.timestamp_log_format, "%b %d %H:%M:%S ");

#ifdef	USE_JCODE
	locale = setlocale (LC_CTYPE, "");
	if (locale != NULL && !g_strncasecmp (locale, "ja", 2))
		prefs.kanji_conv = 1;
#endif

	check_prefs_dir ();
	username = g_get_user_name ();
	if (!username)
		username = "root";

	fh = open (default_file (), OFLAGS | O_RDONLY);
	if (fh != -1)
	{
		fstat (fh, &st);
		cfg = malloc (st.st_size + 1);
		cfg[0] = '\0';
		i = read (fh, cfg, st.st_size);
		if (i >= 0)
			cfg[i] = '\0';					/* make sure cfg is NULL terminated */
		close (fh);
		i = 0;
		do
		{
			switch (vars[i].type)
			{
			case TYPE_STR:
				cfg_get_str (cfg, vars[i].name, (char *) &prefs + vars[i].offset);
				break;
			case TYPE_BOOL:
			case TYPE_INT:
				val = cfg_get_int_with_result (cfg, vars[i].name, &res);
				if (res)
					*((int *) &prefs + vars[i].offset) = val;
				break;
			}
			i++;
		}
		while (vars[i].type != 0);

		free (cfg);

	} else
	{
#ifndef WIN32

#ifdef __EMX__
		/* OS/2 uses UID 0 all the time */
		fe_message (_("The default download directory is your\n"
						"home dir, you should change this at some stage."), TRUE);
#else
		if (getuid () == 0)
			fe_message (_("* Running IRC as root is stupid! You should\n"
							"  create a User Account and use that to login.\n\n"
							"* The default download directory is your\n"
							"  home dir, you should change this at some stage."),
							TRUE);
		else
			fe_message ("The default download directory is your\n"
							"home dir, you should change this at some stage.", TRUE);
#endif

#endif /* !WIN32 */
		/* put in default values, anything left out is automatically zero */

		prefs.autoresume = 1;
		prefs.show_away_once = 1;
		prefs.show_away_message = 1;
		prefs.indent_pixels = 80;
		prefs.dialog_indent_pixels = 80;
		prefs.indent_nicks = 1;
		prefs.dialog_indent_nicks = 1;
		prefs.thin_separator = 1;
		prefs.fastdccsend = 1;
		prefs.wordwrap = 1;
		prefs.dialog_wordwrap = 1;
		prefs.autosave = 1;
		prefs.autodialog = 1;
		prefs.autoreconnect = 1;
		prefs.recon_delay = 10;
		prefs.tabchannels = 1;
		prefs.newtabstofront = 1;
		prefs.use_server_tab = 1;
		prefs.windows_as_tabs = 1;
		prefs.privmsgtab = 1;
		prefs.nickcompletion = 1;
		prefs.style_inputbox = 1;
		prefs.nu_color = 4;
		prefs.dccpermissions = 0600;
		prefs.max_lines = 300;
		prefs.mainwindow_width = 630;
		prefs.mainwindow_height = 422;
		prefs.dcctimeout = 180;
		prefs.dccstalltimeout = 60;
		prefs.notify_timeout = 15;
		prefs.tint_red =
			prefs.tint_green =
			prefs.tint_blue =
			prefs.dialog_tint_red =
			prefs.dialog_tint_green = prefs.dialog_tint_blue = 195;
		strcpy (prefs.nick1, username);
		strcpy (prefs.nick2, username);
		strcat (prefs.nick2, "_");
		strcpy (prefs.nick3, username);
		strcat (prefs.nick3, "__");
		strcpy (prefs.realname, username);
		strcpy (prefs.username, username);
#ifdef WIN32
		strcpy (prefs.sounddir, "./sound");
		strcpy (prefs.dccdir, "./dcc");
#else
		strcpy (prefs.dccdir, g_get_home_dir ());
		strcpy (prefs.sounddir, prefs.dccdir);
#endif
		strcpy (prefs.doubleclickuser, "/QUOTE WHOIS %s");
		strcpy (prefs.awayreason, _("I'm busy"));
		strcpy (prefs.quitreason, _("Client Exiting"));
		strcpy (prefs.partreason, prefs.quitreason);
#ifdef USE_JCODE
		prefs.use_fontset = 1;
		strcpy (prefs.font_normal, DEF_FONT_JCODE);
#else
#ifdef WIN32
		strcpy (prefs.font_normal, DEF_FONT_WIN32);
#else
		strcpy (prefs.font_normal, DEF_FONT);
#endif
#endif
		strcpy (prefs.dialog_font_normal, prefs.font_normal);
		strcpy (prefs.soundcmd, "esdplay");
		strcpy (prefs.dnsprogram, "host");
	}
	if (prefs.mainwindow_height < 138)
		prefs.mainwindow_height = 138;
	if (prefs.mainwindow_width < 106)
		prefs.mainwindow_width = 106;
}

int
save_config (void)
{
	int fh, i;

	check_prefs_dir ();

	fh = open (default_file (), OFLAGS | O_TRUNC | O_WRONLY | O_CREAT, 0600);
	if (fh == -1)
		return 0;

	cfg_put_str (fh, "version", VERSION);
	i = 0;
	do
	{
		switch (vars[i].type)
		{
		case TYPE_STR:
			cfg_put_str (fh, vars[i].name, (char *) &prefs + vars[i].offset);
			break;
		case TYPE_INT:
		case TYPE_BOOL:
			cfg_put_int (fh, *((int *) &prefs + vars[i].offset), vars[i].name);
		}
		i++;
	}
	while (vars[i].type != 0);

	close (fh);

	return 1;
}

static void
set_showval (session *sess, struct prefs *var, char *tbuf)
{
	int len, dots, j;
	static char *offon[] = { "OFF", "ON" };

	len = strlen (var->name);
	memcpy (tbuf, var->name, len);
	dots = 29 - len;
	if (dots < 0)
		dots = 0;
	tbuf[len++] = '\003';
	tbuf[len++] = '2';
	for (j=0;j<dots;j++)
		tbuf[j + len] = '.';
	len += j;
	switch (var->type)
	{
	case TYPE_STR:
		sprintf (tbuf + len, "\0033:\017 %s\n",
					(char *) &prefs + var->offset);
		break;
	case TYPE_INT:
		sprintf (tbuf + len, "\0033:\017 %d\n",
					*((int *) &prefs + var->offset));
		break;
	case TYPE_BOOL:
		sprintf (tbuf + len, "\0033:\017 %s\n", offon[
					*((int *) &prefs + var->offset)]);
		break;
	}
	PrintText (sess, tbuf);
}

static void
set_list (session * sess, char *tbuf)
{
	int i;

	i = 0;
	do
	{
		set_showval (sess, &vars[i], tbuf);
		i++;
	}
	while (vars[i].type != 0);
}

int
cfg_get_bool (char *var)
{
	int i = 0;

	do
	{
		if (!strcasecmp (var, vars[i].name))
		{
			return *((int *) &prefs + vars[i].offset);
		}
		i++;
	}
	while (vars[i].type != 0);

	return -1;
}

int
cmd_set (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	int wild = FALSE;
	int i = 0, finds = 0, found;
	char *var = word[2];
	char *val = word_eol[3];

	if (!*var)
	{
		set_list (sess, tbuf);
		return TRUE;
	}

	if ((strchr (var, '*') || strchr (var, '?')) && !*val)
		wild = TRUE;

	if (*val == '=')
		val++;

	do
	{
		if (wild)
			found = !match (var, vars[i].name);
		else
			found = strcasecmp (var, vars[i].name);

		if (found == 0)
		{
			finds++;
			switch (vars[i].type)
			{
			case TYPE_STR:
				if (*val)
				{
					strcpy ((char *) &prefs + vars[i].offset, val);
					sprintf (tbuf, "%s set to: %s\n", var, val);
					PrintText (sess, tbuf);
				} else
				{
					set_showval (sess, &vars[i], tbuf);
				}
				break;
			case TYPE_INT:
			case TYPE_BOOL:
				if (*val)
				{
					if (vars[i].type == TYPE_BOOL)
					{
						if (atoi (val))
							*((int *) &prefs + vars[i].offset) = 1;
						else
							*((int *) &prefs + vars[i].offset) = 0;
						if (!strcasecmp (val, "YES") || !strcasecmp (val, "ON"))
							*((int *) &prefs + vars[i].offset) = 1;
						if (!strcasecmp (val, "NO") || !strcasecmp (val, "OFF"))
							*((int *) &prefs + vars[i].offset) = 0;
					} else
					{
						*((int *) &prefs + vars[i].offset) = atoi (val);
					}
					sprintf (tbuf, "%s set to: %d\n", var,
								*((int *) &prefs + vars[i].offset));
					PrintText (sess, tbuf);
				} else
				{
					set_showval (sess, &vars[i], tbuf);
				}
				break;
			}
		}
		i++;
	}
	while (vars[i].type != 0);

	if (!finds)
		PrintText (sess, "No such variable.\n");

	return TRUE;
}
