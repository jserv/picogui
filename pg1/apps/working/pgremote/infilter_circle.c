#include <picogui.h>
#include <math.h>

int main(int argc, char **argv) {
  struct pgmodeinfo mi;
  union pg_client_trigger trig;
  float a;
  memset(&trig,0,sizeof(trig));

  pgInit(argc,argv);

  mi = *pgGetVideoMode();
  trig.content.type = PG_TRIGGER_MOVE;
  trig.content.u.mouse.cursor_handle = pgNewCursor();
   
  for (a=0;;a+=0.1) {
    trig.content.u.mouse.x = cos(a)*50 + (mi.lxres>>1);
    trig.content.u.mouse.y = sin(a)*50 + (mi.lyres>>1);
  
    pgInFilterSend(&trig);
    pgFlushRequests();
    usleep(100);
  }

  return 0;
}
