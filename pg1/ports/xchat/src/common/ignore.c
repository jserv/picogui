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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../../config.h"
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "xchat.h"
#include "ignore.h"
#include "plugin.h"
#include "cfgfiles.h"
#include "fe.h"
#include "text.h"
#include "util.h"
#include "xchatc.h"


int ignored_ctcp = 0;			  /* keep a count of all we ignore */
int ignored_priv = 0;
int ignored_chan = 0;
int ignored_noti = 0;
int ignored_invi = 0;
static int ignored_total = 0;

/* ignore_add(...)

 * returns:
 *            0 fail
 *            1 success
 *            2 success (old ignore has been changed)
 */

int
ignore_add (char *mask, int priv, int noti, int chan, int ctcp, int invi,
				int unignore, int no_save)
{
	struct ignore *ig = 0;
	GSList *list;
	int change_only = FALSE;

	/* first check if it's already ignored */
	list = ignore_list;
	while (list)
	{
		ig = (struct ignore *) list->data;
		if (!strcasecmp (ig->mask, mask))
		{
			/* already ignored, change the flags */
			change_only = TRUE;
			break;
		}
		list = list->next;
	}

	if (!change_only)
		ig = malloc (sizeof (struct ignore));

	if (!ig)
		return 0;

	ig->mask = strdup (mask);
	ig->priv = priv;
	ig->noti = noti;
	ig->chan = chan;
	ig->ctcp = ctcp;
	ig->invi = invi;
	ig->unignore = unignore;
	ig->no_save = no_save;

	if (!change_only)
		ignore_list = g_slist_prepend (ignore_list, ig);
	fe_ignore_update (1);

	if (change_only)
		return 2;

	return 1;
}

void
ignore_showlist (struct session *sess)
{
	struct ignore *ig;
	GSList *list = ignore_list;
	char tbuf[256];
	int i = 0;

	EMIT_SIGNAL (XP_TE_IGNOREHEADER, sess, 0, 0, 0, 0, 0);

	while (list)
	{
		ig = (struct ignore *) list->data;
		i++;

		sprintf (tbuf, " %-20s ", ig->mask);
		if (ig->priv)
			strcat (tbuf, _("YES  "));
		else
			strcat (tbuf, _("NO   "));
		if (ig->noti)
			strcat (tbuf, _("YES  "));
		else
			strcat (tbuf, _("NO   "));
		if (ig->chan)
			strcat (tbuf, _("YES  "));
		else
			strcat (tbuf, _("NO   "));
		if (ig->ctcp)
			strcat (tbuf, _("YES  "));
		else
			strcat (tbuf, _("NO   "));
		if (ig->invi)
			strcat (tbuf, _("YES  "));
		else
			strcat (tbuf, _("NO   "));
		if (ig->unignore)
			strcat (tbuf, _("YES  "));
		else
			strcat (tbuf, _("NO   "));
		strcat (tbuf, "\n");
		PrintText (sess, tbuf);
		/*EMIT_SIGNAL (XP_TE_IGNORELIST, sess, ig->mask, 0, 0, 0, 0); */
		/* use this later, when TE's support 7 args */
		list = list->next;
	}

	if (!i)
		EMIT_SIGNAL (XP_TE_IGNOREEMPTY, sess, 0, 0, 0, 0, 0);

	EMIT_SIGNAL (XP_TE_IGNOREFOOTER, sess, 0, 0, 0, 0, 0);
}

/* ignore_del()

 * one of the args must be NULL, use mask OR *ig, not both
 *
 */

int
ignore_del (char *mask, struct ignore *ig)
{
	if (!ig)
	{
		GSList *list = ignore_list;

		while (list)
		{
			ig = (struct ignore *) list->data;
			if (!strcasecmp (ig->mask, mask))
				break;
			list = list->next;
			ig = 0;
		}
	}
	if (ig)
	{
		ignore_list = g_slist_remove (ignore_list, ig);
		free (ig->mask);
		free (ig);
		fe_ignore_update (1);
		return TRUE;
	}
	return FALSE;
}

/* check if a msg should be ignored by browsing our ignore list */

int
ignore_check (char *host, int priv, int noti, int chan, int ctcp, int invi)
{
	struct ignore *ig;
	GSList *list = ignore_list;

	/* check if there's an UNIGNORE first, they take precendance. */
	while (list)
	{
		ig = (struct ignore *) list->data;
		if (ig->unignore)
		{
			if (
				 (ig->priv && priv) ||
				 (ig->noti && noti) ||
				 (ig->chan && chan) || (ig->ctcp && ctcp) || (ig->invi && invi))
			{
				if (match (ig->mask, host))
					return FALSE;
			}
		}
		list = list->next;
	}

	list = ignore_list;
	while (list)
	{
		ig = (struct ignore *) list->data;

		if (
			 (ig->priv && priv) ||
			 (ig->noti && noti) ||
			 (ig->chan && chan) || (ig->ctcp && ctcp) || (ig->invi && invi))
		{
			if (match (ig->mask, host))
			{
				ignored_total++;
				if (priv)
					ignored_priv++;
				if (noti)
					ignored_noti++;
				if (chan)
					ignored_chan++;
				if (ctcp)
					ignored_ctcp++;
				if (invi)
					ignored_invi++;
				fe_ignore_update (2);
				return TRUE;
			}
		}
		list = list->next;
	}

	return FALSE;
}

static char *
ignore_read_next_entry (char *my_cfg, struct ignore *ignore)
{
	char tbuf[1024];
	char mask[256];

	/* Casting to char * done below just to satisfy compiler */

	if (my_cfg)
	{
		my_cfg = cfg_get_str (my_cfg, "mask ", mask);
		ignore->mask = strdup (mask);
	}
	if (my_cfg)
	{
		my_cfg = cfg_get_str (my_cfg, "ctcp ", (char *) tbuf);
		ignore->ctcp = atoi ((char *) tbuf);
	}
	if (my_cfg)
	{
		my_cfg = cfg_get_str (my_cfg, "private ", (char *) tbuf);
		ignore->priv = atoi ((char *) tbuf);
	}
	if (my_cfg)
	{
		my_cfg = cfg_get_str (my_cfg, "channel ", (char *) tbuf);
		ignore->chan = atoi ((char *) tbuf);
	}
	if (my_cfg)
	{
		my_cfg = cfg_get_str (my_cfg, "notice ", (char *) tbuf);
		ignore->noti = atoi ((char *) tbuf);
	}
	if (my_cfg)
	{
		my_cfg = cfg_get_str (my_cfg, "invite ", (char *) tbuf);
		ignore->invi = atoi ((char *) tbuf);
	}
	if (my_cfg)
	{
		my_cfg = cfg_get_str (my_cfg, "unignore ", (char *) tbuf);
		ignore->unignore = atoi ((char *) tbuf);
	}
	return my_cfg;
}

void
ignore_load ()
{
	struct ignore *ignore;
	struct stat st;
	char *cfg, *my_cfg;
	char file[256];
	int fh, i;

	snprintf (file, sizeof file, "%s/ignore.conf", get_xdir ());
	fh = open (file, O_RDONLY | OFLAGS);
	if (fh != -1)
	{
		fstat (fh, &st);
		if (st.st_size)
		{
			cfg = malloc (st.st_size + 1);
			cfg[0] = '\0';
			i = read (fh, cfg, st.st_size);
			if (i >= 0)
				cfg[i] = '\0';
			my_cfg = cfg;
			while (my_cfg)
			{
				ignore = malloc (sizeof (struct ignore));
				if ((my_cfg = ignore_read_next_entry (my_cfg, ignore)))
				{
					ignore_list = g_slist_prepend (ignore_list, ignore);
					ignore->no_save = 0;
				} else
					free (ignore);
			}
			free (cfg);
		}
		close (fh);
	}
}

void
ignore_save ()
{
	char buf[1024];
	int fh;
	GSList *temp = ignore_list;
	struct ignore *ig;

	snprintf (buf, sizeof buf, "%s/ignore.conf", get_xdir ());
	fh = open (buf, O_TRUNC | O_WRONLY | O_CREAT | OFLAGS, 0600);
	if (fh != -1)
	{
		while (temp)
		{
			ig = (struct ignore *) temp->data;
			if (!ig->no_save)
			{
				snprintf (buf, sizeof buf,
							 "mask = %s\n"
							 "ctcp = %d\n"
							 "private = %d\n"
							 "channel = %d\n"
							 "notice = %d\n"
							 "invite = %d\n"
							 "unignore = %d\n\n",
							 ig->mask,
							 ig->ctcp,
							 ig->priv, ig->chan, ig->noti, ig->invi, ig->unignore);
				write (fh, buf, strlen (buf));
			}
			temp = temp->next;
		}
		close (fh);
	}

}

int
flood_check (char *nick, char *ip, server *serv, session *sess, int what)	/*0=ctcp  1=priv */
{
	/*
	   serv
	   int ctcp_counter; 
	   time_t ctcp_last_time;
	   prefs
	   unsigned int ctcp_number_limit;
	   unsigned int ctcp_time_limit;
	 */
	char buf[512];
	char real_ip[132];
	int i;
	time_t current_time;
	current_time = time (NULL);

	if (what == 0)
	{
		if (serv->ctcp_last_time == 0)	/*first ctcp in this server */
		{
			serv->ctcp_last_time = time (NULL);
			serv->ctcp_counter++;
		} else
		{
			if (difftime (current_time, serv->ctcp_last_time) < prefs.ctcp_time_limit)	/*if we got the ctcp in the seconds limit */
			{
				serv->ctcp_counter++;
				if (serv->ctcp_counter == prefs.ctcp_number_limit)	/*if we reached the maximun numbers of ctcp in the seconds limits */
				{
					serv->ctcp_last_time = current_time;	/*we got the flood, restore all the vars for next one */
					serv->ctcp_counter = 0;
					for (i = 0; i <= 128; i++)
						if (ip[i] == '@')
							break;
					snprintf (real_ip, sizeof (real_ip), "*!*%s", &ip[i]);
					/*ignore_add (char *mask, int priv, int noti, int chan,
					   int ctcp, int invi, int unignore, int no_save) */

					snprintf (buf, sizeof (buf),
								 "You are being CTCP flooded from %s, ignoring %s\n",
								 nick, real_ip);
					PrintText (sess, buf);

					/*FIXME: only ignore ctcp or all?, its ignoring ctcps for now */
					ignore_add (real_ip, 0, 0, 0, 1, 0, 0, 0);
					return 0;
				}
			}
		}
	} else
	{
		if (serv->msg_last_time == 0)
		{
			serv->msg_last_time = time (NULL);
			serv->ctcp_counter++;
		} else
		{
			if (difftime (current_time, serv->msg_last_time) <
				 prefs.msg_time_limit)
			{
				serv->msg_counter++;
				if (serv->msg_counter == prefs.msg_number_limit)	/*if we reached the maximun numbers of ctcp in the seconds limits */
				{
					snprintf (buf, sizeof (buf),
					 "You are being MSG flooded from %s, setting autodialog OFF.\n",
								 ip);
					PrintText (sess, buf);
					serv->msg_last_time = current_time;	/*we got the flood, restore all the vars for next one */
					serv->msg_counter = 0;
					/*ignore_add (char *mask, int priv, int noti, int chan,
					   int ctcp, int invi, int unignore, int no_save) */

					/*FIXME: only ignore ctcp or all?, its ignoring ctcps for now */
					prefs.autodialog = 0;
					return 0;
				}
			}
		}
	}
	return 1;
}

