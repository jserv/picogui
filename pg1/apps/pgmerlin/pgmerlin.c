/* pgmerlin.c - footprint single character recognition
 * 
 * PicoGUI:
 * Copyright (C) 2002 bigthor <bigthor@softhome.net>
 * X Window:
 * Copyright (C) 2000 Stefan Hellkvist, Ericsson Radio Systems AB 
 * Email: stefan@hellkvist.org
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
#include "graphics.h"
#include "pattern.h"
#include "wizard.h"
#include "event.h"

Pattern *current;
MODE mode;
BANK bank;

int main(int argc, char **argv) {
  initGraphics( argc, argv );
  initWizard();
  current = newPattern( 100, 'a' );
  setBank( NATURAL );
  setMode( SINGLE_STROKE );
  pgEventLoop();
  return 0;
}
/* The End */







