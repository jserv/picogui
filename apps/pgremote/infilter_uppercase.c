#include <picogui.h>
#include <ctype.h>

int myFilter(struct pgEvent *evt) {
  union pg_client_trigger *trig = evt->e.data.trigger;
  char c;

  /* We should have a PG_TRIGGER_CHAR event now, print out the character value
   */
  c = trig->content.u.kbd.key;
  if (c=='\r') c = '\n';
  write(0,&c,1);

  /* Force the input to uppercase, then retransmit it where it left off
   */
  trig->content.u.kbd.key = toupper(trig->content.u.kbd.key);
  pgInFilterSend(trig);

  return 0;
}


int main(int argc, char **argv) {
  pgInit(argc,argv);

  /* Insert our input filter after the KEY_PREPROCESS filter,
   * get a copy of all CHAR triggers, and don't automatically pass them through
   */
  pgNewInFilter(pgGetServerRes(PGRES_INFILTER_KEY_PREPROCESS),
		PG_TRIGGER_CHAR, PG_TRIGGER_CHAR);

  pgBind(PGBIND_ANY, PG_NWE_INFILTER, myFilter, NULL);

  pgEventLoop();
  return 0;
}
