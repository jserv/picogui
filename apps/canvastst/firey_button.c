#include <picogui.h>
#include <math.h>

pgcontext gc;
const float pi = 3.1415926;


float radius(float a, int t) {
  return sin(a*5+t*0.03)*0.3+1 + sin(a*42+t*0.7)*0.05;
}

void animate(void) {
  const float step = 0.05;
  static int t = 0;
  float a,r1,r2;
  pgSetMapping(gc,0,0,64,64,PG_MAP_SQUARESCALE);

  /* Fade and blur */
  if (!(t&3)) {
    pgSetColor(gc,PGCF_ALPHA | PGC_BLACK | (4<<24));
    pgSetLgop(gc,PG_LGOP_ALPHA);
    pgRect(gc,0,0,64,64);
    pgSetLgop(gc,PG_LGOP_NONE);
  }
  pgBlur(gc,0,0,64,64,2);

  /* Draw our polar function defined above */
  pgSetColor(gc, 0xFF7344); 
  for (a=0;a<2*pi;a+=step) {
    r1 = radius(a,t)*14;
    r2 = radius(a+step,t)*14;
    pgLine(gc,cos(a)*r1+32,sin(a)*r1+32,
	   cos(a+step)*r2+32,sin(a+step)*r2+32);
  }
  
  pgContextUpdate(gc);
  t++;
}

int main(int argc,char **argv) {
  pgcontext bg;
  pghandle bitmap, button, mask;

  pgInit(argc,argv);
  pgRegisterApp(PG_APP_NORMAL,"Burning cinder fury of crimson chaos fire",0);

  bitmap = pgCreateBitmap(64,64);
  mask = pgCreateBitmap(64,64);

  gc = pgNewBitmapContext(mask);
  pgSetColor(gc,0xFFFFFF);
  pgRect(gc,0,0,64,64);
  pgSetColor(gc,0x000000);
  pgFEllipse(gc,8,8,48,48);
  pgDeleteContext(gc);

  gc = pgNewBitmapContext(bitmap);
  pgSetColor(gc,0x000000);
  pgRect(gc,0,0,64,64);

  button = pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_BITMAP,bitmap,
	      PG_WP_BITMASK,mask,
	      PG_WP_SIDE, PG_S_TOP,
	      PG_WP_TEXT, pgNewString("Evil animated button of doom!"),
	      0);

  while (1) {
    animate();
    pgSetWidget(button,
		PG_WP_BITMAP,bitmap,
		0);
    pgUpdate();
    pgEventPoll();
  }
}

