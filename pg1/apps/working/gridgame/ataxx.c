#include <stdlib.h>
#include "gridgame.h"

static currentplayer;

static void showplayer(void)
 {
  char str[]="Player X to move";

  str[7]='0'+currentplayer;
  ggstatusline(str);
 }

static void ataxx_init(void)
 {
  gridpos p={0,0};
  squarestatus s={1, PIECE, 0};

  currentplayer=1;
  ggset(p, s);
  p.x=p.y=6;
  ggset(p, s);
  p.x=0; p.y=6; s.player=2;
  ggset(p, s);
  p.x=6; p.y=0;
  ggset(p, s);
  showplayer();
 }

static void ataxx_drag(int x1, int y1, int x2, int y2)
 {
  gridpos fp, tp;
  squarestatus fs, ts;
  int i, j;

  fp.x=x1/SCALE;
  fp.y=y1/SCALE;
  fs=gggetstatus(fp);
  if(fs.player!=currentplayer)
    return;
  tp.x=x2/SCALE;
  tp.y=y2/SCALE;
  ts=gggetstatus(tp);
  if(ts.player)
    return;
  if(abs(fp.x-tp.x)>2 || abs(fp.y-tp.y)>2)
    return;
  ts.player=fs.player;
  ts.bricktype=fs.bricktype;
  for(i=-1;i<=1;i++)
    for(j=-1;j<=1;j++)
     {
      fp.x=tp.x+i;
      fp.y=tp.y+j;
      fs=gggetstatus(fp);
      if(fs.player>0 && fs.player!=ts.player)
	ggmove(tp, fp, ts);
     }
  fp.x=x1/SCALE;
  fp.y=y1/SCALE;
  ggmove(fp, tp, ts);
  if(abs(fp.x-tp.x)<2 && abs(fp.y-tp.y)<2)
    ggmove(fp, fp, ts);	/* keep original piece if short move */
  currentplayer^=3;
  showplayer();
 }

static const char * const themes[]={"checkers.th", NULL};

static struct gridgame ataxx = {
  name: "Ataxx",
  themes: themes,
  theme: "checkers",
  width: 7,
  height: 7,
  players: 2,
  init: ataxx_init,
  drag: ataxx_drag,
};

struct gridgame *register_ataxx(void)
 {
  return &ataxx;
 }

