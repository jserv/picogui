/* some routines copied from glib, just the ones we need */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "../../config.h"		  /* for HAVE_VSNPRINTF */
#include "myglib.h"

int glib_major_version = 6;
int glib_minor_version = 6;
int glib_micro_version = 6;

char *
g_get_user_name ()
{
	static char *username = 0;
	if (!username)
	{
		username = getenv ("USER");
		if (!username)
		{
			fprintf (stderr, "$USER not set\n");
			exit (0);
		}
	}
	return username;
}

char *
g_get_home_dir ()
{
	static char *homedir = 0;
	if (!homedir)
	{
		homedir = getenv ("HOME");
		if (!homedir)
		{
			fprintf (stderr, "$HOME not set\n");
			exit (0);
		}
	}
	return homedir;
}

GSList *
g_slist_alloc (void)
{
	GSList *list;

	list = malloc (sizeof (GSList));

	list->next = NULL;

	return list;
}

void
g_slist_free (GSList * list)
{
	if (list)
	{
		free (list);
	}
}

GSList *
g_slist_last (GSList * list)
{
	if (list)
	{
		while (list->next)
			list = list->next;
	}
	return list;
}

GSList *
g_slist_append (GSList * list, gpointer data)
{
	GSList *new_list;
	GSList *last;

	new_list = g_slist_alloc ();
	new_list->data = data;

	if (list)
	{
		last = g_slist_last (list);
		last->next = new_list;

		return list;
	} else
		return new_list;
}

GSList *
g_slist_prepend (GSList * list, gpointer data)
{
	GSList *new_list;

	new_list = g_slist_alloc ();
	new_list->data = data;
	new_list->next = list;

	return new_list;
}

GSList *
g_slist_insert (GSList * list, gpointer data, gint position)
{
	GSList *prev_list;
	GSList *tmp_list;
	GSList *new_list;

	if (position < 0)
		return g_slist_append (list, data);
	else if (position == 0)
		return g_slist_prepend (list, data);

	new_list = g_slist_alloc ();
	new_list->data = data;

	if (!list)
		return new_list;

	prev_list = NULL;
	tmp_list = list;

	while ((position-- > 0) && tmp_list)
	{
		prev_list = tmp_list;
		tmp_list = tmp_list->next;
	}

	if (prev_list)
	{
		new_list->next = prev_list->next;
		prev_list->next = new_list;
	} else
	{
		new_list->next = list;
		list = new_list;
	}

	return list;
}

GSList *
g_slist_remove (GSList * list, gpointer data)
{
	GSList *tmp;
	GSList *prev;

	prev = NULL;
	tmp = list;

	while (tmp)
	{
		if (tmp->data == data)
		{
			if (prev)
				prev->next = tmp->next;
			if (list == tmp)
				list = list->next;

			tmp->next = NULL;
			g_slist_free (tmp);

			break;
		}
		prev = tmp;
		tmp = tmp->next;
	}

	return list;
}


void
g_slist_foreach (GSList * list, GFunc func, gpointer user_data)
{
	while (list)
	{
		(*func) (list->data, user_data);
		list = list->next;
	}
}

#ifndef HAVE_SNPRINTF

/*#define G_VA_COPY(ap1, ap2)     memmove ((ap1), (ap2), sizeof (va_list)) */

gchar *
g_strdup_vprintf (const gchar * format, va_list args1)
{
	gchar *buffer;
	/*va_list args2; */

	/*G_VA_COPY (args2, args1); */

	buffer = malloc (strlen (format) + 1024);

	vsprintf (buffer, format, args1);
	va_end (args2);

	return buffer;
}

gint g_snprintf (gchar * str, gulong n, gchar const *fmt,...)
{
#ifdef  HAVE_VSNPRINTF
	va_list args;
	gint retval;

	va_start (args, fmt);
	retval = vsnprintf (str, n, fmt, args);
	va_end (args);
	if (retval < 0)
	{
		str[n - 1] = '\0';
		retval = strlen (str);
	}
	return retval;
#else	/* !HAVE_VSNPRINTF */
	gchar *printed;
	va_list args;
	va_start (args, fmt);
	printed = g_strdup_vprintf (fmt, args);
	va_end (args);
	strncpy (str, printed, n);
	str[n - 1] = '\0';
	free (printed);
	return strlen (str);
#endif /* !HAVE_VSNPRINTF */
}

#endif
