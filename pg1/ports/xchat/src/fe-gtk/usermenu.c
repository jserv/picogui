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

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#include <unistd.h>

#include "fe-gtk.h"
#include "../common/xchat.h"
#include "../common/xchatc.h"
#include "menu.h"


void
usermenu_create (GtkWidget *menu)
{
	menu_create (menu, usermenu_list, "");
}

static void
usermenu_destroy (GtkWidget * menu)
{
	GList *items = ((GtkMenuShell *) menu)->children;
	GList *next;

	/* under gnome, the tearoff is also an item */
#ifdef USE_GNOME
	items = items->next->next->next;	/* don't destroy the 1st 3 items */
#else
	items = items->next->next;	  /* don't destroy the 1st 2 items */
#endif

	while (items)
	{
		next = items->next;
		gtk_widget_destroy (items->data);
		items = next;
	}
}

void
usermenu_update (void)
{
	int done_main = FALSE;
	GSList *list = sess_list;
	session *sess;

	while (list)
	{
		sess = list->data;
		if (sess->is_tab)
		{
			if (!done_main)
			{
				if (sess->gui->usermenu)
				{
					usermenu_destroy (sess->gui->usermenu);
					usermenu_create (sess->gui->usermenu);
					done_main = TRUE;
				}
			}
		} else
		{
			usermenu_destroy (sess->gui->usermenu);
			usermenu_create (sess->gui->usermenu);
		}
		list = list->next;
	}
}
