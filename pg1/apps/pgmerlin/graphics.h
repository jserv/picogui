/*********************************************************
 * graphics.h
 *
 * Copyright (c) Stefan Hellkvist
 *********************************************************/

#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdlib.h>

#define DEFAULT_WINDOW_WIDTH 200
#define DEFAULT_WINDOW_HEIGHT 100
#define BORDER_WIDTH 1

void initGraphics( int argc, char *argv[] );
void paint( void );
void paintPattern( void );

extern Display *myDisplay;
extern Window myWindow;
extern GC myGC;
extern int myScreen;



#endif
