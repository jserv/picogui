#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
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

int tapped (struct pgEvent * evt)
{
  spawn_process((const char*)evt->extra);

  return 1;
}

int pgboard (struct pgEvent * evt)
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

  return 1;
}

void addbutton(const int key, const int num)
{
  pgCreateWidget(PG_WIDGET_BUTTON);
  pgSetWidget(PGDEFAULT,
	       PG_WP_HOTKEY, key,
	       0);
  pgBind(PGDEFAULT, PGBIND_ANY, &tapped, commands[num]);
}

int main(int argc,char **argv) {
  
  pgInit(argc,argv);

  addbutton(PGKEY_F1, 0);
  addbutton(PGKEY_F2, 1);
  addbutton(PGKEY_F3, 2);
  addbutton(PGKEY_F4, 3);
  addbutton(PGKEY_F5, 4);
  addbutton(PGKEY_F6, 5);

  /* pgboard - start/kill */
  pgCreateWidget(PG_WIDGET_BUTTON);
  pgSetWidget(PGDEFAULT,
	       PG_WP_HOTKEY, PGKEY_F7,
	       0);
  pgBind(PGDEFAULT, PGBIND_ANY, &pgboard, NULL);

  pgEventLoop();
  return 0;
}

