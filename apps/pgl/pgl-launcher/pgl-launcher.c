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
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>

#include "pgl-launcher.h"
#include "configfile.h"

char *appDir;
char *confPath;

int directoryScan(char *path){
  int appCount = 0, appCopy = 0;
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
      gAppList = (struct pgllApp *)malloc((sizeof(struct pgllApp)*appCount));
      rewinddir(d);
      while((dent = readdir(d))){
	sprintf(absPath, "%s/%s/app.conf", path, dent->d_name);
	if(configfile_parse(absPath)){
	  if((binpath = get_param_str(architecture, "binpath", NULL))){
	    gAppList[appCopy].appPath = (char *)malloc(strlen(binpath)+1);
	    strcpy(gAppList[appCopy].appPath, binpath);
	  }else{
	    gAppList[appCopy].appPath = NULL;
	  }
	  if((appname = get_param_str(architecture, "appname", NULL))){
	    gAppList[appCopy].appName = (char *)malloc(strlen(appname)+1);
	    strcpy(gAppList[appCopy].appName, appname);
	  }else{
	    gAppList[appCopy].appName = NULL;
	  }
	  if((appicon = get_param_str(architecture, "iconpath", NULL))){
	    gAppList[appCopy].appIcon = (char *)malloc(strlen(appicon)+1);
	    strcpy(gAppList[appCopy].appIcon, appname);
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
  free(app->appName);
  free(app->appPath);
  free(app->appIcon);
}

void parseConfig(char *configfile){
  char *appDirTmp;

  configfile_parse("launcher.conf");
  appDirTmp = get_param_str("Applications", "appdirPath", "/usr/local/apps");
  appDir = strdup(appDirTmp);
  configfile_free();
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
  pghandle cancel, ok;
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
      configfile_parse("launcher.conf");
      set_param_str("Applications", "appdirPath", pgGetString(pgGetWidget(appPath,PG_WP_TEXT)));
      configfile_write("launcher.conf");
      configfile_free();
      //We need to refresh our config data now
      parseConfig("launcher.conf");
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

  items = alloca(sizeof(pghandle) * gAppCount+2);

  for(item = 0; item < gAppCount; item++){
    items[item] = pgNewString(gAppList[item].appName);
  }

  //Add the standard options
  items[gAppCount] = pgNewString("Rescan Application Directory");
  items[gAppCount+1] = pgNewString("Preferences...");

  item = pgMenuFromArray(items, gAppCount+2);

  if(item){
    if(item == gAppCount+2){
      setPreferences();
    }else if(item == gAppCount+1){
      directoryScan(appDir);
    }else{
      runApp(gAppList[item-1].appPath);
    }
  }

  pgLeaveContext();
 
  return 1;
}

int main(int argc, char **argv){
  pghandle pglBar;
  int freeLoop;

  pgInit(argc, argv);

  //Open our config file and parse it
  parseConfig("launcher.conf");

  //Find applications
  directoryScan(appDir);

  //Install into the toolbar
  pglBar = pgFindWidget("PGL-AppletBar");
  if(!pglBar){
    pgMessageDialog(argv[0], "This applet requires PGL",PG_MSGICON_ERROR);
    return 1;
  }

  pgNewWidget(PG_WIDGET_BUTTON, PG_DERIVE_INSIDE, pglBar);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_LEFT,
	      PG_WP_EXTDEVENTS,PG_EXEV_PNTR_DOWN,
	      PG_WP_TEXT,pgNewString("Launch"),
	      0);
  pgBind(PGDEFAULT, PG_WE_PNTR_DOWN, &launchMenu, NULL);

  pgEventLoop();

  for(freeLoop = 0; freeLoop < gAppCount; freeLoop++){
    freeApplication(&gAppList[freeLoop]);
  }
 
  free(gAppList);
  
  return 0;
}
	      



