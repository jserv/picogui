/*  $Id$
 *
 *  edemo - a PicoGUI demo
 *
 *  Author: Daniele Pizzoni - Ascensit s.r.l. - Italy
 *  tsho@ascensit.com - auouo@tin.it
 *
 *  Based on picosm (by ?) and pgdemo.py by Micah Dowty
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

#ifndef EDEMO_H
#define EDEMO_H

#include <picogui.h>

struct edemoUI {
    pghandle wApp;       /* EDemo */

    /* navbar */
    pghandle wNavBar;    /* EDNavBar */

    /* label */
    pghandle wLabel;     /* EDLabel */
  
    /* buttons */
    pghandle wPrev;      /* EDPrev */
    pghandle wNext;      /* EDNext */

    /* page box */
    pghandle wBox;       /* EDBox */

    /* custom widgets */
    pghandle wAppList;
    pghandle wThList;

    /* current page */
    pghandle wCurrPage;

    /* template files */
    int pTot;            /* total number of pages */
    int pCurr;           /* current page */

    /* contexts */
    int mainContext;
    int pageContext;
    int themeContext;
};

/* a simple error handler */
int eerror(char * string);

#endif /* EDEMO_H */
