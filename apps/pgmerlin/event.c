/***************************************************************************
 * event.c
 *
 * PicoGUI:
 * Copyright (c) 2002 bigthor.
 * Email: bigthor@softhome.net
 *
 * X Window:
 * Copyright (c) 2000 Stefan Hellkvist, Ericsson Radio Systems AB 
 * Email: stefan@hellkvist.org
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 *****************************************************************************/

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdlib.h>
#include <stdio.h>
#include "point.h"
#include "graphics.h"
#include "event.h"
#include "pattern.h"
#include "mapping.h"
#include "wizard.h"
#include <ctype.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <picogui.h>

#define PEN_DRAG  0
#define PEN_UP    1
#define PEN_DOWN  2
#define SHIFT_SYMBOL '.'

void penDragged( PenEvent *pe );
void penUp( PenEvent *pe );
void penDown( PenEvent *pe );

extern Pattern *current;
extern int window_width, window_height;
int timer = 0;


void
findMinMax( Pattern *p, int *min_result, int *max_result )
{
    Point tmp;
    int xmin, xmax, i;
    
    tmp = p->pv[ 0 ];
    xmin = xmax = tmp.x;
    for ( i = 1; i < p->nPoints; i++ )
    {
        tmp = p->pv[ i ];
        if ( tmp.x < xmin )
            xmin = tmp.x;
        if ( tmp.x > xmax )
            xmax = tmp.x;
    }
    *min_result = xmin;
    *max_result = xmax;
}

Time
getTimeStamp()
{
    int tint;
    struct timeval tv;
    struct timezone tz;
    gettimeofday( &tv, &tz );
    tint = (int) tv.tv_sec * 1000;
    tint = tint / 1000 * 1000;
    tint = tint + tv.tv_usec/1000;
    return (Time) tint;
}


/* FIXME: Must send event key_pressed inside pgserver (see pgboard?) */
void
outputChar( unsigned char c )
{
  static union pg_client_trigger trig;
/*      XEvent ev; */
    
/*      Window focusWindow; */
/*      int revert_to_return; */
/*      XGetInputFocus( myDisplay, &focusWindow, &revert_to_return ); */
    
/*      ev.type = KeyPress; */
/*      ev.xkey.window = focusWindow; */
/*      ev.xkey.subwindow = None; */
/*      ev.xkey.x = 1; */
/*      ev.xkey.time = getTimeStamp(); */
/*      ev.xkey.y = 1; */
/*      ev.xkey.x_root = ev.xkey.y_root = 1; */
/*      ev.xkey.same_screen = True; */
/*      ev.xkey.send_event = 0; */
/*      ev.xkey.display = myDisplay; */
/*      ev.xkey.keycode = asciiToKeycode( c, &ev.xkey.state ); */
    
/*      if ( ev.xkey.keycode != 0 ) */
/*      { */
/*  	XSendEvent( myDisplay, focusWindow, True, KeyPressMask, &ev ); */
/*  	ev.type = KeyRelease; */
/*  	ev.xkey.time = getTimeStamp(); */
/*  	XSendEvent( myDisplay, focusWindow, True, KeyReleaseMask, &ev ); */
/*      } */
  printf("%c", c);fflush(NULL);
/*    pgSendKeyInput(PG_TRIGGER_CHAR,c,0); */
  trig.content.type       = PG_TRIGGER_CHAR;
  trig.content.u.kbd.key  = c;
  trig.content.u.kbd.mods = 0;
  pgInFilterSend(&trig);
  pgFlushRequests();
}


/**
 * setMode
 * Sets the mode to either single or multiple mode
 **/
void
setMode( MODE m )
{
    mode = m;
}


/**
 * restoreBank
 * Restores the bank
 **/
void
restoreBank()
{
    setMode( SINGLE_STROKE );
    setBank( bank );
}


/**
 * getMode
 * Returns the current mode
 **/
MODE
getMode()
{
    return mode;
}


/**
 * storeBank
 * Stores the bank in a safe place to beware of bank robbers perhaps :)
 **/
void
storeBank()
{
    bank = getBank();
}

    
unsigned char
my_toupper( int c )
{
    if ( c == 229 )
	return 197;
    else
	if ( c == 228  )
	    return 196;
	else
	    if ( c == 246 )
		return 214;
	    else return toupper( c );
}


/**
 * timedOut
 * Called when a pattern is finished and we should analyze it.
 * In SINGLE_MODE when the user lifts the pen, in MULTIPLE_MODE when
 * the user has lifted the pen and a short time of inactivity has occurred
 **/
void
timedOut()
{
    unsigned char c;
    int xmin, xmax;

    if ( getBank() != EXTENDED )
    {
	findMinMax( current, &xmin, &xmax );
	if ( xmax <= window_width / 2 || xmin <= window_width / 2 )
	    setBank( NATURAL );
	else
	    if ( xmin >= window_width / 2 )
		setBank( NUMERAL );
    }

    c  = recognize( current );
    if ( mode == MULTIPLE_STROKE )
	restoreBank(); 

    if ( c == 255 )
    {
	storeBank();
  	setBank( EXTENDED ); 
  	setMode( MULTIPLE_STROKE );
    }
    else   
    {
	if ( xmin <= window_width / 2 && xmax >= window_width / 2 )
	    c = my_toupper( c );
	if ( (c == 'i' || c == '1') && xmin > window_width/2 - 10 && 
	     xmax < window_width/2 + 10 ) 
            c = 'I';        /* Stupid fix to write I */
	outputChar( c );
    }
    deletePattern( current );
    current = newPattern( 100, 'a' ); 
}


void
handleEvent( int x, int y, int buttonStatus )
{
    PenEvent pe;
    pe.x = x;
    pe.y = y;
    
    switch ( buttonStatus )
    {
    case PEN_DRAG: penDragged( &pe ); break;
    case PEN_UP: penUp( &pe ); break;
    case PEN_DOWN: penDown( &pe ); break;
    default:
    }
}

int
checkEvent( XEvent *ev )
{
/*      clock_t now, start_time = clock(); */
/*      do */
/*      { */
/*  	if ( XCheckWindowEvent( myDisplay, myWindow,  */
/*  				PointerMotionMask | ButtonPressMask, ev ) ) */
/*  	    return 1; */
/*  	now = clock(); */
/*      } while ( (now - start_time) < CLOCKS_PER_SEC ); */ /* Wait for 1 sec */
    return 0;
}

void
eventLoop()
{
/*      int x = 0, y = 0; */
/*      XEvent report; */
/*      int status = PEN_UP; */

  /* Select event types wanted */ 
/*      XSelectInput( myDisplay, myWindow,  */
/*  		  ExposureMask | ButtonPressMask |  */
/*  		  ButtonReleaseMask | PointerMotionMask ); */
/*      while ( 1 ) */
/*      { */
/*  	if ( timer )  */
/*  	{ */
/*  	    if ( !checkEvent( &report ) ) */
/*  	    { */
/*  		report.type = 0; */
/*  		timedOut(); */
/*  		timer = 0; */
/*  	    } */
/*  	    else */
/*  		if ( report.type == ButtonPress ) */
/*  		{ */
/*  		    timer = 0; */
/*  		    if ( current->nPoints > 0 ) */
/*  			current->pv[ current->nPoints - 1 ].isLast = 1; */
/*  		} */
/*  	} */
/*  	else */
/*  	    XNextEvent( myDisplay, &report); */
/*  	switch  (report.type) { */
/*  	case 0:  break; */ /* No event occured */
/*  	case Expose: */
  	    /* unless this is the last contiguous expose, 
  	     * don't draw the window */ 
/*  	    if (report.xexpose.count != 0) */
/*  		break; */
/*  	    if (report.xexpose.window == myWindow) */
/*  		paint(); */
/*  	    break; */
/*  	case ButtonPress: */
/*  	    handleEvent( x, y, status = PEN_DOWN ); */
/*  	    break; */
/*  	case ButtonRelease: */
/*  	    handleEvent( x, y, status = PEN_UP ); */
/*  	    break; */
/*  	case LeaveNotify: */
/*  	    break; */
/*  	case EnterNotify: */
/*  	    break; */
/*  	case MotionNotify: */
/*  	    x = report.xmotion.x; */
/*  	    y = report.xmotion.y; */
/*  	    if ( status == PEN_DOWN ) */
/*  		handleEvent( x, y, PEN_DRAG ); */
/*  	    break;    */
/*  	default: */
/*  	    break; */
/*  	} */
/*      } */
}


extern void clearBackground(int);
/**
 * penDown
 * Called when the user has put his foot, no, pen down
 **/
void
penDown( PenEvent *pe )
{
    Point po;
    po.x = pe->x;
    po.y = pe->y;
    addPoint( current, po );
    clearBackground(0xffffff);
}


/**
 * penUp
 * Called when the pen is lifted from the pad
 **/
void
penUp( PenEvent *pe )
{
    Point po;

    po.x = pe->x;
    po.y = pe->y;
    if ( mode == SINGLE_STROKE ) 
      timedOut();
    else
      timer = 1;
}


/**
 * penDragged
 * Called when the user drags the pen over the pad
 **/
void
penDragged( PenEvent *pe )
{
    Point po;

    po.x = pe->x;
    po.y = pe->y; 
    addPoint( current, po );
    
/*      if ( current->nPoints > 2 )  */
/*  	paintPattern(); */
/*      else */
/*  	paint(); */
      }	

