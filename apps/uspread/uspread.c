/*
 *
 * uspread.c -   micro-spread (sheet) application for PicoGUI
 *
 *
 * <license source="demo.c" pkg="PicoGUI/cli_c">
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
 * </license>
 *
 *
 * author: Tasnim Ahmed <tasnim@users.sourceforge.net>
 *
 * contributors:
 *
 */

#include "picogui.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

pghandle wBox, *wRow, wHead, wHBox;
char **data;
int dc, dr;

void loadFile ( char *fileName )
{
   int i, j;
   int h=13, w=45;

   data = (char **) malloc ( 1024 );
   parse_csv ( fileName, data, &dc, &dr);

   //dr -= 2; //first header, 2nd widths
   wRow = (pghandle *) malloc ( (dr-2) * sizeof ( pghandle ) );
   //wHead = (pghandle *) malloc ( dc * sizeof ( pghandle ) );

   /**** <widgets where="table header"> ****/

   wHead = pgNewWidget(PG_WIDGET_BOX,PG_DERIVE_INSIDE,wHBox);
   pgSetWidget(PGDEFAULT,
               PG_WP_SIDE, PG_S_ALL,
               0);

   i=0;
   for ( j=dc-1; j >= 0; j-- )
   {
      pgNewWidget(PG_WIDGET_LABEL,PG_DERIVE_INSIDE,wHead);
      pgSetWidget(PGDEFAULT,
               PG_WP_TEXT, pgNewString( data[j] ),
               PG_WP_SIDE, PG_S_LEFT,
               PG_WP_SIZE, atoi( data[j+dc] ),
               PG_WP_SIZEMODE, PG_SZMODE_PIXEL,
               0);
      i++;
   }
   /**** </widgets> ****/

   /**** <widgets where="client area"> ****/
   for ( i=2; i<dr; i++ )
   {
      wRow [i-2] = pgNewWidget(PG_WIDGET_BOX,PG_DERIVE_INSIDE,wBox);
      pgSetWidget(PGDEFAULT,
                  PG_WP_SIDE, PG_S_BOTTOM,
                  0);

      for ( j=dc-1; j >= 0; j-- )
      {
//printf( "%s , ", data[j+(i*dc)] );
         pgNewWidget(PG_WIDGET_FIELD,PG_DERIVE_INSIDE,wRow[i-2]);
         pgSetWidget(PGDEFAULT,
                  PG_WP_TEXT, pgNewString( data[j+(i*dc)] ),
                  PG_WP_SIDE, PG_S_LEFT,
                  PG_WP_SIZE, atoi( data[j+dc] ),
                  PG_WP_SIZEMODE, PG_SZMODE_PIXEL,
                  0);
      }
    }
   /**** </widgets> ****/
}

int closeboxHandler(struct pgEvent *evt) {
  /* Present a dialog box. If the user doesn't want to close,
     return 1 to prevent further handling of the event */

  return pgMessageDialog("µSpread - EXIT?",
         "Sure to exit?",
         PG_MSGBTN_YES | PG_MSGBTN_NO)
    == PG_MSGBTN_NO;
}


/* The simplest way to make a menu */
int handleFileMenu(struct pgEvent *evt) {
   int iMainMenu, iRecentMenu, i;
   const char *fileName;

   iMainMenu = pgMenuFromString("New|Open...|Recent    >|Save|Save As...|Quit");

   /* what happened? */
   switch (iMainMenu)
   {
      //case 0: /*nothing!*/ break;
      case 1:
      // TODO: 1. if modified then ask to save, 2. reset document
            for ( i=0; i<dr; i++ )
               pgDelete ( wRow[i] );
            pgDelete ( wHead );
            break;
      case 2: fileName = pgFilePicker(NULL,NULL,NULL,PG_FILEOPEN,"Open a File");
            if ( fileName != NULL )
               loadFile ( (char*)fileName );
            break;
      //case 3:
         // TODO: get documents in menu
         //iRecentMenu = pgMenuFromString("1...|2...|3...");
         // TODO: open the one
      //   break;
      case 4:
      // FIXME:save file with current name, if noname let flow to save as...
      case 5: fileName = pgFilePicker(NULL,NULL,NULL,PG_FILESAVE,"Save a File");
            break;
      case 6:
            //TODO: if modified then ask to save
            if ( pgMessageDialog("µSpread - EXIT?",
                                 "Sure to exit?",
                                 PG_MSGBTN_YES | PG_MSGBTN_NO)
                     == PG_MSGBTN_YES )
               exit(0);
            break;
      //default: no need yet
   }
   return 0;
}

int main(int argc, char *argv[])
{
   pghandle wToolbar, wScroll, temp, fileMenu;
   pghandle fnt;

   /**** PicoGUI initialization and --pg* arguments ****/
   pgInit(argc,argv);

   /**** Register our application ****/
   pgRegisterApp(PG_APP_NORMAL,"µSpread",0);
   //FIXME: fnt = pgNewFont ( "Lucida", 10, PG_FSTYLE_DEFAULT);

   /**** <widgets where="top level"> ****/

   /* A normal (non-standalone) toolbar */
   wToolbar = pgNewWidget(PG_WIDGET_TOOLBAR,0,0);

   /* table column headers */
   wHBox = pgNewWidget(PG_WIDGET_BOX,0,0);
   pgSetWidget(PGDEFAULT,
            PG_WP_SIDE, PG_S_TOP,
            0);

   /* Scrollable box */

   wScroll = pgNewWidget(PG_WIDGET_SCROLL,0,0);
   wBox = pgNewWidget(PG_WIDGET_BOX,0,0);
   pgSetWidget(PGDEFAULT,
            PG_WP_SIDE,PG_S_ALL,
            0);
   pgSetWidget(wScroll,PG_WP_BIND,wBox,0);

   /**** </widgets> ****/

   /**** <widgets where="toolbar"> ****/

   /* menu */
   fileMenu = pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,wToolbar);
   pgSetWidget(PGDEFAULT,
               PG_WP_TEXT,pgNewString("File"),
               PG_WP_SIDE,PG_S_LEFT,
               PG_WP_EXTDEVENTS,PG_EXEV_PNTR_DOWN,
               0);
   pgBind(PGDEFAULT,PG_WE_PNTR_DOWN,&handleFileMenu,NULL);

   /**** </widgets> ****/

   /**** A handler to confirm closing the app ****/
   pgBind(PGBIND_ANY,PG_WE_CLOSE,&closeboxHandler,NULL);

   /**** Run it! ****/
   pgEventLoop();

   free ( data );
   return 0;
}
