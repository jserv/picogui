/* Little demo of inter-application messaging. Recieve and display
 * messages sent to this app's named widget.
 */

#include <picogui.h>

pghandle wMessageLbl;

int evtMessage(struct pgEvent *evt);

int main(int argc, char **argv) {
  pgInit(argc,argv);
  pgRegisterApp(PG_APP_NORMAL,"Message Receiver",0);

  /* Receive messages using our app widget */
  pgSetWidget(PGDEFAULT,
	      PG_WP_NAME,pgNewString("demos/message_receiver/app"),
	      0);
  pgBind(PGDEFAULT,PG_WE_APPMSG,&evtMessage,NULL);
  
  /* Make a label to stick the messages in */
  wMessageLbl = pgNewWidget(PG_WIDGET_LABEL,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_LEFT,
	      PG_WP_TEXT,pgNewString("Listening for a PG_WE_APPMSG on:\n"
				     "demos/message_receiver/app"),
	      0);

  pgEventLoop();
}

int evtMessage(struct pgEvent *evt) {
  /* Display some info on the received message */
  pgReplaceTextFmt(wMessageLbl,
		   "Received message: %d bytes\n\"%s\"",
		   evt->e.data.size, evt->e.data.pointer);
  return 0;
}
