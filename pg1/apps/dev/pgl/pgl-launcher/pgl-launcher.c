/* 
 * pgl-launcher.c - Simple launcher for PGL
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Daniel Jackson <carpman@voidptr.org>
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

#include "pgl-launcher.h"
#include "linkedList.h"
#include "configfile.h"

#define PGL_VERSION 1

pgoLinkedListNode *gAppList = NULL;
char *appDir = NULL;
char *confPath = NULL;
char *toolbarResponse = NULL;
char *architecture = NULL;
pghandle pglBar;

int directoryScan(char *path);
pgllApp *parseApp(char *appConfPath, int *appCount);

void emptyList(void);
void freeApplication(pgllApp *app);
void childDied(int foo);
int runApp(pgllApp *app);
int setPreferences(void);
int launchMenu(struct pgEvent *evt);
void loadPreferences(void);
int recieveMessage(struct pgEvent *evt);
char **buildArgv(char *exename, char *optionString);

int parse_line(char* line, char* demchar, char*** arglist, int type){
  char* t;
  char* snew;
  int numtokens;
  int i;
   
  /*snew is real start of string after skipping leading demchar*/
  snew=line+strspn(line, demchar);
   
  if ((t=calloc(strlen(snew)+1, sizeof(char)))==NULL){
    *arglist=NULL;
    numtokens=-1;
    return numtokens;
  }
  //count the number of tokens in snew
  strcpy(t, snew);
  if (strtok(t, demchar)==NULL){
    numtokens=0;
  }else{
    for(numtokens=1; strtok(NULL, demchar)!=NULL; numtokens++);
  }
  /*create an argument array to contain ptrs to tokens*/
  if((*arglist=calloc(numtokens+1, sizeof(char*)))==NULL){
    free(t);
    numtokens=-1;
    return numtokens;
  }else{             //insert pointers to tokens into the array
    if (numtokens>0){
      if (type == 1)
	strcpy(t, snew);
      else {
	free(t);
	t = snew;
      }
      **arglist=strtok(t, demchar);
      for(i=1; i<numtokens+1; i++)
	*((*arglist)+i)=strtok(NULL, demchar);
    } else {
      **arglist=NULL;
      free(t);
      return numtokens;
    }
  }
  return numtokens;
}
 
int directoryScan(char *path){
  int appCount, listInsert;
  pgllApp *newApp;
  char *absPath;
  struct dirent *dent;
  DIR *d;
 
  if(!architecture)
    architecture = strdup("Default");
 
  if(!path)
    return 0;
 
  if(pgoLLListLength(gAppList) > 0){
    emptyList();
  }
 
  d = opendir(path);
 
  if(d){
    while((dent = readdir(d))){
      absPath = malloc(sizeof(path)+sizeof(dent->d_name)+strlen("app.conf")+3);
      strcpy(absPath, path);
      strcat(absPath, "/");
      strcat(absPath, dent->d_name);
      strcat(absPath, "/app.conf");
 
      newApp = parseApp(absPath, &appCount);
      if(newApp)
	for(listInsert = 0; listInsert < appCount; listInsert++){
	  pgoLLAddRecord(gAppList, newApp[listInsert].variantName, &newApp[listInsert], sizeof(pgllApp));
	}
 
      free(absPath);
    }
  }
}
 
pgllApp *parseApp(char *appConfPath, int *appCount){
  char **variantListing, *basePath;
  int variantCount, moveLoop;
  pgllApp *newApp, *currentApp;
   
  if(configfile_parse(appConfPath)){
    char **variantListing, *basePath;
    int variantCount;
     
    if(atoi(get_param_str("PGL-Launcher", "version", "-1")) == PGL_VERSION){
      basePath = get_param_str("Application", "basePath", "/usr/local/apps");
      variantCount = parse_line(get_param_str(architecture, "variantList", NULL),
				",",
				&variantListing,
				1);
 
      newApp = malloc(sizeof(pgllApp)*variantCount);
 
      if(appCount)
	*appCount = variantCount;
      variantCount--;
 
      while(variantCount >= 0){
	currentApp = &newApp[variantCount];
	currentApp->basePath = strdup(basePath);
	currentApp->appName = strdup(get_param_str(variantListing[variantCount], "name", "Unnamed App"));
	currentApp->exeName = strdup(get_param_str(variantListing[variantCount], "binary", ""));
	currentApp->variantName = strdup(variantListing[variantCount]);
	currentApp->optionCount = parse_line(get_param_str(variantListing[variantCount], "options", ""),
					     " ",
					     &currentApp->appOptions,
					     1);
	/* Account for arg[0] and the NULL at the end*/
	currentApp->optionCount+=2;
 
	/* appOptions[0] needs to be the binary name */
	currentApp->appOptions = realloc(currentApp->appOptions, 
					 (currentApp->optionCount)*sizeof(char *));
	for(moveLoop = currentApp->optionCount; moveLoop >= 0; moveLoop--){
	  if(moveLoop == 0){
	    currentApp->appOptions[0] = currentApp->exeName;
	  }else if(moveLoop == currentApp->optionCount){
	    currentApp->appOptions[moveLoop] = NULL;
	  }else{
	    currentApp->appOptions[moveLoop] = currentApp->appOptions[moveLoop-1];
	  }
	} 
	variantCount--;
      }
    }else{
      printf("This config is too old\n");
      configfile_free();
      return NULL;
    }
    configfile_free();
    return newApp;
  }else{
    return NULL;
  }
}

void emptyList(void){
  int keyCount, freeLoop;
  pgoNodeIdentifier *keys = pgoLLListIdentifiers(gAppList, &keyCount);
  pgllApp *app;
 
  printf("KEY: %s\n", keys[0]);
 
  if(keys){
    for(freeLoop = 0; freeLoop < keyCount; freeLoop++){
      app = pgoLLGetRecord(gAppList, keys[freeLoop], NULL);
      freeApplication(app);
      pgoLLDeleteRecord(gAppList, keys[freeLoop]);
    }
    free(keys);
  }
}


void freeApplication(pgllApp *app){
int optionFree;
 
if(app->basePath)
     free(app->basePath);
     if(app->appName)
     free(app->appName);
     free(app);
     if(app->variantName)
     free(app->variantName);
 
     /* app->appOptions[0] is a pointer to exeName, so this takes care of exeName as well */
     for(optionFree = 0; optionFree < app->optionCount; optionFree++){
if(app->appOptions[optionFree])
     free(app->appOptions[optionFree]);
     }
   
free(app->appOptions);
}

void childDied(int foo){ 

  waitpid(-1, NULL, WNOHANG); 
}

int runApp(pgllApp *app){
  char *finalPath = malloc(strlen(app->exeName)+strlen(app->basePath)+1);

  strcpy(finalPath, app->basePath);
  strcat(finalPath, app->exeName);

  signal(SIGCHLD, childDied);

  if(!vfork()){
    execv(finalPath, app->appOptions);
  }
  
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
  pgoNodeIdentifier *nodeList;
  pgllApp *app;
  int item;

  /* Make a neat little scrolling popup menu for our apps, with some decoration */
  pgEnterContext();
  pgNewPopupAt(PG_POPUP_ATEVENT, PG_POPUP_ATEVENT, 0, 0);
  pgSetPayload(PGDEFAULT,0);

  pgNewWidget(PG_WIDGET_LABEL,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT, pgNewString("PicoGUI"),
	      PG_WP_TRANSPARENT, 0,
	      PG_WP_THOBJ, PGTH_O_PANELBAR_V,
	      PG_WP_SIDE, PG_S_LEFT,
	      PG_WP_DIRECTION, PG_DIR_VERTICAL,
	      0);
  
  pgNewWidget(PG_WIDGET_SCROLLBOX,0,0);

  nodeList = pgoLLListIdentifiers(gAppList, NULL);
  for(item = 0; item < pgoLLListLength(gAppList); item++){
    app = pgoLLGetRecord(gAppList, nodeList[item], NULL);
    pgNewWidget(PG_WIDGET_MENUITEM, item ? PG_DERIVE_AFTER : PG_DERIVE_INSIDE,0);
    pgSetWidget(PGDEFAULT,
		PG_WP_TEXT, pgNewString(app->appName),
		0);
    pgSetPayload(PGDEFAULT, item+1);
  }

  // Add the standard options
  pgNewWidget(PG_WIDGET_MENUITEM, 0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT, pgNewString("Preferences..."),
	      0);
  pgSetPayload(PGDEFAULT,++item);

  /* Get an event */
  item = pgGetPayload(pgGetEvent()->from);

  if(item){
    if(item == pgoLLListLength(gAppList)+1){
      setPreferences();
    }else{
      printf("nodeList: %s\n", nodeList[item-1]);
      app = pgoLLGetRecord(gAppList, nodeList[item-1], NULL);
      if(app){
	printf("ExeName: %s\n", app->exeName);
	runApp(app);
      }
    }
  }

  free(nodeList);

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
  
  inMessageTmp = (pglMessage *)evt->e.data.pointer;
  inMessage = malloc(evt->e.data.size);
  memcpy(inMessage, inMessageTmp, evt->e.data.size);

  pgEnterContext();

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
  struct utsname unameInfo;
  int freeLoop;
  int gotPrefs = 0;

  toolbarResponse = NULL;
  appDir = NULL;
  gAppList = pgoLLInit();

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
//	      PG_WP_THOBJ,pgFindThemeObject("button.launch"),
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
	      



