/*
 * Little demo of 2D game-like graphics via the canvas widget.
 * At runtime, this depends on having tileset.th in the current directory.
 *
 *  -- Micah Dowty <micahjd@users.sourceforge.net>
 */

#include <picogui.h>
#include <time.h>

struct rectsize {
  int w,h;
};

struct sceneGlobals {
  /* Bitmaps */
  pghandle bShip, bLogo, bLasers;
  int shipw,shiph,logow,logoh,laserw,laserh;

  /* PGFX context to draw on */
  pgcontext gc;
  struct rectsize canvasSize;

  /* Toggle flags */
  int drawShip, drawGrass, drawLogo, drawLasers;

  /* Animation */
  int y, laser_y;
};  

int evtToggle(struct pgEvent *evt);
int evtResize(struct pgEvent *evt);
void makeToggle(pghandle wTB, const char *name, int *var);

void initScene(struct sceneGlobals *sg);
void drawScene(struct sceneGlobals *sg);
void drawGrass(struct sceneGlobals *sg);

/***************************************** Main Program */

int main(int argc, char **argv) {
  pghandle wCanvas,wTB,wFPS;
  long frames = 0;
  time_t start_time, now;
  struct sceneGlobals sg;
  memset(&sg,0,sizeof(sg));

  /* Set up us the PicoGUI 
   */
  pgInit(argc,argv);
  pgRegisterApp(PG_APP_NORMAL,"Canvas Tiles Demo",0);

  wTB = pgNewWidget(PG_WIDGET_TOOLBAR,0,0);

  /* Canvas with persistent contest, we'll nuke it every frame.
   * This is better than an immediate context, as pgserver can redraw
   * our scene between frames when it needs to. It shouldn't affect
   * the speed noticeably either way.
   */
  wCanvas = pgNewWidget(PG_WIDGET_CANVAS,0,0);
  pgSetWidget(wCanvas,
	      PG_WP_SIDE, PG_S_ALL,
	      0);
  pgBind(wCanvas, PG_WE_BUILD, evtResize, &sg.canvasSize);
  sg.gc = pgNewCanvasContext(wCanvas,PGFX_PERSISTENT);

  wFPS = pgNewWidget(PG_WIDGET_LABEL,PG_DERIVE_INSIDE,wTB);
  pgSetWidget(wFPS,
	      PG_WP_SIDE, PG_S_RIGHT,
	      PG_WP_TRANSPARENT, 0,
	      PG_WP_FONT, pgNewFont(NULL,0,PG_FSTYLE_FIXED),
	      PG_WP_TEXT, pgNewString("---.-- FPS"),
	      0);

  /* Give us some toggle buttons to affect the scene 
   */
  sg.drawGrass = sg.drawLogo = sg.drawShip = 1;
  makeToggle(wTB,"Logo", &sg.drawLogo);
  makeToggle(wTB,"Ship", &sg.drawShip);
  makeToggle(wTB,"Grass", &sg.drawGrass);
  makeToggle(wTB,"Lasers", &sg.drawLasers);

  /* Load graphics and such */
  initScene(&sg);

  /* Main graphics loop. We need to do pgUpdate ourselves since
   * pgEventPoll doesn't do it automatically. Just drawScene
   * and keep track of frames per second.
   */
  pgUpdate();
  start_time = time(NULL);
  while (1) {
    pgWriteCmd(wCanvas,PGCANVAS_NUKE,0);
    drawScene(&sg);
    pgContextUpdate(sg.gc);
    pgEventPoll();

    now = time(NULL);
    frames++;
    if (now > start_time+2) {
      pgReplaceTextFmt(wFPS,"%6.2f FPS",((float)frames)/(now-start_time));
      frames = 0;
      start_time = now;
      pgUpdate();
    }
  }
  return 0;
}

/***************************************** Graphics guts */

#define MAPWIDTH     8
#define MAPHEIGHT    16
#define TILESIZE     32
#define LOG_TILESIZE 5

/* This is in tile indexes, we'll look up real handles in init */
pghandle map[MAPHEIGHT][MAPWIDTH] = {
  { 0, 6, 2, 2, 2, 7, 0, 0, },
  { 0, 9, 4, 4, 4, 8, 0, 0, },
  { 2, 7, 0, 0, 0, 6, 2, 2, },
  { 1, 3, 0, 0, 0, 5, 1, 1, },
  { 1, 3, 0, 0, 0, 5, 1, 1, },
  { 4, 8, 0, 0, 0, 9, 4, 4, },
  { 0, 6, 7, 0, 0, 0, 0, 0, },
  { 0, 9, 8, 0, 6, 2, 2, 7, },
  { 0, 0, 0, 0, 9, 4, 4, 8, },
  { 0, 0, 0, 0, 0, 0, 0, 0, },
  { 0, 0, 6, 2, 2, 7, 0, 0, },
  { 0, 0, 5, 1, 1, 3, 6, 7, },
  { 0, 0, 5, 1, 1, 3, 9, 8, },
  { 0, 0, 5, 1, 1, 3, 0, 0, },
  { 6, 7, 5, 1, 1, 3, 0, 0, },
  { 9, 8, 9, 4, 4, 8, 0, 0, },
};

void initScene(struct sceneGlobals *sg) {
  int i,j;
  int thobj;

  /* This has all our tiles and sprites */
  pgLoadTheme(pgFromFile("tileset.th"));
  
  /* Convert our map from tile indexes to handles */
  thobj = pgFindThemeObject("canvastiles/tileset");
  for (i=0;i<MAPWIDTH;i++)
    for (j=0;j<MAPHEIGHT;j++)
      map[j][i] = pgThemeLookup(thobj,PGTH_P_USER + map[j][i]);

  /* Get handles to the sprites and their sizes */
  sg->bShip = pgThemeLookup(pgFindThemeObject("canvastiles/sprites/ship"),PGTH_P_BITMAP1);
  sg->bLogo = pgThemeLookup(pgFindThemeObject("canvastiles/sprites/logo"),PGTH_P_BITMAP1);
  sg->bLasers = pgThemeLookup(pgFindThemeObject("canvastiles/sprites/lasers"),PGTH_P_BITMAP1);
  pgSizeBitmap(&sg->shipw,&sg->shiph,sg->bShip);
  pgSizeBitmap(&sg->logow,&sg->logoh,sg->bLogo);
  pgSizeBitmap(&sg->laserw,&sg->laserh,sg->bLasers);
}

void drawScene(struct sceneGlobals *sg) {
  int h,y;

  /* Find the total height of the two sprites */
  h = 0;
  if (sg->drawShip)
    h += sg->shiph;
  if (sg->drawLogo)
    h += sg->logoh;
  y = (sg->canvasSize.h - h)>>1;

  if (sg->drawGrass)
    drawGrass(sg);

  /* Everything on top of the background has alpha */
  pgSetLgop(sg->gc,PG_LGOP_ALPHA);

  /* Draw some laser beams coming from the ship */
  if (sg->drawLasers) {
    pgSetClip(sg->gc,0,0,sg->canvasSize.w,y+45);
    pgBitmap(sg->gc,(sg->canvasSize.w - sg->laserw)>>1,sg->laser_y,
	     sg->laserw,sg->laserh,sg->bLasers);
    sg->laser_y -= 5;
    if (sg->laser_y < -sg->laserh)
      sg->laser_y = y+45;
    pgSetClip(sg->gc,0,0,sg->canvasSize.w,sg->canvasSize.h);
  }
      
  /* The Ship */
  if (sg->drawShip) {
    pgBitmap(sg->gc,(sg->canvasSize.w - sg->shipw)>>1,y,
	     sg->shipw,sg->shiph,sg->bShip);
    y += sg->shiph;
  }

  /* Neat little picogui logo */
  if (sg->drawLogo) {
    pgSetLgop(sg->gc,PG_LGOP_ALPHA);
    pgBitmap(sg->gc,(sg->canvasSize.w - sg->logow)>>1,
	     y,sg->logow,sg->logoh,sg->bLogo);
  }
}

/* Simple tile map drawing, with vertical scrolling and wraparound on the edges */
void drawGrass(struct sceneGlobals *sg) {
  int i,j;
  
  /* Enough tiles to cover our canvas */
  for (i=0;i<=(sg->canvasSize.w >> LOG_TILESIZE);i++)
    for (j=0;j<=(sg->canvasSize.h >> LOG_TILESIZE)+1;j++)
      pgBitmap(sg->gc,
	       /* Horizontal is always lined up with the map */
	       i<<LOG_TILESIZE,
	       /* Vertical is smoothly scrolled within each tile boundary */
	       (sg->y&(TILESIZE-1)) + (j<<LOG_TILESIZE) - TILESIZE,
	       TILESIZE,
	       TILESIZE,
	       /* Then it's snapped back by one complete tile at the tile boundary */
	       map[(j - (sg->y>>LOG_TILESIZE))&(MAPHEIGHT-1)][i&(MAPWIDTH-1)]);
  sg->y += 3;
}


/***************************************** Event Handlers */

/* Easy handler for setting the int in evt->extra to the button's on state */
int evtToggle(struct pgEvent *evt) {
  *(int*)evt->extra = pgGetWidget(evt->from, PG_WP_ON);
  return 0;
}

/* Update the size in evt->extra */
int evtResize(struct pgEvent *evt) {
  ((struct rectsize*)evt->extra)->w = evt->e.size.w;
  ((struct rectsize*)evt->extra)->h = evt->e.size.h;
  return 0;
}

/* Add a button to the toolbar to toggle the given variable */
void makeToggle(pghandle wTB, const char *name, int *var) {
  pgNewWidget(PG_WIDGET_BUTTON, PG_DERIVE_INSIDE,wTB);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT, pgNewString(name),
	      PG_WP_EXTDEVENTS, PG_EXEV_TOGGLE,
	      PG_WP_ON, *var,
	      0);
  pgBind(PGDEFAULT, PG_WE_ACTIVATE, evtToggle, var);
}
  
/* The End */
