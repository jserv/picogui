/*
 *
 * uspread.c -   micro-spread (sheet) application for PicoGUI
 *
 * Copyright (C) 2001 Tasnim Ahmed <tasnim@users.sourceforge.net>
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

#include <picogui.h>
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEF_W 25

pghandle wBox, *wRow, wHead, wHBox;
char **data;
int dc, dr;
int isModified = 1;  /* 0=false, 1=true */
int save = 1;  /* 1=save, 2=save as */
char *fileName;

//FIXME: PicoGUI bug: does not return PG_WP_SIZE so store them till its fixed

int *widths;

int textEdit ( struct pgEvent *evt )
{
	printf ( "MODIFY\n" );
	isModified = 1;
}

void loadFile ( )
{
   int i, j;
   char *strWName = malloc( 15 );
   char *strTmp = malloc( 15 );

   csv_info ( fileName, &dc, &dr );
   data = (char **) malloc ( dc*dr*sizeof(char*) );
   isModified = 0;
   parse_csv ( fileName, data, dc, dr);

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
      if ( data[j] != NULL && data[j+dc] != NULL && atoi ( data[j+dc] ) != 0 )
         pgSetWidget(PGDEFAULT,
                  PG_WP_TEXT, pgNewString( data[j] ),
                  PG_WP_SIZE, atoi( data[j+dc] ),
                  PG_WP_SIZEMODE, PG_SZMODE_PIXEL,
                  0);
      else
         if ( data[j] == NULL )
         {
            if ( data[j+dc] == NULL ||  atoi ( data[j+dc] ) == 0)
               pgSetWidget(PGDEFAULT,
                        PG_WP_TEXT, pgNewString( ":)" ),
                        PG_WP_SIZE, DEF_W,
                        PG_WP_SIZEMODE, PG_SZMODE_PIXEL,
                        0);
            else
               pgSetWidget(PGDEFAULT,
                        PG_WP_TEXT, pgNewString( ":)" ),
                        PG_WP_SIZE, atoi( data[j+dc] ),
                        PG_WP_SIZEMODE, PG_SZMODE_PIXEL,
                        0);
         }
         else
            pgSetWidget(PGDEFAULT,
                     PG_WP_TEXT, pgNewString( data[j] ),
                     PG_WP_SIZE, DEF_W,
                     PG_WP_SIZEMODE, PG_SZMODE_PIXEL,
                     0);

            sprintf( strWName, "HEAD.%d.", j );
            pgSetWidget ( PGDEFAULT,
                  PG_WP_SIDE, PG_S_LEFT,
                  PG_WP_NAME, pgNewString ( strWName ),
                  0 );
   }
   /**** </widgets> ****/

   /**** <widgets where="client area"> ****/
   for ( i=(dr-1); i>=2; i-- )
   {
      wRow [i-2] = pgNewWidget(PG_WIDGET_BOX,PG_DERIVE_INSIDE,wBox);
      pgSetWidget(PGDEFAULT,
                  PG_WP_SIDE, PG_S_TOP,
                  0);

      for ( j=dc-1; j >= 0; j-- )
      {
         pgNewWidget(PG_WIDGET_FIELD,PG_DERIVE_INSIDE,wRow[i-2]);
         // FIXME: pgBind ( PGDEFAULT, PG_WE_ACTIVATE, &textEdit, NULL );

         if ( data[j+(i*dc)] != NULL && data[j+dc] != NULL )
            pgSetWidget(PGDEFAULT,
                     PG_WP_TEXT, pgNewString( data[j+(i*dc)] ),
                     PG_WP_SIZE, atoi( data[j+dc] ),
                     0);
         else
            if ( data[j+(i*dc)] == NULL )
            {
               if ( data[j+dc] == NULL )
                  pgSetWidget(PGDEFAULT,
                           PG_WP_TEXT, pgNewString( ":)" ),
                           PG_WP_SIZE, DEF_W,
                           0);
               else
                  pgSetWidget(PGDEFAULT,
                           PG_WP_TEXT, pgNewString( ":)" ),
                           PG_WP_SIZE, atoi( data[j+dc] ),
                           0);
            }
            else
               pgSetWidget(PGDEFAULT,
                        PG_WP_TEXT, pgNewString( data[j+(i*dc)] ),
                        PG_WP_SIZE, DEF_W,
                        0);
         // in any case:
         // set widget name to CELL.c.r wher c=col no. nad r=row no.
         sprintf( strWName, "CELL.%d.%d.", j, i-2 );
         pgSetWidget ( PGDEFAULT,
                  PG_WP_NAME, pgNewString( strWName ),
                  PG_WP_SIDE, PG_S_LEFT,
                  PG_WP_SIZEMODE, PG_SZMODE_PIXEL,
                  0);
      }
    }
   /**** </widgets> ****/
   //FIXME:
   widths = (int *) malloc ( dc * sizeof ( int ) );
   for ( i=0; i<dc; i++ )
   	widths [ i ] = atoi ( data [ i + dc ] );
   free ( data );
}

void saveFile ( )
{
	char *oldFile;
	char *tmp;
        FILE *fle;
        pghandle ctrl;
        char *strWName;
        char *strTmp;
        pghandle ctrlStr;

        int i, j;

        if ( fileName == NULL || save == 2 ) {
              oldFile = (char *) pgFilePicker(NULL,NULL,NULL,PG_FILESAVE,"µSpread - Save File As");
              if ( oldFile == NULL ) {
                	pgMessageDialog ("µSpread", "File Not Saved!", PGDEFAULT );
                        return;
              }
                 strcpy ( fileName, oldFile );
        }

        // if filename already exists: Want to override?
	if ( save == 2 ) {
                fle = fopen ( fileName, "r" );
                if ( fle != NULL ) {
                        if ( pgMessageDialog ( "µSpread", "Override Existing File?", PG_MSGBTN_YES|PG_MSGBTN_NO ) == PG_MSGBTN_NO ) {
                                pgMessageDialog ("µSpread", "File Not Saved!", PGDEFAULT );
                                return;
                        }
                        fclose ( fle );
                }
 	}
//SAVE IT
tmp = (char *) malloc ( 5120 );
        fle = fopen ( fileName, "wt+" );
        for ( i=0; i < dr; i++ ) {
        	strcpy ( tmp, "" );
                for ( j=0; j < dc; j++ ) {
                        if ( i != 1 ) {
                        	strcat ( tmp, "\"" );
                                if ( i != 0 ) {
                                         sprintf( strWName, "CELL.%d.%d.", j, i-2 );
                                         ctrl = pgFindWidget ( strWName );
                                         ctrlStr = pgGetWidget ( ctrl, PG_WP_TEXT );
                                         strcat ( tmp, pgGetString ( ctrlStr ) );
                                }
                                else {
                                         sprintf( strWName, "HEAD.%d.", j );
                                         ctrl = pgFindWidget ( strWName );
                                         ctrlStr = pgGetWidget ( ctrl, PG_WP_TEXT );
                                         strcat ( tmp, pgGetString ( ctrlStr ) );
                                }
                                strcat ( tmp, "\"" );
                        }
                        else {
                                sprintf( strWName, "HEAD.%d.", j );
                                ctrl = pgFindWidget ( strWName );
                                sprintf ( strTmp, "%d", widths [ j ] ); //pgGetWidget ( ctrl, PG_WP_SIZE ) );
                        	strcat ( tmp, strTmp );
                        }
                        if ( j != ( dc -1 ) )
                        	strcat ( tmp, "," );
        	}
                strcat ( tmp, "\n" );
                fputs ( tmp, fle );
	}
      fclose ( fle );
}

int closeboxHandler(struct pgEvent *evt) {
  /* Present a dialog box. If the user doesn't want to close,
     return 1 to prevent further handling of the event */

    if ( isModified )
       	if ( pgMessageDialog ( "µSpread", "Save Modified File?", PG_MSGBTN_YES | PG_MSGBTN_NO ) == PG_MSGBTN_YES)
               	saveFile ( );
  return 0;
  /*pgMessageDialog("µSpread - EXIT?",
         "Sure to exit?",
         PG_MSGBTN_YES | PG_MSGBTN_NO)
    == PG_MSGBTN_NO;*/
}

void newSheet ( ) {
	int i;
        if ( isModified == 1)
                if ( pgMessageDialog ( "µSpread", "Save Modified File?", PG_MSGBTN_YES | PG_MSGBTN_NO ) == PG_MSGBTN_YES)
                        saveFile ( );
        for ( i=0; i<dr; i++ )
                pgDelete ( wRow[i] );
        pgDelete ( wHead );
        fileName = NULL;
}

int handleFileMenu(struct pgEvent *evt) {
   int iMainMenu, iRecentMenu, i;

   iMainMenu = pgMenuFromString("New|Open...|Recent    >|Save|Save As...|Quit");

   /* what happened? */
   switch (iMainMenu)
   {
      //case 0: /*nothing!*/ break;
      case 1:
      // 1. if modified then ask to save, 2. reset document
	    //printf ( "%d\n", isModified );
            	newSheet ( );
                break;
      case 2:
      		newSheet ( );
      		fileName = (char *) pgFilePicker(NULL,NULL,NULL,PG_FILEOPEN,"µSpread - Open File");
            	if ( fileName != NULL ) {
                        loadFile ( );
                        //FIXME: see textEdit ( )
                        isModified = 1;
             	}
            	break;
      case 3:
                // TODO: get documents in menu
                iRecentMenu = pgMenuFromString("1...|2...|3...");
                // TODO: open the one
                // strcpy ( fileName, ... );
                // loadFile ( );
      		break;
      case 5:
      		save = 2;
                saveFile ( );
                break;
      case 4:
      		save = 1;
    		saveFile ( );
                break;
      case 6:
	        closeboxHandler ( NULL );
                exit(0);
                break;
      //default: no need yet
   }
   return 0;
}

int handleInsertMenu(struct pgEvent *evt) {
   int iMenu;

   iMenu = pgMenuFromString("Row|Column");

   /* what happened? */
   switch (iMenu)
   {
      case 0:
      		break;
      case 1:
      		break;
      //default: no need yet
   }
   return 0;
}

int main(int argc, char *argv[])
{
   pghandle wToolbar, wScroll, temp;
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
   pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,wToolbar);
   pgSetWidget(PGDEFAULT,
               PG_WP_TEXT,pgNewString("Append"),
               PG_WP_SIDE,PG_S_LEFT,
               PG_WP_EXTDEVENTS,PG_EXEV_PNTR_DOWN,
               0);
   pgBind(PGDEFAULT,PG_WE_PNTR_DOWN,&handleInsertMenu,NULL);

   pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,wToolbar);
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
   free ( widths );
   return 0;
}
