/*
 * $Id$
 *
 * Small test appllication displaying buttons allowing the user to send various
 * commands to the virtual keyboard.
 *
 * -- Christian Grigis <christian.grigis@smartdata.ch>
 *
 */

#include <netinet/in.h>
#include <picogui.h>
#include "pgboard.h"

/*
 * Send a message to the keyboard application
 *
 * msg : pointer to the command to send
 */
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

/*
 * Handler for all the buttons
 * The actual keyboard message to send is passed through evt->extra.
 */
int handleButton (struct pgEvent * evt)
{
  sendMsg (evt->extra);

  return 1;
}

/*
 * main
 */
int main (int argc, char * argv [])
{
  /* Constants to facilitate passing commands through pgBind() */
  struct keyboard_command cmd_show           = {htons (PG_KEYBOARD_SHOW)};
  struct keyboard_command cmd_hide           = {htons (PG_KEYBOARD_HIDE)};
  struct keyboard_command cmd_toggle         = {htons (PG_KEYBOARD_TOGGLE)};
  struct keyboard_command cmd_enable         = {htons (PG_KEYBOARD_ENABLE)};
  struct keyboard_command cmd_disable        = {htons (PG_KEYBOARD_DISABLE)};
  struct keyboard_command cmd_toggle_display = {htons (PG_KEYBOARD_TOGGLE_DISPLAY)};
  struct keyboard_command cmd_select_pattern = {htons (PG_KEYBOARD_SELECT_PATTERN), htons (PG_KBPATTERN_NUMERIC)};
  struct keyboard_command cmd_push_context   = {htons (PG_KEYBOARD_PUSH_CONTEXT)};
  struct keyboard_command cmd_pop_context    = {htons (PG_KEYBOARD_POP_CONTEXT)};
  struct keyboard_command cmd_block          = {htons (PG_KEYBOARD_BLOCK)};
  struct keyboard_command cmd_release        = {htons (PG_KEYBOARD_RELEASE)};

  /* Handle for GUI placement */
  pghandle lastBox;

  /* Handle for font object */
  pghandle font;

  /* Init and register the app */
  pgInit (argc, argv);
  pgRegisterApp (PG_APP_NORMAL, "Virtual Keyboard Commands", 0);

  font = pgNewFont ("Helvetica", 8, 0);

  /* Create all the widgets */
  lastBox = pgNewWidget (PG_WIDGET_BOX, 0, 0);

  pgNewWidget (PG_WIDGET_BUTTON, PG_DERIVE_INSIDE, PGDEFAULT);	
  pgSetWidget (PGDEFAULT,
	       PG_WP_TEXT, pgNewString ("SHOW"),
	       PG_WP_SIDE, PG_S_LEFT,
	       PG_WP_FONT, font,
	       0);
  pgBind (PGDEFAULT, PG_WE_ACTIVATE, &handleButton, &cmd_show);

  pgNewWidget (PG_WIDGET_BUTTON, PG_DERIVE_AFTER, PGDEFAULT);
  pgSetWidget (PGDEFAULT,
	       PG_WP_TEXT, pgNewString ("HIDE"),
	       PG_WP_SIDE, PG_S_LEFT,
	       PG_WP_FONT, font,
	       0);
  pgBind (PGDEFAULT, PG_WE_ACTIVATE, &handleButton, &cmd_hide);

  pgNewWidget (PG_WIDGET_BUTTON, PG_DERIVE_AFTER, PGDEFAULT);
  pgSetWidget (PGDEFAULT,
	       PG_WP_TEXT, pgNewString ("TOGGLE"),
	       PG_WP_SIDE, PG_S_LEFT,
	       PG_WP_FONT, font,
	       0);
  pgBind (PGDEFAULT, PG_WE_ACTIVATE, &handleButton, &cmd_toggle);

  lastBox = pgNewWidget (PG_WIDGET_BOX, PG_DERIVE_AFTER, lastBox);

  pgNewWidget (PG_WIDGET_BUTTON, PG_DERIVE_INSIDE, PGDEFAULT);	
  pgSetWidget (PGDEFAULT,
	       PG_WP_TEXT, pgNewString ("ENABLE"),
	       PG_WP_SIDE, PG_S_LEFT,
	       PG_WP_FONT, font,
	       0);
  pgBind (PGDEFAULT, PG_WE_ACTIVATE, &handleButton, &cmd_enable);

  pgNewWidget (PG_WIDGET_BUTTON, PG_DERIVE_AFTER, PGDEFAULT);	
  pgSetWidget (PGDEFAULT,
	       PG_WP_TEXT, pgNewString ("DISABLE"),
	       PG_WP_SIDE, PG_S_LEFT,
	       PG_WP_FONT, font,
	       0);
  pgBind (PGDEFAULT, PG_WE_ACTIVATE, &handleButton, &cmd_disable);

  pgNewWidget (PG_WIDGET_BUTTON, PG_DERIVE_AFTER, PGDEFAULT);	
  pgSetWidget (PGDEFAULT,
	       PG_WP_TEXT, pgNewString ("TOGGLE_ENABLE"),
	       PG_WP_SIDE, PG_S_LEFT,
	       PG_WP_FONT, font,
	       0);
  pgBind (PGDEFAULT, PG_WE_ACTIVATE, &handleButton, &cmd_toggle_display);

  lastBox = pgNewWidget (PG_WIDGET_BOX, PG_DERIVE_AFTER, lastBox);

  pgNewWidget (PG_WIDGET_BUTTON, PG_DERIVE_INSIDE, PGDEFAULT);	
  pgSetWidget (PGDEFAULT,
	       PG_WP_TEXT, pgNewString ("SELECT_PATTERN NUMERIC"),
	       PG_WP_SIDE, PG_S_LEFT,
	       PG_WP_FONT, font,
	       0);
  pgBind (PGDEFAULT, PG_WE_ACTIVATE, &handleButton, &cmd_select_pattern);

  pgNewWidget (PG_WIDGET_BUTTON, PG_DERIVE_AFTER, PGDEFAULT);	
  pgSetWidget (PGDEFAULT,
	       PG_WP_TEXT, pgNewString ("PUSH_CONTEXT"),
	       PG_WP_SIDE, PG_S_LEFT,
	       PG_WP_FONT, font,
	       0);
  pgBind (PGDEFAULT, PG_WE_ACTIVATE, &handleButton, &cmd_push_context);

  pgNewWidget (PG_WIDGET_BUTTON, PG_DERIVE_AFTER, PGDEFAULT);	
  pgSetWidget (PGDEFAULT,
	       PG_WP_TEXT, pgNewString ("POP_CONTEXT"),
	       PG_WP_SIDE, PG_S_LEFT,
	       PG_WP_FONT, font,
	       0);
  pgBind (PGDEFAULT, PG_WE_ACTIVATE, &handleButton, &cmd_pop_context);

  lastBox = pgNewWidget (PG_WIDGET_BOX, PG_DERIVE_AFTER, lastBox);

  pgNewWidget (PG_WIDGET_BUTTON, PG_DERIVE_INSIDE, PGDEFAULT);	
  pgSetWidget (PGDEFAULT,
	       PG_WP_TEXT, pgNewString ("BLOCK"),
	       PG_WP_SIDE, PG_S_LEFT,
	       PG_WP_FONT, font,
	       0);
  pgBind (PGDEFAULT, PG_WE_ACTIVATE, &handleButton, &cmd_block);

  pgNewWidget (PG_WIDGET_BUTTON, PG_DERIVE_AFTER, PGDEFAULT);	
  pgSetWidget (PGDEFAULT,
	       PG_WP_TEXT, pgNewString ("RELEASE"),
	       PG_WP_SIDE, PG_S_LEFT,
	       PG_WP_FONT, font,
	       0);
  pgBind (PGDEFAULT, PG_WE_ACTIVATE, &handleButton, &cmd_release);

  font = pgNewFont ("Helvetica", 12, 0);

  pgNewWidget (PG_WIDGET_FIELD, PG_DERIVE_AFTER, lastBox);
  pgSetWidget (PGDEFAULT,
	       PG_WP_SIZE, 16,
	       PG_WP_FONT, font,
	       0);

  /* Wait for events */
  pgEventLoop ();

  return 0;
}
