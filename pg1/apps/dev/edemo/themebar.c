/* $Id$
 *
 * dialogdemo.c - based on  PicoGUI's "dialogdemo.c"
 *
 * Modified by: Daniele Pizzoni - Ascensit s.r.l. - Italy
 * tsho@ascensit.com - auouo@tin.it
 *
 * Original header:
 * 
 * this is a really quick and hackish theme picker app.
 * It will be replaced by a PGL applet or menu that reads a theme's
 * tags to get info on it, and a thumbnail maybe... This is just for
 * debugging the theme loading.
 */

// #define THEME_DIRECTORY "themes"
char *THEME_DIRECTORY;

#include <sys/types.h>   /* For making directory listings */
#include <dirent.h>
#include <sys/stat.h>
#include <malloc.h>
#include <unistd.h>
#include <string.h>

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
    pghandle handle;
    char active;
} array[LENGHT];
static int idx = 0;

static int evtChangeTheme(struct pgEvent *evt) {
  char buf[80];
  int i;
  pghandle currContext;

  i = pgGetPayload(evt->from);

  if (array[i].active) {
      pgDelete(array[i].handle);
      array[i].active = 0;
  }
  else {

      /* fixme buffer overflow... */
      strcpy(buf, pgGetString(array[i].dir));
      strcat(buf, pgGetString(array[i].fname));

      printf("buf: %s", buf);

/*       pgSetContext(*themeContext); */
/*       array[i].handle = pgLoadTheme(pgFromFile(buf)); */
/*       pgSetContext(*pageContext); */

      currContext = pgGetContext();
      pgSetContext(*(pghandle*)(evt->extra));
      array[i].handle = pgLoadTheme(pgFromFile(buf));
      pgSetContext(currContext);


      array[i].active = 1;
  }

  return 0;
}

/* widget: the parent widget to derive inside
 * directory: the directory into which search for themes */
void themebar(pghandle widget, char* directory, int* themecontext)
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
    if (strcmp(dent->d_name+l-3,".th")) continue;

    buf[0] = 0;
/*     dent->d_name[l-3] = 0; */

    if (idx == LENGHT) {
	eerror("max array lenght reached in themebar.c: too many themes.");
	break;
    }

    array[idx].dir = pgNewString(directory);
    array[idx].fname = pgNewString(dent->d_name);
    array[idx].active = 0;

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

    pgBind(PGDEFAULT,PG_WE_ACTIVATE,evtChangeTheme, themecontext);
  }
  closedir(d);

  pgSubUpdate(widget);
  return;
}

void themebar_clear(void)
{
    int i;

    for (i = 0; i < idx; i++) {
	array[i].active = 0;
    }
    return;
}

void themebar_delete(void)
{
    idx = 0;
    return;
}
   
/* The End */
