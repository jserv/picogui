/*****************************************************************************
 * preprocessor.c
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


#include <preprocessor.h>
#include <pattern.h>
#include <stdlib.h>

Preprocessor *
newPreprocessor( filter *fs, int nFilters )
{
    Preprocessor *pnew = (Preprocessor *) malloc( sizeof( Preprocessor ) );
    if (pnew == NULL)
	return NULL;
    pnew->fs = fs;
    pnew->nFilters = nFilters;
    return pnew;
}

void
deletePreprocessor( Preprocessor *p )
{
    free( p );
}

Pattern *
applyFilter( filter f, Pattern *p)
{
    return f( p );
}


Pattern *
apply( Preprocessor *p, Pattern *pat )
{
    Pattern *np, *old;
    int i;
    if ( p->nFilters == 0 )
	return pat;

    np = applyFilter( p->fs[ 0 ], pat );
    for ( i = 1; i < p->nFilters; i++)
    {
	old = np;
	np = applyFilter( p->fs[ i ], np );
	deletePattern( old );
    }
    return np;
}
