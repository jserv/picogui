/****************************************************************************
 * pattern.c
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

#include <pattern.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <def.h>

#define PI M_PI

Pattern *
newPattern( int initialSize, unsigned char ascii )
{
    Pattern *p = (Pattern *) malloc ( sizeof( Pattern ) );
    if ( p == NULL )
	return NULL;
    p->pv = (Point *) malloc ( initialSize * sizeof( Point ) );
    if ( p->pv == NULL )
    {
	free( p );
	return NULL;
    }

    p->s = NULL;
    p->nPoints = 0;
    p->ascii = ascii;
    p->size = initialSize;

    return p;
}


void
deletePattern( Pattern *p )
{
    if ( p->s != NULL)
	free( p->s );
    free( p->pv );
    free( p );
}

Point *
growVector( Point *old, int oldSize, int newSize )
{
    Point *np = (Point *) malloc( newSize * sizeof( Point ) );
    if (np == NULL)
	return NULL;
    memcpy( np, old, oldSize * sizeof( Point ) );
    return np;
}

void
addPoint( Pattern *p, Point po )
{
    po.isLast = 0;
    p->pv[ p->nPoints++ ] = po;
    if ( p->nPoints == p->size )
    {
	Point *old = p->pv;
	int newSize = p->size * 2;
	p->pv = growVector( p->pv, p->size, newSize );
	if ( p->pv != NULL)  /* Did we manage to grow? */
	{
	    p->size = newSize;  /* Yes, use new vector */
	    free( old );
	}
	else
	{
	    p->pv = old;  /* No, use old */
	    p->nPoints--; /* Go back one step */
	}
    }
}

int
pointSlope( Point p1, Point p0)
{
    float hyp = (float) sqrt( (( p1.x - p0.x ) * ( p1.x - p0.x )) 
			      + (( p1.y - p0.y ) * ( p1.y - p0.y )) );
    float alpha = (float) acos( ( p1.x - p0.x ) / hyp );
    if (hyp == 0)
	fprintf(stderr, "Hyp is zero!!!!!!!!!!!!!!!!!!!!!!!!!!\n");

    if ( p1.y < p0.y )
	alpha = 2 * PI - alpha;
    return (int) ( C * alpha );
}

void
calcSlopes( Pattern *p )
{
    int i;
    if ( p->s != NULL )
	free( p->s );
    p->s = (int *) malloc( p->nPoints * sizeof( int ) );
    if ( p->nPoints == 1 )
    {
	p->s[0] = 0;
	return;
    }

    for ( i = 0; i < p->nPoints - 1; i++ )
	p->s[i] = pointSlope( p->pv[ i + 1 ], p->pv[ i ] );
    p->s[ p->nPoints - 1 ] = p->s[ p->nPoints - 2 ];
}
