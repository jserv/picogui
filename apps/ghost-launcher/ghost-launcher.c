#include <picogui.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <picogui.h>

#define MAX_BUTTONS 20
#define FILENAME "/etc/ghost-launcher.conf"

typedef struct {
  int x1;
  int x2;
  int y1;
  int y2;
  char *command;
} button;

button buttons[MAX_BUTTONS];
int last_button;

void
parse_buttons()
{
  int i;
  FILE *f=fopen(FILENAME, "r");

  for (i=0; i<MAX_BUTTONS; i++) {
    if (5 > fscanf(f, "%d %d %d %d %a[^\n]\n",
                   &buttons[i].x1,
                   &buttons[i].x2,
                   &buttons[i].y1,
                   &buttons[i].y2,
                   &buttons[i].command))
      break;
  }
  last_button = i-1;
}

/* Find the button given the clicked coordinates */
button * find_clicked_button (int x, int y)
{
  int i;

  for (i=0; i<=last_button; i++)
    if (x >= buttons[i].x1 && y >= buttons[i].y1 &&
        x <= buttons[i].x2 && y <= buttons[i].y2)
      return &buttons[i];

  return NULL;
}

/* execute a command in a subprocess */
void spawn_process (const char *command)
{
  if (fork() == 0)
    {
      execl ("/bin/sh", "/bin/sh", "-c", command, NULL);
      _exit (EXIT_FAILURE);
    }
}

int tap_filter (struct pgEvent * evt)
{
  union pg_client_trigger *trig = evt->e.data.trigger;

  button *b;
  b = find_clicked_button(trig->content.u.mouse.x, trig->content.u.mouse.y);
  if (b)
    spawn_process(b->command);
  return 0;
}

int
main(int argc, char *argv[])
{
  parse_buttons();
  pgInit(argc,argv);

  pgNewInFilter(pgGetServerRes(PGRES_INFILTER_TOUCHSCREEN),
		PG_TRIGGER_UP, PG_TRIGGER_UP);

  pgBind(PGBIND_ANY, PG_NWE_INFILTER, tap_filter, NULL);

  pgEventLoop();
  exit(0);
}
