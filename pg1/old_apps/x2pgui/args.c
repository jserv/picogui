/*
 *  Copyright (C) 1997, 1998 Olivetti & Oracle Research Laboratory
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

/*
 * args.c - argument processing.
 */

#include <sys/utsname.h>
#include "x2pgui.h"

extern int resurface;

char *programName;

char hostname[256];
int port;

char *displayname = NULL;

char *geometry = NULL;

int wmDecorationWidth = 4;
int wmDecorationHeight = 24;

char *passwdFile = NULL;

int updateRequestPeriodms = 0;

int updateRequestX = 0;
int updateRequestY = 0;
int updateRequestW = 0;
int updateRequestH = 0;

int rawDelay = 0;
int copyRectDelay = 0;

Bool debug = False;

void usage()
{
    fprintf(stderr,
	    "x2pgui\n"
	    "Based on x2vnc which is Copyright (C) 2000 Fredrik Hubinette\n"
	    "Based on vncviewer which is copyright by AT&T\n"
	    "x2vnc comes with ABSOLUTELY NO WARRANTY. This is free software,\n"
	    "and you are welcome to redistribute it under certain conditions.\n"
            "See the file COPYING in the x2pgui source for details.\n"
	    "\n"
	    "usage: %s [<options>]\n"
	    "\n"
	    "<options> are:\n"
	    "              [-north] [-south] [-east] [-west]\n"
	    "              [-resurface] [-edgewidth width]\n"
	    "              [-display xdisplay]\n"
	    "              [--pgserver pgserver]\n"
	    ,programName);
    exit(1);
}


void
processArgs(int argc, char **argv)
{
    int i;
    Bool argumentSpecified = False;

    programName = argv[0];

    for (i = 1; i < argc && argv[i]; i++) {

	if (strcmp(argv[i],"-display") == 0) {

	    if (++i >= argc) usage();
	    displayname = argv[i];

	} else if (strcmp(argv[i],"-east") == 0) {
	  edge=EDGE_EAST;
	} else if (strcmp(argv[i],"-west") == 0) {
	  edge=EDGE_WEST;
	} else if (strcmp(argv[i],"-north") == 0) {
	  edge=EDGE_NORTH;
	} else if (strcmp(argv[i],"-south") == 0) {
	  edge=EDGE_SOUTH;
	} else if (strcmp(argv[i],"-nowheel") == 0) {
	  extern int emulate_wheel;
	  emulate_wheel=0;
	} else if (strcmp(argv[i],"-resurface") == 0) {
	  resurface=True;
	} else if (strcmp(argv[i],"-edgewidth") == 0) {
	  extern int edge_width;
	    if (++i >= argc) usage();
	    edge_width = atoi(argv[i]);
	    if(edge_width < 1)
	    {
	      fprintf(stderr,"x2pgui: -edgewidth cannot be less than 1\n");
	      exit(1);
	    }
	} else {

	    usage();

	}
    }
}
