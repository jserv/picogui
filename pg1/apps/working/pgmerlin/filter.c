/****************************************************************************
 * filter.c
 *
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

#include <filter.h>
#include <pattern.h>
#include <point.h>
#include <stdlib.h>
#include <stdio.h>

#define NORM_WIDTH 100
#define NORM_HEIGHT 100
#define THRESHOLD 36

#define MAX( x, y ) ( (x) > (y) ? (x) : (y))

Pattern *
sizeNormFilter( Pattern *p )
{
    int i, xmin, xmax, ymin, ymax; 
    float xm, ym, dx, dy, delta, centerx, centery;
    Point tmp, po;
    Pattern *np;

    if ( p == NULL )
	return NULL;
    np = newPattern( p->size, p->ascii );
    if ( np == NULL )
	return NULL;
    
    tmp = p->pv[ 0 ];
    xmin = xmax = tmp.x;
    ymin = ymax = tmp.y;

    for ( i = 1; i < p->nPoints; i++ )
    {
	tmp = p->pv[ i ];
	if ( tmp.x < xmin )
	    xmin = tmp.x;
	if ( tmp.y < ymin )
	    ymin = tmp.y;
	if ( tmp.x > xmax )
	    xmax = tmp.x;
	if ( tmp.y > ymax )
	    ymax = tmp.y;
    }
    
    /* Center of gravity */
    xm = ( xmax + xmin ) / 2.0;
    ym = ( ymax + ymin ) / 2.0;

    /* the spreads */
    dx = ( xmax - xmin ) / 2.0;
    dy = ( ymax - ymin ) / 2.0;
            
    delta = MAX( dx, dy );

    /* Start to transform coordinates */
    centerx = (NORM_WIDTH - 1) / 2.0;
    centery = (NORM_HEIGHT - 1) / 2.0;

    for ( i = 0; i < p->nPoints; i++ )
    {
	tmp = p->pv[ i ];
	po.x = (int) ( centerx + centerx * (( tmp.x - xm ) / delta ));
	po.y = (int) ( centery + centery * (( tmp.y - ym ) / delta ));
	addPoint( np, po );
    }
    
    return np;
}


int 
distanceSq( Point p0, Point p1 )
{
    return (( p0.x - p1.x ) * ( p0.x - p1.x )) 
	+ (( p0.y - p1.y ) * ( p0.y - p1.y )); 
}

Pattern *
pointDistanceNormFilter( Pattern *p )
{
    Pattern *np;
    Point last, tmp;
    int i;

    if ( p == NULL )
	return NULL;
    np = newPattern( p->size, p->ascii );
    if ( np == NULL )
	return NULL;

    last = p->pv[ 0 ];
    addPoint( np, last );
    for ( i = 1; i < p->nPoints; i++ )
    {
	tmp = p->pv[ i ];
	if ( distanceSq( tmp, last ) >= THRESHOLD )
	{
	    last = tmp;
	    addPoint( np, last );
	}
	/*
	else
	{
	  printf( "Not big enough: %d\n", distanceSq( tmp, last ));
	  printf( "    (%d, %d)-(%d, %d)\n", last.x, last.y, tmp.x,tmp.y);
	  
	}
	*/
    }

    return np;
}
    

Pattern *
smoothFilter( Pattern *p )
{
    Pattern *np;
    Point p1, p2, p3, tmp;
    int i;

    if ( p == NULL )
	return NULL;
    np = newPattern( p->size, p->ascii );
    if ( np == NULL )
	return NULL;

    addPoint( np, p->pv[ 0 ] );
    for ( i = 1; i < p->nPoints - 1; i++ )
    {
	p1 = p->pv[ i - 1 ];
	p2 = p->pv[ i ];
	p3 = p->pv[ i + 1 ];
	tmp.x = ( p1.x + p2.x + p3.x ) / 3;
	tmp.y = ( p1.y + p2.y + p3.y ) / 3;
	addPoint( np, tmp );
    }
    addPoint( np, p->pv[ p->nPoints - 1 ] );
    return np;
}
