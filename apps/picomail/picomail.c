#include <picogui.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "imap.h"
#include "configfile.h"

#define CONFIG_FILE "/.picomail.conf"

pghandle wBox;
int row;


int closeboxHandler(struct pgEvent *evt) {
  return 0;
}


int getList(struct pgEvent *evt) {
   imap_getlist();
   return 0;
}

void
addheader( char * sender, char * title, int msg )
{
    pghandle wItem;
    
    wItem = pgNewWidget(PG_WIDGET_LISTITEM,
                        row ? PGDEFAULT : PG_DERIVE_INSIDE,
                        row ? PGDEFAULT : wBox);
        
    pgReplaceTextFmt(PGDEFAULT,"[%d] %s - (%s)",1, "Title", "Sender");

    row++;
}

int main(int argc, char *argv[])
{
   char *home, *conffile;

   pghandle wToolbar, wScroll, wItem;
   
   row = 0;
   home = getenv("HOME");
   conffile = malloc( strlen(home) + strlen(CONFIG_FILE) + 1);
   if (home && conffile) {
       strcpy( conffile, home );
       strcat( conffile, CONFIG_FILE );

       if(!configfile_parse(conffile))
           exit(1);
       free(conffile);
   }
   
   pgInit(argc,argv);
   pgRegisterApp(PG_APP_NORMAL,"PicoMail",0);
   
   
   wToolbar = pgNewWidget(PG_WIDGET_TOOLBAR,0,0);

   

   wScroll = pgNewWidget(PG_WIDGET_SCROLL,0,0);
   wBox = pgNewWidget(PG_WIDGET_BOX,0,0);
   pgSetWidget(PGDEFAULT,
            PG_WP_SIDE,PG_S_ALL,
            0);
   pgSetWidget(wScroll,PG_WP_BIND,wBox,0);


   
    
   pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,wToolbar);
   pgSetWidget(PGDEFAULT,
               PG_WP_TEXT,pgNewString("Get list of messages!"),
               PG_WP_SIDE,PG_S_LEFT,
               PG_WP_EXTDEVENTS,PG_EXEV_PNTR_DOWN,
               0);
   pgBind(PGDEFAULT,PG_WE_PNTR_DOWN,&getList,NULL);


   pgBind(PGBIND_ANY,PG_WE_CLOSE,&closeboxHandler,NULL);





   pgEventLoop();

   configfile_free();
   return 0;
}
