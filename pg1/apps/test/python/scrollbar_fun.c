/* Test for PicoGUI Python client library. If Python batch requests
 * (via hold()/flush()) are working, this C version shouldn't be
 * much faster than scrollbar_fun.py
 */

#include <picogui.h>
#include <math.h>

int main(int argc, char **argv) {
  pghandle w[40];
  pghandle app;
  int i;
  float t;
  
  pgInit(argc,argv);
  app = pgRegisterApp(PG_APP_NORMAL,"Scrollbar Fun - C",0);
  
  for (i=0;i<40;i++) {
    w[i] = pgNewWidget(PG_WIDGET_SCROLL,PG_DERIVE_INSIDE,app);
    pgSetWidget(PGDEFAULT,
		PG_WP_SIDE, PG_S_LEFT,
		0);
  }
  
  t = 0;
  while (1) {
    t += 0.04;
    for (i=0;i<40;i++)
      pgSetWidget(w[i],
		  PG_WP_VALUE, (int)(50 + 50 * sin(i*0.2+t)),
		  0);
    pgUpdate();
  }

  return 0;
}
    
