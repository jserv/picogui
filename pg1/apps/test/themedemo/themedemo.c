/* $Id$
 *
 * themedemo.c - Test proggie for the new theme stuff I've been
 *               working on in the theme_overhaul branch of
 *               the PicoGUI server
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
#include <stdio.h>   /* For getc */

int main(int argc, char **argv) {
  pgInit(argc,argv);

  pgLoadTheme(pgFromFile(argv[1]));
  pgUpdate();

  getc(stdin);

  return 0;
}





/* The End */
