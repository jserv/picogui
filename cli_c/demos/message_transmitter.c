/* This transmits messages to the message_receiver.c program */

#include <picogui.h>

int evtSend(struct pgEvent *evt);

int main(int argc, char **argv) {
  pgInit(argc,argv);
  pgRegisterApp(PG_APP_NORMAL,"Message Transmitter",0);

  /* Build a little UI for sending messages */
  pgNewWidget(PG_WIDGET_LABEL,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("To send a message to:\n"
				     "demos/message_receiver/app\n"
				     "Type it below and press enter\n"),
	      0);
  pgNewWidget(PG_WIDGET_FIELD,0,0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,evtSend,0);

  pgEventLoop();
}

/* Send the contents of the field to the receiver */
int evtSend(struct pgEvent *evt) {
  const char *msg;
  pghandle dest;

  /* Find the receiver */
  dest = pgFindWidget("demos/message_receiver/app");

  if (!dest) {
    pgMessageDialog("Message Transmitter", "Can't find message reciever!", 0);
    return 0;
  }

  /* get a dynamically allocated copy of the field's text */
  msg = strdup(pgGetString(pgGetWidget(evt->from,PG_WP_TEXT)));
  
  /* Using pgTempMemory, it will be freed when the client lib's done with it */
  pgAppMessage(dest,pgFromTempMemory(msg,strlen(msg)));
  
  return 0;
}

