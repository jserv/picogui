#include <stdlib.h>
#include <string.h>

#include "gridgame.h"

struct gridgame *register_ataxx(void);
struct gridgame *register_overflow(void);

static struct gridgame **games=NULL, *game=NULL;
static int currentgame, totalgames=0, thobj=0;
static int bgfill, bgevenodd[2], doredraw;
static int player[MAXPLAYERS];
static pghandle toolbar, canvas, app, statuslabel;
static squarestatus *squares=NULL;
static gridpos lastselected={-1,-1};
#define SQ(pos) (squares[game->height*((pos).y)+((pos).x)])

/* Grid game support functions declared in gridgame.h */

void ggstatusline(const char *msg)
 {
  pgReplaceText(statuslabel, msg);
 }

static void fullredraw(void)
 {
  gridpos p;

  pgWriteCmd(canvas, PG_GROP_RESETCLIP, 0);
  for(p.x=0; p.x<game->width; p.x++)
    for(p.y=0; p.y<game->height; p.y++)
     {
      if(SQ(p).bricktype)
	pgWriteCmd(canvas, PGCANVAS_EXECFILL, 6,
	    SQ(p).player?player[SQ(p).player-1]:thobj,
	    SQ(p).bricktype, SCALE*p.x, SCALE*p.y, SCALE, SCALE);
     }
  pgWriteCmd(canvas, PGCANVAS_INCREMENTAL, 0);
  pgSubUpdate(canvas);
 }

static void redraw(gridpos p)
 {
  pgWriteCmd(canvas, PG_GROP_SETCLIP, 4, SCALE*p.x, SCALE*p.y, SCALE, SCALE);
  if(bgfill)
    pgWriteCmd(canvas, PGCANVAS_EXECFILL, 6, thobj, PGTH_P_BGFILL,
	0, 0, SCALE*game->width, SCALE*game->height);
  if(bgevenodd[1&(p.x^p.y)])
    pgWriteCmd(canvas, PGCANVAS_EXECFILL, 6, thobj, (1&(p.x^p.y))?BGODD:BGEVEN,
	p.x*SCALE, p.y*SCALE, SCALE, SCALE);
  if(SQ(p).bricktype)
    pgWriteCmd(canvas, PGCANVAS_EXECFILL, 6,
	SQ(p).player?player[SQ(p).player-1]:thobj,
	SQ(p).bricktype, SCALE*p.x, SCALE*p.y, SCALE, SCALE);
  doredraw=1;
 }

void ggset(gridpos pos, squarestatus status)
 {
  if(ggisvalid(pos) && memcmp(&status, &SQ(pos), sizeof(status)))
   {
    SQ(pos)=status;
    redraw(pos);
   }
 }

void ggmove(gridpos from, gridpos to, squarestatus newstatus)
 {
  /* this is the only way for the games to put pieces on the board */
  if(ggisvalid(from) && ggisvalid(to) &&
      memcmp(&SQ(to),&newstatus,sizeof(newstatus)))
   {
    SQ(to)=newstatus;
    if(to.x!=from.x || to.y!=from.y)
     {
      memset(&SQ(from), 0, sizeof SQ(from));
      redraw(from);
     }
    redraw(to);
   }
 }

static void drawboard(void)
 {
  int i, j;

  /* Remove contents of canvas */
  pgWriteCmd(canvas, PGCANVAS_NUKE, 0);
  /* Persistent mode */
  pgWriteCmd(canvas, PGCANVAS_DEFAULTFLAGS, 1, 0);
  /* Scaling */
  pgWriteCmd(canvas, PGCANVAS_INPUTMAPPING, 5, 0, 0,
      game->width*SCALE, game->height*SCALE, PG_MAP_SQUARESCALE);
  pgWriteCmd(canvas, PGCANVAS_GROP, 6, PG_GROP_SETMAPPING, 0, 0,
      game->width*SCALE, game->height*SCALE, PG_MAP_SQUARESCALE);
  pgWriteCmd(canvas, PGCANVAS_GROPFLAGS, 1, PG_GROPF_UNIVERSAL);
  /* Board */
  /* full board themes */
  if(bgfill)
    pgWriteCmd(canvas, PGCANVAS_EXECFILL, 6, thobj, PGTH_P_BGFILL,
	0, 0, SCALE*game->width, SCALE*game->height);
  /* checkerboard themes */
  for(i=0; i<game->width; i++)
    for(j=0; j<game->height; j++)
      if(bgevenodd[1&(i^j)])
	pgWriteCmd(canvas, PGCANVAS_EXECFILL, 6, thobj, (1&(i^j))?BGODD:BGEVEN,
	    i*SCALE, j*SCALE, SCALE, SCALE);
  /* immediate drawing of pieces */
  pgWriteCmd(canvas, PGCANVAS_DEFAULTFLAGS, 1, PG_GROPF_TRANSIENT);
 }

squarestatus gggetstatus(gridpos square)
 {
  squarestatus invalid={-1, -1, 0};

  if(ggisvalid(square))
    return SQ(square);
  else
    return invalid;
 }

int ggisvalid(gridpos square)
 {
  return square.x>=0 && square.x<game->width &&
    square.y>=0 && square.y<game->height &&
    squares;
 }

int ggisselected(gridpos square)
 {
  if(ggisvalid(square))
    return SQ(square).selected;
  else
    return 0;
 }

void ggselect(gridpos square)
 {
  if(ggisvalid(square))
   {
    SQ(square).selected=1;
    lastselected=square;
   }
 }

void ggdeselect(gridpos square)
 {
  if(ggisselected(square))
   {
    lastselected.x=lastselected.y=-1;
    SQ(square).selected=0;
   }
 }

static void selectgame(int which)
 {
  int i=0;
  char thobjname[256];

  which%=totalgames;	/* no segfaults here, please */
  if(game)
   {
    if(game->cleanup)
      game->cleanup();
    pgLeaveContext();
    free(squares);
   }
  currentgame=which;
  game=games[currentgame];
  pgReplaceTextFmt(app, "Gridgame: %s", game->name);
  pgEnterContext();
  while(game->themes[i])
    pgLoadTheme(pgFromFile(game->themes[i++]));
  thobj=pgFindThemeObject(game->theme);
  for(i=1; i<=game->players; i++)
   {
    sprintf(thobjname, "%s.player%d", game->theme, i);
    player[i-1]=pgFindThemeObject(thobjname);
   }
  bgfill=pgThemeLookup(thobj, PGTH_P_BGFILL);
  bgevenodd[0]=pgThemeLookup(thobj, BGEVEN);
  bgevenodd[1]=pgThemeLookup(thobj, BGODD);
  squares=malloc(sizeof(squares[0])*game->width*game->height);
  memset(squares, 0, sizeof(squares[0])*game->width*game->height);
  drawboard();
  if(game->init)
    game->init();
 }

static int ptrx, ptry;

static int evtPtrDown(struct pgEvent *evt)
 {
  ptrx=evt->e.pntr.x;
  ptry=evt->e.pntr.y;
 }

static int evtPtrUp(struct pgEvent *evt)
 {
  if(game && game->drag)
   {
    doredraw=0;
    game->drag(ptrx, ptry, evt->e.pntr.x, evt->e.pntr.y);
    if(doredraw)
     {
      pgWriteCmd(canvas, PGCANVAS_INCREMENTAL, 0);
      pgSubUpdate(canvas);
     }
   }
 }

static int evtBuild(struct pgEvent *evt)
 {
  if(game)
    fullredraw();
 }

static int evtNewGame(struct pgEvent *evt)
 {
  selectgame((int)evt->extra);
 }

int main(int argc, char *argv[])
 {
  int i=0, j;

  /* Initialize game list */
  games=realloc(games, 2*sizeof(games[0]));
  games[totalgames++]=register_ataxx();
  games[totalgames++]=register_overflow();
  /* For dynamically loaded games:
   * games=realloc(games, (++totalgames)*sizeof(games[0]));
   * games[totalgames-1]=register_yourgame(); */
  while(i<totalgames)
   {
    /* TODO: game unloading? */
    if(games[i]->players>MAXPLAYERS)	/* unsupported game */
      games[i]=games[--totalgames];
    else
      i++;
   }

  /* Initialize UI */
  pgInit(argc, argv);
  app=pgRegisterApp(PG_APP_NORMAL, "Gridgame", 0);
  toolbar=pgNewWidget(PG_WIDGET_TOOLBAR, 0, 0);
  canvas=pgNewWidget(PG_WIDGET_CANVAS, PG_DERIVE_AFTER, toolbar);
  pgBind(PGDEFAULT, PG_WE_PNTR_DOWN, evtPtrDown, NULL);
  pgBind(PGDEFAULT, PG_WE_PNTR_UP, evtPtrUp, NULL);
  pgBind(PGDEFAULT, PG_WE_BUILD, evtBuild, NULL);

  for(i=0; i<totalgames; i++)
   {
    pgNewWidget(PG_WIDGET_BUTTON, i?0:PG_DERIVE_INSIDE, i?0:toolbar);
    pgSetWidget(PGDEFAULT, PG_WP_TEXT, pgNewString(games[i]->name), 0);
    pgBind(PGDEFAULT, PG_WE_ACTIVATE, evtNewGame, (void*)i);
   }
  statuslabel=pgNewWidget(PG_WIDGET_LABEL, 0, 0);

  /* Start game engine */
  selectgame(0);

  pgEventLoop();
  return 0;
 }

