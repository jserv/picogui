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

#include "pgl-launcher.h"
#include "configfile.h"

int directoryScan(char *path){
  int appCount = 0, appCopy = 0;
  char architecture[] = "Application";  //DJ: For now.
  char *appname, *binpath, *appicon;
  char absPath[1024];              //DJ: THIS MUST BE FIXED!!!
  struct dirent *dent;
  DIR *d;

  d = opendir(path);

  if(d){
    while((dent = readdir(d))){
      sprintf(absPath, "%s/%s/app.conf", path, dent->d_name);
      if(configfile_parse(absPath)){
	printf("Got one config.\n");
	appCount++;
	configfile_free();
      }
    }
    
    if(appCount > 0){
      printf("Parsing configs\n");
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

int runApp(char *appPath){
  
  printf("Would execute %s\n", appPath);

  if(!vfork())
    execl(appPath, NULL);
  
  return 1;
}

int addItem(struct pgEvent *evt){return 0;}

int removeItem(struct pgEvent *evt){return 0;}

int launchMenu(struct pgEvent *evt){
  pghandle *items;
  int item;

  printf("Generating menu\n");

  pgEnterContext();

  items = alloca(sizeof(pghandle) * gAppCount);

  for(item = 0; item < gAppCount; item++){
    items[item] = pgNewString(gAppList[item].appName);
  }

  item = pgMenuFromArray(items, gAppCount);

  if(item)
    runApp(gAppList[item-1].appPath);

  pgLeaveContext();
 
  return 1;
}

int main(int argc, char **argv){
  pghandle pglBar;

  pgInit(argc, argv);

  directoryScan("/usr/local/apps");

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
 
  free(gAppList);
  
  return 0;
}
	      



