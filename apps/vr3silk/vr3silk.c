#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <ctype.h>
#include <picogui.h>
#include "commands.h"

/* execute a command in a subprocess */
void spawn_process (const char *command)
{
  if (fork() == 0)
    {
      execl ("/bin/sh", "/bin/sh", "-c", command, NULL);
      _exit (EXIT_FAILURE);
    }
}

void pgboard ()
{
  static int pgboard_on = 0;

  if (pgboard_on) {
    system(commands[7]);
    pgboard_on = 0;
  }
  else {
    spawn_process(commands[6]);
    pgboard_on = 1;
  }
}

int tap_filter (struct pgEvent * evt)
{
  union pg_client_trigger *trig = evt->e.data.trigger;

  /* We should have a PG_TRIGGER_CHAR event now, print out the character value
   */
  switch (trig->content.u.kbd.key) {
  case PGKEY_F1:
    spawn_process(commands[0]);
    break;
  case PGKEY_F2:
    spawn_process(commands[1]);
    break;
  case PGKEY_F3:
    spawn_process(commands[2]);
    break;
  case PGKEY_F4:
    spawn_process(commands[3]);
    break;
  case PGKEY_F5:
    spawn_process(commands[4]);
    break;
  case PGKEY_F6:
    spawn_process(commands[5]);
    break;
  case PGKEY_F7:
    pgboard();
    break;
  default:
    pgInFilterSend(trig);
    break;
  }

  return 0;
}

int main(int argc,char **argv) {
  
  pgInit(argc,argv);

  pgNewInFilter(pgGetServerRes(PGRES_INFILTER_KEY_PREPROCESS),
		PG_TRIGGER_KEYDOWN, PG_TRIGGER_KEYDOWN);

  pgBind(PGBIND_ANY, PG_NWE_INFILTER, tap_filter, NULL);

  pgEventLoop();
  return 0;
}

