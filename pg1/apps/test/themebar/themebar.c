/* $Id$
 * 
 * this is a really quick and hackish theme picker app.
 * It will be replaced by a PGL applet or menu that reads a theme's
 * tags to get info on it, and a thumbnail maybe... This is just for
 * debugging the theme loading.
 */

#define THEME_DIRECTORY "."

#include <sys/types.h>   /* For making directory listings */
#include <dirent.h>
#include <sys/stat.h>
#include <malloc.h>
#include <picogui.h>
#include <unistd.h>

int evtChangeTheme(struct pgEvent *evt) {
  pghandle old;
  char buf[80];  /* FIXME: Buffer overflow 'sploit waiting to happen! */

  if (pgGetWidget(evt->from,PG_WP_ON)) {
    /* Load theme */
  
    strcpy(buf,THEME_DIRECTORY "/");
    strcat(buf,pgGetString(pgGetWidget(evt->from,PG_WP_TEXT)));
    strcat(buf,".th");
    pgSetPayload(evt->from,pgLoadTheme(pgFromFile(buf)));
  }
  else {
    /* Unload theme */
    
    pgDelete(pgGetPayload(evt->from));
  }

  return 0;
}

int main(int argc, char **argv) {
  struct dirent *dent;
  int i,l,derive;
  DIR *d;
  pgInit(argc,argv);
  pgRegisterApp(PG_APP_TOOLBAR,"ThemeBar",0);
  pgSetWidget(PGDEFAULT, PG_WP_SIDE, PG_S_LEFT,0);
  pgNewWidget(PG_WIDGET_SCROLLBOX,0,0);
  pgSetWidget(PGDEFAULT, PG_WP_SIZE, 100, 0);
  derive = PG_DERIVE_INSIDE;

  d = opendir(THEME_DIRECTORY);

  /* Make handles */
  rewinddir(d);
  i = 0;
  while (dent = readdir(d)) {
    l = strlen(dent->d_name);
    if (l<4) continue;
    if (strcmp(dent->d_name+l-3,".th")) continue;

    /* Strip extension */
    dent->d_name[l-3] = 0;

    /* Add item */
    pgNewWidget(PG_WIDGET_LISTITEM,derive,0);
    pgSetWidget(PGDEFAULT,
		PG_WP_EXTDEVENTS, pgGetWidget(PGDEFAULT, PG_WP_EXTDEVENTS) & ~PG_EXEV_EXCLUSIVE,
		PG_WP_TEXT, pgNewString(dent->d_name),
		0);
    pgBind(PGDEFAULT,PG_WE_ACTIVATE,evtChangeTheme,NULL);
    derive = 0;
  }
  closedir(d);
  
  pgEventLoop();
  return 0;
}
   
/* The End */
