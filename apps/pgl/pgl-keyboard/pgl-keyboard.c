#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>

#include <picogui.h>

#define PG_KEYBOARD_TOGGLE          3  /* Toggle between SHOW and HIDE */
#define PG_KEYBOARD_APPNAME "app/Keyboard"

/* Data structure for a keyboard command */

struct keyboard_command
{
  unsigned short type;    /* Network byte order, 16-bits */

  /* Command-dependent data */
  union
  {
    unsigned short pattern;
  } data;
};

char *pglToolbarResponse;
pghandle pglButton, pglToolbar;

int recieveMessage(struct pgEvent *evt);

void sendMsg (struct keyboard_command * cmd)
{
  if (cmd)
    {
      struct pgmemdata data = {cmd, sizeof (struct keyboard_command), 0};
      pghandle kb = pgFindWidget (PG_KEYBOARD_APPNAME);

      if (kb)
	{
	  pgAppMessage (kb, data);
	}
      else
	{
	  printf ("Can't find keyboard app\n");
	}
    }
}

int handleButton (struct pgEvent * evt)
{
  sendMsg (evt->extra);

  return 1;
}

void loadSettings(void){
  char *dataPath, *bitmapName, *finalPath;
  pghandle bitmapHandle;

  pgAppMessage(pglToolbar, pglBuildMessage(PGL_GETPREF, 
					   "PGL-Keyboard", 
					   "dataPath", 
					   ""));
  recieveMessage(pgGetEvent());
  dataPath = strdup(pglToolbarResponse);
  pgAppMessage(pglToolbar, pglBuildMessage(PGL_GETPREF, "PGL-Keyboard", "kbIcon", ""));
  recieveMessage(pgGetEvent());
  bitmapName = strdup(pglToolbarResponse);
  
  finalPath = malloc(strlen(dataPath)+strlen(bitmapName)+1);
  strcpy(finalPath, dataPath);
  strcat(finalPath, bitmapName);

  printf("%s\n", finalPath);

  bitmapHandle = pgNewBitmap(pgFromFile(finalPath));

  printf("%d\n", pglButton);

  pgSetWidget(pglButton,
	      PG_WP_BITMAP, bitmapHandle,
	      0);
}

int recieveMessage(struct pgEvent *evt){
  pglMessage *inMessage, *inMessageTmp;
  char *data;

  inMessageTmp = (pglMessage *)evt->e.data.pointer;
  inMessage = malloc(evt->e.data.size);
  memcpy(inMessage, inMessageTmp, evt->e.data.size);
  inMessage = pglDecodeMessage(inMessage);
  switch(inMessage->messageType){
  case PGL_LOADPREFS:
    loadSettings();
    break;
  case PGL_GETPREF:
    if(pglToolbarResponse)
      free(pglToolbarResponse);
    data = pglGetMessageData(inMessage, (inMessage->senderLen+inMessage->keyLen)+2);
    pglToolbarResponse = strdup(data);
    break;
  }

  free(inMessage);

  return 1;
}

int main(int argc, char **argv){
  struct keyboard_command cmd_toggle         = {htons (PG_KEYBOARD_TOGGLE)};

  pgInit(argc, argv);
  
  /* Find the applet container */
  pglToolbar = pgFindWidget("PGL-AppletBar");
  if (!pglToolbar) {
    pgMessageDialog(argv[0],"This applet requires PGL",PG_MSGICON_ERROR);
    return 1;
  }

  pglButton = pgNewWidget(PG_WIDGET_FLATBUTTON, PG_DERIVE_INSIDE, pglToolbar);
  pgSetWidget (pglButton,
	       PG_WP_NAME, pgNewString("PGL-Keyboard"),
	       PG_WP_SIDE, PG_S_RIGHT,
	       0);
  pgBind(pglButton, PG_WE_APPMSG, &recieveMessage, NULL);
  pgBind(pglButton, PG_WE_ACTIVATE, &handleButton, &cmd_toggle);

  pgAppMessage(pglToolbar, pglBuildMessage(PGL_APPLETINSTALLED, "PGL-Keyboard", "", ""));

  pgEventLoop();
  return 0;
}
