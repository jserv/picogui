/* $Id$
 *
 * appbar.c - based on PicoGUI's "themebar"
 * Copyright (C) 2000 Micah Dowty <micahjd@users.sourceforge.net>
 *
 * Modified by: Daniele Pizzoni - Ascensit s.r.l. - Italy
 * tsho@ascensit.com - auouo@tin.it
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micahjd@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */


// #define THEME_DIRECTORY "themes"
char *THEME_DIRECTORY;

#include <sys/types.h>   /* For making directory listings */
#include <dirent.h>
#include <sys/stat.h>
#include <malloc.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include <picogui.h>
#include <edemo.h>

/* int* pageContext; */
/* int* themeContext; */

/* this is not dynamic because I don't have destroy widget events */
#define LENGHT 100
static struct {
    pghandle fname;
    pghandle dir;
    pghandle name;
    pid_t pid;
} array[LENGHT];
static int idx = 0;

static int evtRunApp(struct pgEvent *evt) {
  char buf[80];
  int i;
  pid_t pid;

  i = pgGetPayload(evt->from);

  if (array[i].pid) {
      printf("kill, pid: %d\n", array[i].pid);
      if ( !kill(array[i].pid, SIGTERM)) {
	  array[i].pid = 0;
      }

  } else {

      /* fixme buffer overflow... */
      strcpy(buf, pgGetString(array[i].dir));
      strcat(buf, pgGetString(array[i].fname));

      if ( (pid = fork()) == -1)
	  return 0;

      if (pid == 0) {
	  execl(buf, buf, NULL);
	  printf("exec failed\n");
	  exit(1);
      }

      array[i].pid = pid;
  }

  return 0;
}

/* widget: the parent widget to derive inside
 * directory: the directory into which search for themes */
void appbar(pghandle widget, char* directory)
{
  struct dirent *dent;
  int i,l;
  DIR *d;
  char buf[80];
  pghandle box;

  THEME_DIRECTORY=directory;
  if (! (d = opendir(THEME_DIRECTORY))) {
      eerror("directory not found");
      exit(1);
  }

  /* simple & dirty */
/*   pageContext = pagecontext; */
/*   themeContext = themecontext; */

  box = pgNewWidget(PG_WIDGET_SCROLLBOX, PG_DERIVE_AFTER, widget);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE, PG_S_ALL,
/*  	      PG_WP_TRANSPARENT, 1, */
	      0);

  /* Make handles */
  rewinddir(d);
  i = 0;
  while ((dent = readdir(d))) {
    l = strlen(dent->d_name);
    if (l<4) continue;
/*     if (strcmp(dent->d_name+l-3,".th")) continue; */

    buf[0] = 0;
/*     dent->d_name[l-3] = 0; */

    if (idx == LENGHT) {
	break;
    }

    array[idx].dir = pgNewString(directory);
    array[idx].fname = pgNewString(dent->d_name);
    array[idx].pid = 0;

    /* Add item */
    pgNewWidget(PG_WIDGET_BUTTON, PG_DERIVE_INSIDE, box);
    pgSetWidget(PGDEFAULT,
/* 		PG_WP_EXTDEVENTS, pgGetWidget(PGDEFAULT, PG_WP_EXTDEVENTS) & ~PG_EXEV_EXCLUSIVE, */
/* 		PG_WP_EXTDEVENTS, PG_EXEV_TOGGLE, */
		PG_WP_TEXT, array[idx].fname,
		PG_WP_SIDE, PG_S_BOTTOM,
		0);

    pgSetPayload(PGDEFAULT, idx);
    idx++;

    pgBind(PGDEFAULT,PG_WE_ACTIVATE,evtRunApp, NULL);
  }
  closedir(d);

  pgSubUpdate(widget);
  return;
}

void appbar_killall(void)
{
    int i;

    for (i = 0; i < idx; i++)
      if (array[i].pid) {
  	  /* if kill fails we are in trouble... */
	  if (!kill(array[i].pid, SIGTERM)) {
	      array[i].pid = 0;
	  }
	  else
	      fprintf(stderr, "appbar: kill failed on pid: %d\n", array[i].pid);
      }
    idx = 0;
    return;
}

void appbar_delete(void)
{
    idx = 0;
    return;
}
   
/* The End */
