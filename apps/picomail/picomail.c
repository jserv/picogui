#include <picogui.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "imap.h"
#include "configfile.h"

pghandle wBox, *wRow, wHead, wHBox;
int dc, dr;


int closeboxHandler(struct pgEvent *evt) {
  return 0;
}


int getList(struct pgEvent *evt) {
   imap_getlist();
   return 0;
}


int main(int argc, char *argv[])
{
   pghandle wToolbar, wScroll, wItem;

   if(configfile_parse("./picomail.conf")){

   }
   else
       exit(1);
   
   pgInit(argc,argv);
   pgRegisterApp(PG_APP_NORMAL,"PicoMail",0);
   
   
   wToolbar = pgNewWidget(PG_WIDGET_TOOLBAR,0,0);

   

   wScroll = pgNewWidget(PG_WIDGET_SCROLL,0,0);
   wBox = pgNewWidget(PG_WIDGET_BOX,0,0);
   pgSetWidget(PGDEFAULT,
            PG_WP_SIDE,PG_S_ALL,
            0);
   pgSetWidget(wScroll,PG_WP_BIND,wBox,0);


   
    wItem = pgNewWidget(PG_WIDGET_LISTITEM,
			0 ? PGDEFAULT : PG_DERIVE_INSIDE,
			0 ? PGDEFAULT : wBox);
    pgReplaceTextFmt(PGDEFAULT,"Normal listitem #%d",0);



    
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
