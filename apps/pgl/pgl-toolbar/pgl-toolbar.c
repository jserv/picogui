/* 
 * pgl-toolbar.c -  PGL toolbar
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

#include <stdlib.h>
#include <string.h>
#include <picogui.h>

#include "applet.h"
#include "configfile.h"

char configFilePath[] = "toolbar.conf";

void setPref(char *section, char *key, char *data){

  set_param_str(section, key, data);
  configfile_write(configFilePath);
}

char *getPref(char *section, char *key){
  
  return (char *)get_param_str(section, key, "NOTSET");
}

int messageHandler(struct pgEvent *evt){
  pglMessage *appletMessage;
  char *sender = NULL, *key = NULL, *data = NULL, *response = NULL;
  pghandle applet;

  pgEnterContext();

  appletMessage = malloc(evt->e.data.size);
  memcpy(appletMessage, evt->e.data.pointer, evt->e.data.size);
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
  }

  free(appletMessage);

  pgLeaveContext();

  return 1;
}

int main(int argc, char **argv) {
  pgInit(argc,argv);

  if(configfile_parse("toolbar.conf")){
    
    pgRegisterApp(PG_APP_TOOLBAR,"PGL Toolbar",0);
    pgSetWidget(PGDEFAULT,
		PG_WP_PUBLICBOX,1,
		PG_WP_NAME,pgNewString("PGL-AppletBar"),
		0);
    pgBind(PGDEFAULT, PG_WE_APPMSG, &messageHandler, NULL);

    pgEventLoop();
    
    configfile_free();

    return 1;
  }else{
    printf("Could not load config file, exiting\n");
    
    return 0;
  }

  return 1;
}
