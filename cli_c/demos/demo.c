/* $Id: demo.c,v 1.2 2000/09/16 07:04:41 micahjd Exp $
 *
 * demo.c -   source file for testing PicoGUI
 *
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micah@homesoftware.com>
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
 * Contributors: Philippe Ney <philippe.ney@smartdata.ch>
 * 
 * 
 * 
 */

#include <picogui.h>

int main(int argc, char *argv[])
{
  pgInit(argc,argv);

  pgNewPopupAt(20,20,10,50);
  pgEventLoop();

  return 0;
}





/* The End */
