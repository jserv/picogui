#include <stdlib.h>
#include "gridgame.h"
#include "overflow.h"

static currentplayer;

static void showplayer(void)
 {
  char str[]="Player X to move";

  str[7]='0'+currentplayer;
  ggstatusline(str);
 }

static void overflow_init(void)
 {
  currentplayer=1;
  showplayer();
 }

static int recursiveflow(gridpos p)
 {
  static int levels=0;
  int bricks;
  squarestatus s;

  s=gggetstatus(p);
  if(s.player==-1)
    return 0;
  s.player=currentplayer;
  if(s.bricktype)
    s.bricktype++;
  else
    s.bricktype=ONEQUARTER;
  if(s.bricktype<FOURQUARTERS)	/* full square */
   {
    ggset(p, s);
    return 1;
   }
  if(levels>256)	/* board is more than full.. */
   {
    s.bricktype=OWNEDBUTEMPTY;	/* player has won anyway.. */
    ggset(p, s);
    return 1;
   }
  levels++;
  s.bricktype-=4;	/* pick 4 bricks up */
  bricks=4;
  ggset(p, s);
  p.x--;	/* left */
  bricks-=recursiveflow(p);
  p.x+=2;	/* right */
  bricks-=recursiveflow(p);
  p.x--;
  p.y--;	/* up */
  bricks-=recursiveflow(p);
  p.y+=2;	/* down */
  bricks-=recursiveflow(p);
  p.y--;	/* back to original */
  s=gggetstatus(p);		/* pick up extra bricks left here */
  s.bricktype+=bricks-1;
  ggset(p, s);			/* drop the bricks... */
  recursiveflow(p);		/* letting the last one continue */
  levels--;
  return 1;
 }

static void overflow_drag(int x1, int y1, int x2, int y2)
 {
  gridpos p;
  squarestatus s;
  int i, j;

  p.x=x2/SCALE;
  p.y=y2/SCALE;
  s=gggetstatus(p);
  if(s.player&&s.player!=currentplayer)
    return;
  if(recursiveflow(p))
   {
    currentplayer^=3;
    showplayer();
   }
 }

static const char * const themes[]={"checkers.th", "overflow.th", NULL};

static struct gridgame overflow = {
  name: "Overflow",
  themes: themes,
  theme: "checkers",
  width: 8,
  height: 8,
  players: 2,
  drag: overflow_drag,
  init: overflow_init,
};

struct gridgame *register_overflow(void)
 {
  return &overflow;
 }

