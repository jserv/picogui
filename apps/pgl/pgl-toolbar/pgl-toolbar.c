/* Simulate the PGL applet container */

#include <stdlib.h>
#include <string.h>
#include <picogui.h>

#include "applet.h"

char configFilePath[] = "toolbar.conf";

void setPref(char *section, char *key, char *data){

  if(configfile_parse(configFilePath)){
    set_param_str(section, key, data);
    configfile_write(configFilePath);
    configfile_free();
  }
}

char *getPref(char *section, char *key){
  char *responseTmp = NULL, *response = NULL;
  
  if(configfile_parse(configFilePath)){
    responseTmp = (char *)get_param_str(section, key, "NOTSET");
    response = (char *)strdup(responseTmp);
    configfile_free();
  }else{
    response = (char *)strdup("NOCONF");
  }

  return response;
}

int messageHandler(struct pgEvent *evt){
  pglMessage *appletMessage;
  char *sender = NULL, *key = NULL, *data = NULL, *response = NULL;
  pghandle applet;

  pgEnterContext();

  appletMessage = malloc(evt->e.data.size);
  memcpy(appletMessage, evt->e.data.pointer, evt->e.data.size);
  appletMessage = (pglMessage *)alignMessageData(appletMessage);
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
    free(sender);
    free(key);
    free(data);
    break;
  case PGL_GETPREF:
    sender = pglGetMessageData(appletMessage, 0);
    key = pglGetMessageData(appletMessage, appletMessage->senderLen+1);
    response = getPref(sender, key);
    pgAppMessage(pgFindWidget(sender), 
		 pglBuildMessage(PGL_GETPREF, "PGL-AppletBar", key, response));
    free(response);
    free(sender);
    free(key);
    break;
  }

  pgLeaveContext();

  return 1;
}

int main(int argc, char **argv) {
  pgInit(argc,argv);
  pgRegisterApp(PG_APP_TOOLBAR,"PGL Toolbar",0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_PUBLICBOX,1,
	      PG_WP_NAME,pgNewString("PGL-AppletBar"),
	      0);
  pgBind(PGDEFAULT, PG_WE_APPMSG, &messageHandler, NULL);
  pgEventLoop();
}
