#include <stdlib.h>
#include "gridgame.h"

static currentplayer=1;

static void ataxx_cleanup(void)
 {
 }

static void ataxx_init(void)
 {
  gridpos outside={-1,-1}, p={0,0};
  squarestatus s={1, PIECE, 0};

  ggmove(outside, p, s);
  p.x=p.y=6;
  ggmove(outside, p, s);
  p.x=0; p.y=6; s.player=2;
  ggmove(outside, p, s);
  p.x=6; p.y=0;
  ggmove(outside, p, s);
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
  ggmove(fp, tp, ts);
  if(abs(fp.x-tp.x)==2 || abs(fp.y-tp.y)==2)
   {
    fs.bricktype=fs.player=0;
    ggmove(fp, fp, fs);
   }
  for(i=-1;i<=1;i++)
    for(j=-1;j<=1;j++)
     {
      fp.x=tp.x+i;
      fp.y=tp.y+j;
      fs=gggetstatus(fp);
      if(fs.player>0 && fs.player!=ts.player)
	ggmove(tp, fp, ts);
     }
  currentplayer^=3;
 }

static struct gridgame ataxx = {
  name: "ataxx",
  width: 7,
  height: 7,
  players: 2,
  init: ataxx_init,
  drag: ataxx_drag,
  cleanup: ataxx_cleanup
};

struct gridgame *register_ataxx(void)
 {
  return &ataxx;
 }

