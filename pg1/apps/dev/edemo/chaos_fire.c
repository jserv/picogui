/* $Id$
 *
 * chaos_fire.c - based on  PicoGUI's "chaos_fire.c"
 *
 * Modified by: Daniele Pizzoni - Ascensit s.r.l. - Italy
 * tsho@ascensit.com - auouo@tin.it
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micahjd@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#include <math.h>
#include <picogui.h>
#include <special.h>

pgcontext gc;
const float pi = 3.1415926;


float radius(float a, int t) {
  return sin(a*5+t*0.08)*0.3+1 + sin(a*42+t*0.7)*0.05;
}

void animate2(void) {
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

/* int main(int argc,char **argv) { */
void chaos_fire(pghandle widget)
{
  pgcontext bg;

/*   pgInit(argc,argv); */
/*   pgRegisterApp(PG_APP_NORMAL,"Burning cinder fury of crimson chaos fire",0); */
/*   pgNewWidget(PG_WIDGET_CANVAS,0,0); */

  bg = pgNewCanvasContext(widget,PGFX_PERSISTENT);
  pgSetMapping(bg,0,0,100,100,PG_MAP_SCALE);
  pgSetColor(bg, 0);
  pgRect(bg,0,0,100,100);

  gc = pgNewCanvasContext(widget,PGFX_IMMEDIATE);

  idleHandlerAdd(animate2);

  return;

  while (1) {
    animate2();
    pgEventPoll();
  }
}

