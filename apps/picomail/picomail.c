#include <picogui.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "imap.h"
#include "configfile.h"

#define CONFIG_FILE "/.picomail.conf"

pghandle wBox, lastItem, picomailapp, mesgviewer, messagebox;
int row;
int selectedMessage = -1;

int
closeboxHandler (struct pgEvent *evt)
{
  return 0;
}

int
viewerCloseboxHandler (struct pgEvent *evt)
{
  pgSetWidget (picomailapp, PG_WP_SIZE, 0x7FFF, 0);
  pgSetWidget (mesgviewer, PG_WP_SIZE, 0, 0);
  return 1;
}


int
deleteMessage (struct pgEvent *evt)
{
  pgMessageDialog ("PicoMail",
		   "This function is not implemented (yet).", PG_MSGBTN_OK);
  return 0;
}

int
readMessage (struct pgEvent *evt)
{
  char *message;

  if (selectedMessage > 0)
    {
      pgSetWidget (mesgviewer, PG_WP_SIZE, 0x7FFF, 0);
      pgSetWidget (picomailapp, PG_WP_SIZE, 0, 0);
      message = imap_getmesg (selectedMessage);
      pgSetWidget (messagebox, PG_WP_TEXT, pgNewString (message), 0);
    }
  else
    pgMessageDialog ("PicoMail",
		     "Please select a message first!", PG_MSGBTN_OK);

  return 0;
}

int
getList (struct pgEvent *evt)
{
  imap_getlist ();
  return 0;
}

int
setSelected (struct pgEvent *evt)
{
  selectedMessage = (int) (evt->extra);
  //printf ("Message %d selected.\n", selectedMessage);
  return 0;
}

void
addheader (int msg, char *sender, char *subject, char *date)
{
  lastItem = pgNewWidget (PG_WIDGET_LISTITEM,
			  row ? PGDEFAULT : PG_DERIVE_INSIDE,
			  row ? lastItem : wBox);

  pgReplaceTextFmt (lastItem, "[%s] %s - %s",date, subject, sender);
  pgBind (lastItem, PG_WE_ACTIVATE, &setSelected, (void *) (msg));
  pgEventPoll ();

  row++;
}

int
main (int argc, char *argv[])
{
  char *home, *conffile;

  pghandle msgscroll, msgbox;
  pghandle wToolbar, wScroll, wItem;

  row = 0;

  pgInit (argc, argv);
  picomailapp = pgRegisterApp (PG_APP_NORMAL, "PicoMail", 0);

  home = getenv ("HOME");
  conffile = malloc (strlen (home) + strlen (CONFIG_FILE) + 1);
  if (home && conffile)
    {
      strcpy (conffile, home);
      strcat (conffile, CONFIG_FILE);

      if (!configfile_parse (conffile))
	{
	  pgMessageDialog ("PicoMail",
			   "Configuration file could not be parsed.\n"
			   "Please make sure you have a good '~/.picomail.conf'!",
			   PG_MSGBTN_OK || PG_MSGICON_ERROR);
	  exit (1);
	}
      free (conffile);
    }

  wToolbar = pgNewWidget (PG_WIDGET_TOOLBAR, 0, 0);



  wScroll = pgNewWidget (PG_WIDGET_SCROLL, 0, 0);
  wBox = pgNewWidget (PG_WIDGET_BOX, 0, 0);
  pgSetWidget (PGDEFAULT, PG_WP_SIDE, PG_S_ALL, 0);
  pgSetWidget (wScroll, PG_WP_BIND, wBox, 0);




  pgNewWidget (PG_WIDGET_BUTTON, PG_DERIVE_INSIDE, wToolbar);
  pgSetWidget (PGDEFAULT,
	       PG_WP_TEXT, pgNewString ("Read Message"),
	       PG_WP_SIDE, PG_S_LEFT, PG_WP_EXTDEVENTS, PG_EXEV_PNTR_DOWN, 0);
  pgBind (PGDEFAULT, PG_WE_PNTR_DOWN, &readMessage, NULL);


  pgNewWidget (PG_WIDGET_BUTTON, PG_DERIVE_INSIDE, wToolbar);
  pgSetWidget (PGDEFAULT,
	       PG_WP_TEXT, pgNewString ("Delete Message"),
	       PG_WP_SIDE, PG_S_LEFT, PG_WP_EXTDEVENTS, PG_EXEV_PNTR_DOWN, 0);
  pgBind (PGDEFAULT, PG_WE_PNTR_DOWN, &deleteMessage, NULL);


  pgNewWidget (PG_WIDGET_BUTTON, PG_DERIVE_INSIDE, wToolbar);
  pgSetWidget (PGDEFAULT,
	       PG_WP_TEXT, pgNewString ("Get list of messages!"),
	       PG_WP_SIDE, PG_S_LEFT, PG_WP_EXTDEVENTS, PG_EXEV_PNTR_DOWN, 0);
  pgBind (PGDEFAULT, PG_WE_PNTR_DOWN, &getList, NULL);

  pgBind (PGBIND_ANY, PG_WE_CLOSE, &closeboxHandler, NULL);

/* message viewer */

  mesgviewer = pgRegisterApp (PG_APP_NORMAL, "PicoMail Message", 0);
  pgBind (mesgviewer, PG_WE_CLOSE, &viewerCloseboxHandler, NULL);
  pgSetWidget (mesgviewer, PG_WP_SIZE, 0, 0);
  msgscroll = pgNewWidget (PG_WIDGET_SCROLL, 0, 0);
  msgbox = pgNewWidget (PG_WIDGET_BOX, 0, 0);
  pgSetWidget (msgbox, PG_WP_SIDE, PG_S_ALL, 0);
  pgSetWidget (msgscroll, PG_WP_BIND, msgbox, 0);
  messagebox = pgNewWidget (PG_WIDGET_TEXTBOX, PG_DERIVE_INSIDE, PGDEFAULT);
//        pgSetWidget(msgscroll, PG_WP_BIND,messagebox, 0);
  pgSetWidget (messagebox,
	       PG_WP_TEXT, pgNewString ("Hier is uw bericht!"), 0);



  pgEventLoop ();

  configfile_free ();
  return 0;
}
