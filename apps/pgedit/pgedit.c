/* Demonstrate the textbox widget's editing capabilities */

#include <picogui.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

pghandle wText;
char *filename=NULL;

int evtLoad(struct pgEvent *evt)
 {
  const char *file;
  pghandle hstr;

  file = pgFilePicker(NULL,NULL,filename,PG_FILEOPEN,"Load a plain text file");
  if (file)
   {
    if(filename)
      free(filename);
    filename=strdup(file);
    hstr = pgDataString(pgFromFile(filename));
    pgSetWidget(wText, PG_WP_TEXT, hstr, 0);
    pgDelete(hstr);
   }
  return 0;
 }

int evtSave(struct pgEvent *evt)
 {
  pghandle hstr;
  const char *file;
  char *str;
  FILE *f;

  file = pgFilePicker(NULL,NULL,filename,PG_FILESAVE,"Save a plain text file");
  if (file)
   {
    if(filename)
      free(filename);
    filename=strdup(file);
    hstr=pgGetWidget(wText, PG_WP_TEXT);
    str=pgGetString(hstr);
    f=fopen(filename, "w");
    if(f)
     {
      fputs(str, f);
      fclose(f);
     }
    else
      pgMessageDialog("Save error", strerror(errno),
	  PG_MSGBTN_OK|PG_MSGICON_ERROR);
    pgDelete(hstr);
   }
  return 0;
 }

int main(int argc, char **argv) {
  pghandle title, app;

  pgInit(argc,argv);
  app=pgRegisterApp(PG_APP_NORMAL,"Textbox Editing Test",0);

  title = pgGetWidget(0, PG_WP_PANELBAR_LABEL);

  pgNewWidget(PG_WIDGET_BUTTON, PG_DERIVE_BEFORE, title);
  pgSetWidget(PGDEFAULT, PG_WP_TEXT, pgNewString("Load"),
      PG_WP_SIDE, PG_S_LEFT, 0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&evtLoad,NULL);

  pgNewWidget(PG_WIDGET_BUTTON, PG_DERIVE_BEFORE, title);
  pgSetWidget(PGDEFAULT, PG_WP_TEXT,pgNewString("Save"),
      PG_WP_SIDE, PG_S_LEFT, 0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&evtSave,NULL);
 
  wText = pgNewWidget(PG_WIDGET_TEXTBOX, PG_DERIVE_INSIDE, app);
  pgSetWidget(PGDEFAULT, PG_WP_SIDE,PG_S_ALL,
	      PG_WP_TRIGGERMASK, 0x67fd,  /* not all constants are defined */
	      0);

  pgNewWidget(PG_WIDGET_SCROLL,PG_DERIVE_BEFORE,wText);
  pgSetWidget(PGDEFAULT,PG_WP_BIND,wText,0);

  pgFocus(PGDEFAULT);

  pgEventLoop();
  return 0;
}
  
