/* This is a little graphics hack for PicoGUI */

#include <picogui.h>
#include <stdlib.h>
#include <time.h>

struct star {
  int x,y;    /* Position (fixed point) */
  int ox,oy;  /* Old position */
  int vx,vy;  /* Velocity (fixed point) */
  int life;   /* Number of frames the star has been alive */
};

struct galaxy {
  struct star *stars;
  int numstars;
  pgcolor bg,fg;  /* Background and foreground colors */
  pgcontext gc;
  int w,h;
};

#define PRECISION 8
#define SPEEDVAR  200
#define SPEEDMIN  100

void emit_star(struct galaxy *g, struct star *s);
void init_galaxy(struct galaxy *g, int numstars, pgcontext gc, int w, int h);
void draw_galaxy(struct galaxy *g);
void init_galaxy_fullscreen(struct galaxy *g, int numstars);

/* Reset a star to the emitter location */
void emit_star(struct galaxy *g, struct star *s) {
  s->life = 0;
  s->x = g->w << (PRECISION-1);
  s->y = g->h << (PRECISION-1);
  while (s->vx + s->vy < SPEEDMIN) {
	 s->vx = 1 + (rand()%SPEEDVAR);
	 s->vy = 1 + (rand()%SPEEDVAR);
  }
  if (rand()%2)
    s->vx = -s->vx;
  if (rand()%2)
    s->vy = -s->vy;
}

/* General galaxy init */
void init_galaxy(struct galaxy *g, int numstars, pgcontext gc, int w, int h) {
  int i;

  g->numstars = numstars;
  g->stars = (struct star *) malloc(numstars * sizeof(struct star));
  g->w = w;
  g->h = h;
  g->gc = gc;

  for (i=0;i<numstars;i++)
    emit_star(g,&g->stars[i]);

  /* Find background and foreground colors. Usually this will be white
   * on black, but if this is probably an LCD reverse this.
   */
  if (pgThemeLookup(PGTH_O_DEFAULT,PGTH_P_BGCOLOR)==0xFFFFFF) {
    g->fg = 0x000000;
    g->bg = 0xFFFFFF;
  }
  else {
    g->bg = 0x000000;
    g->fg = 0xFFFFFF;
  }

  /* Clear background */
  pgSetColor(g->gc,g->bg);
  pgRect(g->gc,0,0,g->w,g->h);
}

/* Draw a frame */
void draw_galaxy(struct galaxy *g) {
  int i,x,y;

  /* Update stars */
  for (i=0;i<g->numstars;i++) {
    g->stars[i].ox = g->stars[i].x >> PRECISION;
    g->stars[i].oy = g->stars[i].y >> PRECISION;
    g->stars[i].x += g->stars[i].vx;
    g->stars[i].y += g->stars[i].vy;
    x = g->stars[i].x >> PRECISION;
    y = g->stars[i].y >> PRECISION;

    if (g->stars[i].ox != x ||
	g->stars[i].oy != y) {
      pgSetColor(g->gc,g->fg);
      pgPixel(g->gc,x,y);
      pgSetColor(g->gc,g->bg);
      pgPixel(g->gc,g->stars[i].ox,g->stars[i].oy);
    }

    g->stars[i].life++;

    /* Time to re-emit this star? */
    if (x<0 || y<0 || x>g->w || y>g->h)
      emit_star(g,&g->stars[i]);
  }
  pgContextUpdate(g->gc);
}

/* Init the galaxy for fullscreen rendering */
void init_galaxy_fullscreen(struct galaxy *g, int numstars) {
  struct pgmodeinfo mi = *pgGetVideoMode();
  pgRegisterOwner(PG_OWN_DISPLAY);
  init_galaxy(g,numstars,pgNewBitmapContext(0),mi.lxres,mi.lyres);
}

/* Main! Yay! */
int main(int argc, char **argv) {
  struct galaxy galax;

  pgInit(argc,argv);
  srand(time(NULL));
  init_galaxy_fullscreen(&galax, 100);

  /* This will run the graphics as fast as possible, but ignore PicoGUI
	* events. This is necessary until we get a non-blocking pgGetEvent
	* or something similar.
	* The only way to close this program is with CTRL-C
	*/
  while (1)
    draw_galaxy(&galax);
  return 0;
}

