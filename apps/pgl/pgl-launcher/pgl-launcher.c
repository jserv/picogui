/* 
 * pgl-launcher.c - Simple launcher for PGL
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000,2001 Daniel Jackson <carpman@voidptr.org>
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
 * Contributors:
 * 
 * 
 * 
 */

#include <picogui.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>

#include "pgl-launcher.h"
#include "applet.h"
#include "configfile.h"

struct pgllApp *gAppList = NULL;
int gAppCount;
char *appDir;
char *confPath;
char *toolbarResponse;
pghandle pglBar;

int directoryScan(char *path);
void freeApplication(struct pgllApp *app);
void childDied(int foo);
int runApp(char *appPath);
int setPreferences(void);
int launchMenu(struct pgEvent *evt);
void loadPreferences(void);
int recieveMessage(struct pgEvent *evt);

int directoryScan(char *path){
  int appCount = 0, appCopy = 0, freeLoop;
  char architecture[] = "Application";  //DJ: For now.
  char *appname, *binpath, *appicon;
  char absPath[1024];              //DJ: THIS MUST BE FIXED!!!
  struct dirent *dent;
  DIR *d;

  if(!path)
    return 0;

  d = opendir(path);

  if(d){
    while((dent = readdir(d))){
      sprintf(absPath, "%s/%s/app.conf", path, dent->d_name);
      if(configfile_parse(absPath)){
	appCount++;
	configfile_free();
      }
    }
    
    if(appCount > 0){
      if(gAppList){
	  for(freeLoop = 0; freeLoop < gAppCount; freeLoop++){
	    freeApplication(&gAppList[freeLoop]);
	  }      
	  free(gAppList);
      }
      gAppList = (struct pgllApp *)malloc((sizeof(struct pgllApp)*appCount));
      rewinddir(d);
      while((dent = readdir(d))){
	sprintf(absPath, "%s/%s/app.conf", path, dent->d_name);
	if(configfile_parse(absPath)){
	  if((binpath = (char *)get_param_str(architecture, "binpath", NULL))){
	    gAppList[appCopy].appPath = strdup(binpath);
	  }else{
	    gAppList[appCopy].appPath = NULL;
	  }
	  if((appname = get_param_str(architecture, "appname", NULL))){
	    gAppList[appCopy].appName = strdup(appname);
	  }else{
	    gAppList[appCopy].appName = NULL;
	  }
	  if((appicon = get_param_str(architecture, "iconpath", NULL))){
	    gAppList[appCopy].appIcon = strdup(appicon);
	  }else{
	    gAppList[appCopy].appIcon = NULL;
	  }
	  appCopy++;
	  configfile_free();
	}
      }
      return gAppCount = appCount;
    }else{
      return 0;
    }
  }
}

void freeApplication(struct pgllApp *app){
  if(app->appName)
    free(app->appName);
  if(app->appPath)
    free(app->appPath);
  if(app->appIcon)
    free(app->appIcon);
}

void childDied(int foo){ 

  waitpid(-1, NULL, WNOHANG); 
}

int runApp(char *appPath){
  
  signal(SIGCHLD, childDied);

  if(!vfork()){
    execl(appPath, NULL);
  }
  
  return 1;
}

int setPreferences(void){
  pghandle toolbar, optionBox;
  pghandle appPath;
  pghandle rescan, cancel, ok;
  struct pgEvent evt;

  pgEnterContext();

  pgDialogBox("Launcher Preferences");
  
  toolbar = pgNewWidget(PG_WIDGET_TOOLBAR, 0, 0);
  pgSetWidget(toolbar, PG_WP_SIDE, PG_S_BOTTOM, 0);

  optionBox = pgNewWidget(PG_WIDGET_BOX, 0, 0);
  pgNewWidget(PG_WIDGET_SCROLL, PG_DERIVE_BEFORE, optionBox);
  pgSetWidget(PGDEFAULT, PG_WP_BIND, optionBox, 0);

  appPath = pgNewWidget(PG_WIDGET_FIELD, PG_DERIVE_INSIDE, optionBox);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT, pgNewString(appDir),
	      0);
  pgNewWidget(PG_WIDGET_LABEL, PG_DERIVE_INSIDE, optionBox);
  pgSetWidget(PGDEFAULT, 
	      PG_WP_TEXT, pgNewString("App path:"), 
	      0);

  rescan = pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,toolbar);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Rescan App Dir"),
	      PG_WP_SIDE,PG_S_LEFT,
	      0);

  cancel = pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,toolbar);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,
				       PGTH_P_STRING_CANCEL),
	      PG_WP_SIDE,PG_S_RIGHT,
	      PG_WP_HOTKEY,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,
					 PGTH_P_HOTKEY_CANCEL),
	      PG_WP_BITMAP,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,
					 PGTH_P_ICON_CANCEL),
	      PG_WP_BITMASK,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,
					  PGTH_P_ICON_CANCEL_MASK),
	      0);
  ok = pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,toolbar);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,
				       PGTH_P_STRING_OK),
	      PG_WP_SIDE,PG_S_RIGHT,
	      PG_WP_HOTKEY,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,
					 PGTH_P_HOTKEY_OK),
	      PG_WP_BITMAP,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,
					 PGTH_P_ICON_OK),
	      PG_WP_BITMASK,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,
					  PGTH_P_ICON_OK_MASK),
	      0);

  for(;;){
    evt = *pgGetEvent();            /* Wait for an event */

    if(evt.from == cancel){
      break;
    }else if(evt.from == ok){
      pgAppMessage(pglBar, pglBuildMessage(PGL_STOREPREF, 
					   "PGL-Launcher", 
					   "appdir", 
					   pgGetString(pgGetWidget(appPath,PG_WP_TEXT))));
      //We need to refresh our config data now
      loadPreferences();
      break;
    }else if(evt.from == rescan){
      directoryScan(appDir);
      break;
    }
  }
  pgLeaveContext();

  return 0;
}

int launchMenu(struct pgEvent *evt){
  pghandle *items;
  int item;

  pgEnterContext();

  items = alloca(sizeof(pghandle) * gAppCount+1);

  for(item = 0; item < gAppCount; item++){
    items[item] = pgNewString(gAppList[item].appName);
  }

  //Add the standard options
  items[gAppCount] = pgNewString("Preferences...");

  item = pgMenuFromArray(items, gAppCount+1);

  if(item){
    if(item == gAppCount+1){
      setPreferences();
    }else{
      runApp(gAppList[item-1].appPath);
    }
  }

  pgLeaveContext();
 
  return 1;
}

void loadPreferences(void){

  pgAppMessage(pglBar, pglBuildMessage(PGL_GETPREF, "PGL-Launcher", "appdir", ""));
  recieveMessage(pgGetEvent());
  if(appDir)
    free(appDir);
  appDir = strdup(toolbarResponse);

  //We probably should re-scan our app dir now
  directoryScan(appDir);
}

int recieveMessage(struct pgEvent *evt){
  pglMessage *inMessage, *inMessageTmp;
  char *data;
  
  pgEnterContext();

  inMessageTmp = (pglMessage *)evt->e.data.pointer;
  inMessage = malloc(evt->e.data.size);
  memcpy(inMessage, inMessageTmp, evt->e.data.size);
  inMessage = pglDecodeMessage(inMessage);
  switch(inMessage->messageType){
  case PGL_LOADPREFS:
    loadPreferences();
    break;
  case PGL_GETPREF:
    if(toolbarResponse)
      free(toolbarResponse);
    data = pglGetMessageData(inMessage, (inMessage->senderLen+inMessage->keyLen)+2);
    toolbarResponse = strdup(data);
    break;
  }

  free(inMessage);

  pgLeaveContext();

  return 1;
}

int main(int argc, char **argv){
  pghandle pglButton;
  int freeLoop;
  int gotPrefs = 0;

  toolbarResponse = NULL;
  appDir = NULL;

  pgInit(argc, argv);

  //Install into the toolbar
  pglBar = pgFindWidget("PGL-AppletBar");
  if(!pglBar){
    pgMessageDialog(argv[0], "This applet requires PGL",PG_MSGICON_ERROR);
    return 1;
  } 

  //Create our widget
  pglButton = pgNewWidget(PG_WIDGET_BUTTON, PG_DERIVE_INSIDE, pglBar);
  pgSetWidget(pglButton,
	      PG_WP_SIDE,PG_S_LEFT,
	      PG_WP_EXTDEVENTS,PG_EXEV_PNTR_DOWN,
	      PG_WP_TEXT,pgNewString("Launch"),
	      PG_WP_NAME,pgNewString("PGL-Launcher"),
	      0);

  pgBind(pglButton, PG_WE_APPMSG, &recieveMessage, NULL);
  pgBind(pglButton, PG_WE_PNTR_DOWN, &launchMenu, NULL);

  pgAppMessage(pglBar, pglBuildMessage(PGL_APPLETINSTALLED, "PGL-Launcher", "", ""));

  pgEventLoop();

  for(freeLoop = 0; freeLoop < gAppCount; freeLoop++){
    freeApplication(&gAppList[freeLoop]);
  }
 
  free(gAppList);
  
  return 0;
}
	      



