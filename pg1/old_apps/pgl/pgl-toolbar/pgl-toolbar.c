/* 
 * pgl-toolbar.c -  PGL toolbar
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

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <picogui.h>

#include "configfile.h"

char configFilePath[] = "toolbar.conf";

void setPref(char *section, char *key, char *data){

  set_param_str(section, key, data);
  configfile_write(configFilePath);
}

char *getPref(char *section, char *key){
  
  return (char *)get_param_str(section, key, "NOTSET");
}

void reloadPrefs(void){
  
  configfile_free();
  configfile_parse("/etc/toolbar.conf");
}

int messageHandler(struct pgEvent *evt){
  pglMessage *appletMessage;
  char *sender = NULL, *key = NULL, *data = NULL, *response = NULL;
  pghandle applet;
  appletMessage = malloc(evt->e.data.size);
  memcpy(appletMessage, evt->e.data.pointer, evt->e.data.size);

  pgEnterContext();

  appletMessage = pglDecodeMessage(appletMessage);
  switch(appletMessage->messageType){
  case PGL_APPLETINSTALLED:
    sender = pglGetMessageData(appletMessage, 0);
    pgAppMessage(pgFindWidget(sender), 
		 pglBuildMessage(PGL_LOADPREFS, "PGL-AppletBar", "", ""));
    break;
  case PGL_STOREPREF:
    sender = pglGetMessageData(appletMessage, 0);
    key = pglGetMessageData(appletMessage, appletMessage->senderLen+1);
    data = pglGetMessageData(appletMessage, appletMessage->senderLen+appletMessage->keyLen+2);
    setPref(sender, key, data);
    break;
  case PGL_GETPREF:
    sender = pglGetMessageData(appletMessage, 0);
    key = pglGetMessageData(appletMessage, appletMessage->senderLen+1);
    response = getPref(sender, key);
    pgAppMessage(pgFindWidget(sender), 
		 pglBuildMessage(PGL_GETPREF, "PGL-AppletBar", key, response));
    break;
  case PGL_LOADAPPLET:
    sender = pglGetMessageData(appletMessage, 0);
    key = pglGetMessageData(appletMessage, appletMessage->senderLen+1);
    launchApplet(key);
    break;
  case PGL_RELOADPREFS:
    reloadPrefs();
    break;
  }

  free(appletMessage);

  pgLeaveContext();

  return 1;
}

void childDied(int foo){ 

  waitpid(-1, NULL, WNOHANG); 
}

int launchApplet(char *appPath){
  
  if(!vfork()){
    execl(appPath, NULL);
  }
  
  return 1;
}

int main(int argc, char **argv) {
  int appletCount, appletLoop;
  char **appletList;
  pghandle toolbar;

  pgInit(argc,argv);

  signal(SIGCHLD, childDied);

  if(configfile_parse("/etc/toolbar.conf")){
    
    toolbar = pgRegisterApp(PG_APP_TOOLBAR,"PGL Toolbar",0);
    pgSetWidget(toolbar,
		PG_WP_PUBLICBOX,1,
		PG_WP_NAME,pgNewString("PGL-AppletBar"),
		0);
    pgBind(toolbar, PG_WE_APPMSG, &messageHandler, NULL);

    appletList = get_section_params("PGL-Toolbar Applets", &appletCount);

    if(appletList){
      for(appletLoop = 0; appletLoop < appletCount; appletLoop++){
	pgAppMessage(toolbar, pglBuildMessage(PGL_LOADAPPLET, "PGL-Launcher", appletList[appletLoop], ""));
      }
      free(appletList);
    }else{
      printf("No applets!\n");
    }

    pgEventLoop();
    
    configfile_free();

    return 1;
  }else{
    printf("Could not load config file, exiting\n");
    
    return 0;
  }

  return 1;
}
