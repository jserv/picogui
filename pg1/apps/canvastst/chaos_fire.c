#include <picogui.h>
#include <math.h>

pgcontext gc;
const float pi = 3.1415926;


float radius(float a, int t) {
  return sin(a*5+t*0.08)*0.3+1 + sin(a*42+t*0.7)*0.05;
}

void animate(void) {
  const float step = 0.05;
  static int t = 0;
  float a,r1,r2;
  pgSetMapping(gc,0,0,1000,1000,PG_MAP_SQUARESCALE);

  /* Fade and blur */
  if (!(t&3)) {
    pgSetColor(gc,PGCF_ALPHA | PGC_BLACK | (4<<24));
    pgSetLgop(gc,PG_LGOP_ALPHA);
    pgRect(gc,0,0,1000,1000);
    pgSetLgop(gc,PG_LGOP_NONE);
  }
  pgBlur(gc,0,0,1000,1000,2);

  /* Draw our polar function defined above */
  pgSetColor(gc, 0xFF7344); 
  for (a=0;a<2*pi;a+=step) {
    r1 = radius(a,t)*300;
    r2 = radius(a+step,t)*300;
    pgLine(gc,cos(a)*r1+500,sin(a)*r1+500,
	   cos(a+step)*r2+500,sin(a+step)*r2+500);
  }
  
  pgContextUpdate(gc);
  t++;
}

int main(int argc,char **argv) {
  pgcontext bg;

  pgInit(argc,argv);
  pgRegisterApp(PG_APP_NORMAL,"Burning cinder fury of crimson chaos fire",0);
  pgNewWidget(PG_WIDGET_CANVAS,0,0);

  bg = pgNewCanvasContext(PGDEFAULT,PGFX_PERSISTENT);
  pgSetMapping(bg,0,0,100,100,PG_MAP_SCALE);
  pgSetColor(bg, 0);
  pgRect(bg,0,0,100,100);

  gc = pgNewCanvasContext(PGDEFAULT,PGFX_IMMEDIATE);
  pgUpdate();
  while (1) {
    animate();
    pgEventPoll();
  }
}

