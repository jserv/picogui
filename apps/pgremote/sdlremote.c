/* $Id: sdlremote.c,v 1.4 2001/05/13 01:30:46 micahjd Exp $
 * 
 * sdlremote.c - pgremote is a networked PicoGUI input driver.
 *               This uses SDL, so hopefully it is fairly portable.
 *               Currently it simply relays mouse and keyboard events to the 
 *               pgserver, but maybe it's possible in the near future to have
 *               it take screenshots or run macros?
 *               I should save the whole remote-desktop idea for the VNC driver
 *               though ;-)
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
 * Contributors:
 * 
 * 
 * 
 */

#include <picogui.h>
#include <SDL.h>

int main(int argc, char **argv) {
   SDL_Surface *surf;
   SDL_Event evt;
   struct pgmodeinfo mi;
   int ox=0,oy=0,btnstate=0;
   int scale = 1;
   
   /* Don't need an app, but a connection would be nice... */
   pgInit(argc,argv); 
   mi = *pgGetVideoMode();

   /* If the server is especially low resolution, magnify it */
   if (mi.xres < 300 || mi.yres < 300) {
      if (mi.xres > mi.yres)
	scale = 300/mi.xres;
      else
	scale = 300/mi.yres;
   }
	
   /* Start up SDL */
   if (SDL_Init(SDL_INIT_VIDEO)) {
      printf("Error initializing SDL: %s\n",SDL_GetError());
      return 1;
   }
   
   /* Set a video mode to match the server's _physical_ resolution */
   surf = SDL_SetVideoMode(mi.xres*scale,mi.yres*scale,8,0);
   if (!surf) {
      printf("Error setting video mode: %s\n",SDL_GetError());
      return 1;
   }
   SDL_EnableUNICODE(1);

   /* Time to wait! Most PicoGUI apps spend their time waiting in a
    * pgEventLoop, but we don't even have one... */
   while (SDL_WaitEvent(&evt)) {
   
      switch (evt.type) {
    
       case SDL_MOUSEMOTION:
	 evt.motion.x /= scale;
	 evt.motion.y /= scale;
	 
	 /* Skip false moves (like dragging outside the window edge)
	  * and ignore moves we can't keep up with */
	 if ((evt.motion.x==ox) && (evt.motion.y==oy)) break;
	 if (SDL_PollEvent(NULL)) break;

         pgSendPointerInput(PG_TRIGGER_MOVE,ox = evt.motion.x,
			   oy = evt.motion.y,btnstate=evt.motion.state);
	 break;
	 
       case SDL_MOUSEBUTTONDOWN:
	 evt.button.x /= scale;
	 evt.button.y /= scale;
	 
	 pgSendPointerInput(PG_TRIGGER_DOWN,evt.button.x,
			    evt.button.y,btnstate |= 1<<(evt.button.button-1));
	 break;
	 
       case SDL_MOUSEBUTTONUP:
	 evt.button.x /= scale;
	 evt.button.y /= scale;

	 pgSendPointerInput(PG_TRIGGER_UP,evt.button.x,
			    evt.button.y,btnstate &= ~(1<<(evt.button.button-1)));
	 break;
	 
       case SDL_KEYDOWN:
	 if (evt.key.keysym.unicode)
	   pgSendKeyInput(PG_TRIGGER_CHAR,evt.key.keysym.unicode,
			  evt.key.keysym.mod);
	 pgSendKeyInput(PG_TRIGGER_KEYDOWN,evt.key.keysym.sym,
			evt.key.keysym.mod);
	 break;
	 
       case SDL_KEYUP:
	 pgSendKeyInput(PG_TRIGGER_KEYUP,evt.key.keysym.sym,
			evt.key.keysym.mod);
	 break;
	 
       case SDL_QUIT:
	 SDL_Quit();
	 return 0;
	 break;
      }
      
      pgFlushRequests();
   }
   
   SDL_Quit();
   return 0;
}
   
/* The End */
