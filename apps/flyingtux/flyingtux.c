/*
 * Demonstration of canvas animation with bitmaps
 *
 * -- Micah Dowty <micahjd@users.sourceforge.net>
 *
 */

#include <picogui.h>

#define NUMTUXES  20
#define TUXWIDTH  16
#define TUXHEIGHT 16
#define TUXFILE   "data/tux.pnm"
#define TUXMASK   "data/tux_mask.pnm"
#define PRECISION 8
#define SPEEDVAR  200
#define SPEEDMIN  200

/* Tux bitmaps */
pghandle tuxbits,tuxmask;

struct tux {
  int x,y;    /* Position (fixed point) */
  int ox,oy;  /* Old position */
  int vx,vy;  /* Velocity (fixed point) */
};

struct flock {
  struct tux tuxes[NUMTUXES];
  pgcontext gc;
  pgcolor bg;
  int w,h;
};

void emit_tux(struct flock *f, struct tux *t);
void init_flock(struct flock *f);
void draw_flock(struct flock *f);

/* Reset a tux to the emitter location */
void emit_tux(struct flock *f, struct tux *t) {
  t->x = f->w << (PRECISION-1);
  t->y = f->h << (PRECISION-1);
  while (t->vx + t->vy < SPEEDMIN) {
	 t->vx = 1 + (rand()%SPEEDVAR);
	 t->vy = 1 + (rand()%SPEEDVAR);
  }
  if (rand()%2)
    t->vx = -t->vx;
  if (rand()%2)
    t->vy = -t->vy;
}

/* Reset all the tuxes */
void init_flock(struct flock *f) {
  int i;
  for (i=0;i<NUMTUXES;i++)
    emit_tux(f,&f->tuxes[i]);
}

/* Draw a frame */
void draw_flock(struct flock *f) {
  int i,x,y;

  for (i=0;i<NUMTUXES;i++) {

    /* Update position */
    f->tuxes[i].ox = f->tuxes[i].x >> PRECISION;
    f->tuxes[i].oy = f->tuxes[i].y >> PRECISION;
    f->tuxes[i].x += f->tuxes[i].vx;
    f->tuxes[i].y += f->tuxes[i].vy;
    x = f->tuxes[i].x >> PRECISION;
    y = f->tuxes[i].y >> PRECISION;

    /* Clear the old tux, draw the new one. */
    pgSetColor(f->gc,f->bg);
    pgRect(f->gc,f->tuxes[i].ox,f->tuxes[i].oy,TUXWIDTH,TUXHEIGHT);
    pgSetLgop(f->gc,PG_LGOP_AND);
    pgBitmap(f->gc,x,y,TUXWIDTH,TUXHEIGHT,tuxmask);
    pgSetLgop(f->gc,PG_LGOP_OR);
    pgBitmap(f->gc,x,y,TUXWIDTH,TUXHEIGHT,tuxbits);
    pgSetLgop(f->gc,PG_LGOP_NONE);

    /* Time to re-emit this tux? */
    if (x<-TUXWIDTH || y<-TUXHEIGHT || x>f->w || y>f->h)
      emit_tux(f,&f->tuxes[i]);
  }

  /* Update screen */
  pgContextUpdate(f->gc);
}

/* Canvas build event */
int buildCanvas(struct pgEvent *evt) {
  struct flock *f = (struct flock *) evt->extra;

  /* Store width and height */
  f->w = evt->e.size.w;
  f->h = evt->e.size.h;
  return 0;
}

int main(int argc,char **argv) {
  pghandle canvas;
  struct flock f;
  pgcontext gc;
   
  pgInit(argc,argv);

  /* Load bitmaps */
  tuxbits = pgNewBitmap(pgFromFile(TUXFILE));
  tuxmask = pgNewBitmap(pgFromFile(TUXMASK));

  /* Make an application and canvas */
  pgRegisterApp(PG_APP_NORMAL,"Flying Tux!",0);
  canvas = pgNewWidget(PG_WIDGET_CANVAS,0,0);
  pgBind(canvas, PG_WE_BUILD, buildCanvas, &f);

  /* Find foreground and background colors, hack to use a pure white
   * background if we're probably on an LCD. 
   */
  if (pgThemeLookup(PGTH_O_DEFAULT,PGTH_P_BGCOLOR)==0xFFFFFF)
    f.bg = 0xFFFFFF;
  else
    f.bg = 0x404040;

      
  /* Draw the background on a persistent context */
  gc = pgNewCanvasContext(canvas,PGFX_PERSISTENT);
  pgSetColor(gc,f.bg);
  pgRect(gc,0,0,0x7FFF,0x7FFF);
  pgDeleteContext(gc);
   

  /* Init tuxes */
  f.gc = pgNewCanvasContext(canvas,PGFX_IMMEDIATE);
  init_flock(&f);

  /* Animation loop */
  while (1) {
    draw_flock(&f);
//    usleep(2);
    pgEventPoll();
  }

  return 0;
}



