/* 
 * pgl-launcher.c - Simple launcher for PGL
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2002 Daniel Jackson <carpman@voidptr.org>
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
#include <sys/utsname.h>
#include <string.h>
#include <res_c/res_c.h>

#include "pgl-launcher.h"
//#include "linkedList.h"
//#include "configfile.h"

#define PGL_VERSION 1

pgllApp **gAppList = NULL;
int gAppCount = 0;

char appDir[] = "/usr/share/picogui/apps";
char *confPath = NULL;
char *toolbarResponse = NULL;
char *architecture = NULL;
pghandle pglBar;

void directoryScan(char *path);

void emptyList(void);
void freeApplication(pgllApp *app);
void childDied(int foo);
int runApp(pgllApp *app);
int setPreferences(void);
int launchMenu(struct pgEvent *evt);
void loadPreferences(void);
int recieveMessage(struct pgEvent *evt);
char **buildArgv(char *exename, char *optionString);

void directoryScan(char *path){
  char **appPaths = NULL;
  pgllApp *apps = NULL;
  int appCount = resGetAppCount(path);
  resResource *appRes = NULL;

  if(appCount){
    if(gAppList)
      emptyList();
    gAppCount = appCount;
    appPaths = resGetAppPaths(path);
    gAppList = malloc(sizeof(pgllApp *)*appCount);
    for(;appCount > 0; appCount--){
      appRes = resLoadResource(appPaths[appCount-1]);
      gAppList[appCount-1] = malloc(sizeof(pgllApp));
      gAppList[appCount-1]->appName = resGetProperty(appRes, "PGL-Launcher", "name", NULL);
      gAppList[appCount-1]->basePath = strdup(appRes->workingDir);
      if(!(gAppList[appCount-1]->exeName = resGetProperty(appRes, architecture, "executable", NULL))){
	gAppList[appCount-1]->exeName = resGetProperty(appRes, "any-arch", "executable", NULL);
      }
      if(!(gAppList[appCount-1]->args = resGetProperty(appRes, architecture, "args", NULL))){
	gAppList[appCount-1]->args = resGetProperty(appRes, "any-arch", "args", NULL);
      }
    }
  }
}

void emptyList(void){
  if(gAppList){
    for(; gAppCount > 0; gAppCount--){
      freeApplication(gAppList[gAppCount-1]);
    }
    free(gAppList);
    gAppList = 0;
  }
}


void freeApplication(pgllApp *app){
  if(app->basePath)
    free(app->basePath);
  if(app->appName)
    free(app->appName);
  if(app->exeName)
    free(app->exeName);
  if(app->args)
    free(app->args);
  free(app);
}

void childDied(int foo){ 

  waitpid(-1, NULL, WNOHANG); 
}

int runApp(pgllApp *app){
  char *finalPath = malloc(strlen(app->exeName)+strlen(app->basePath)+1);
  char *argStr = NULL;
  char *arg; 
  char **args = malloc(sizeof(char *)*2);
  int argCount = 1;

  if(app->args)
    argStr = strdup(args);

  //Count arguments
  if(argStr){
    while(arg = strtok(argStr, " "))
      argCount++;
    
    //Make array
    if(args)
      free(args);
    args = malloc(sizeof(char *)*argCount+1);
    free(argStr);

    argStr = strdup(app->args);
    argCount = 1;
  
    //Split
    while(arg = strtok(argStr, " ")){
      args[argCount] = strdup(arg);
      argCount++;
    }
  }

  sprintf(finalPath, "%s%s", app->basePath, app->exeName);
  args[0] = strdup(finalPath);
  args[argCount] = NULL;

  signal(SIGCHLD, childDied);
  if(!vfork()){
    execv(finalPath, args);
  }
  
  //Clean up
  for(;argCount > 0; argCount--)
    free(args[argCount-1]);
  if(argStr)
    free(argStr);
  //free(args);
  free(finalPath);

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

  /*appPath = pgNewWidget(PG_WIDGET_FIELD, PG_DERIVE_INSIDE, optionBox);
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
	      PG_WP_TEXT,pgGetServerRes(PGRES_STRING_CANCEL),
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
	      PG_WP_TEXT,pgGetServerRes(PGRES_STRING_OK),
	      PG_WP_SIDE,PG_S_RIGHT,
	      PG_WP_HOTKEY,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,
					 PGTH_P_HOTKEY_OK),
	      PG_WP_BITMAP,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,
					 PGTH_P_ICON_OK),
	      PG_WP_BITMASK,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,
					  PGTH_P_ICON_OK_MASK),
					  0);*/

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
  int appCount, item;

  pgEnterContext();

  directoryScan(appDir);

  appCount = gAppCount;
  items = alloca(sizeof(pghandle) * appCount+1);
  for(; appCount > 0; appCount--){
    printf("appName: %s\n", gAppList[appCount-1]->appName);
    items[appCount-1] = pgNewString(gAppList[appCount-1]->appName);
  }

  //Add the standard options
  items[gAppCount] = pgNewString("Preferences...");

  item = pgMenuFromArray(items, gAppCount+1);

  if(item){
    if(item == gAppCount+1){
      setPreferences();
    }else{
      printf("ExeName: %s\n", gAppList[item-1]->exeName);
      runApp(gAppList[item-1]);
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
  //appDir = strdup(toolbarResponse);

  //We probably should re-scan our app dir now
  directoryScan(appDir);
}

int recieveMessage(struct pgEvent *evt){
  pglMessage *inMessage, *inMessageTmp;
  char *data;
  
  /*pgEnterContext();

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

  pgLeaveContext();*/

  return 1;
}

int main(int argc, char **argv){
  pghandle pglButton;
  struct utsname unameInfo;
  int freeLoop;
  int gotPrefs = 0;

  toolbarResponse = NULL;
  //appDir = NULL;

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

  //Get our arch name
  uname(&unameInfo);
  architecture = strdup(unameInfo.machine);

  pgAppMessage(pglBar, pglBuildMessage(PGL_APPLETINSTALLED, "PGL-Launcher", "", ""));

  pgEventLoop();

  return 0;
}
	      



