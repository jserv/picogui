/*  $Id$
 *
 *  edemo - a PicoGUI demo
 *
 *  Author: Daniele Pizzoni - Ascensit s.r.l. - Italy
 *  tsho@ascensit.com - auouo@tin.it
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

#ifndef SPECIAL_H
#define SPECIAL_H

#include <edemo.h>

/* special template widgets. Every time a widget whose name is the
 * string is loaded from a template file the correspondin function is
 * called */
struct edemoHandle {
    char* sName;
    int (*action)(struct edemoUI*, pghandle widget);
    int (*delete)(struct edemoUI*, pghandle widget);
};


void parseSpecialWidgets(struct edemoUI* interface);
void idleHandlerAdd (pgidlehandler func);
void idleDelete(void);

/* imports */
void themebar(pghandle widget, char* directory, int* themecontext);
void themebar_clear(void);
int dialogdemo(pghandle widget);
void chaos_fire(pghandle widget);
int canvastst(pghandle widget);
int pterm(pghandle widget);
int menutest(pghandle widget);
#endif /* SPECIAL_H */
