/* $Id: jetengine.cpp,v 1.13 2002/11/26 19:18:07 micahjd Exp $
 *
 * jetengine.cpp - Main program for a more complex example of pgserver embedding
 *
 * Copyright (C) 2002 Micah Dowty and David Trowbridge
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

#include <stdio.h>
#include <GL/gl.h>
#include <SDL/SDL.h>
#include "EmbeddedPGserver.h"
#include "PythonThread.h"
#include "PythonInterpreter.h"
#include "FlatLand.h"
#include "Ship.h"
#include "Camera.h"
#include "Skybox.h"


int main(int argc, char **argv) {
  PythonInterpreter py(argc, argv);
  try {
    EmbeddedPGserver pgserver(argc, argv);
    PythonThread pythread(&py);
    FlatLand world(&py);
    Camera camera(&py);
    Ship ship(&py);
    Skybox skybox(&py);
    u32 old_ticks, new_ticks;
    float frame_time;

    pythread.addObject("world",&world);
    pythread.addObject("ship",&ship);
    pythread.addObject("camera",&camera);
    pythread.addObject("skybox",&skybox);
    pythread.run();

    old_ticks = SDL_GetTicks();
    while (pgserver.mainloopIsRunning()) {
      new_ticks = SDL_GetTicks();
      frame_time = (new_ticks - old_ticks) / 1000.0f;
      old_ticks = new_ticks;

      glEnable(GL_DEPTH_TEST);
      glDepthFunc(GL_LEQUAL);
      glClear(GL_DEPTH_BUFFER_BIT);
      glShadeModel(GL_SMOOTH);

      camera.setMatrix();
      skybox.draw();
      world.draw();
      ship.draw();
      world.animate(frame_time);

      pgserver.mainloopIteration();
    }  
  }
  catch (SimpleException &e) {
    e.show();
    return 1;
  }
  return 0;
}

