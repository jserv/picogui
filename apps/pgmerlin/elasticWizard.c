/****************************************************************************
 * elasticWizard.c
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

/**
 * elasticWizard.c
 * A wizard implementation using elastic matching
 * More info about this method can be found in my
 * Master's thesis report which at the time of this 
 * writing can be found at: http://www.testbed.era.ericsson.se/~erasshe
 * If it's not there anymore, contact me at: peffis@mindless.com 
 **/   
 

#include <pattern.h>
#include <wizard.h>
#include <preprocessor.h>
#include <filter.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <math.h>
#include <def.h>

#define PI M_PI
#define MAX_PATH 1024
#define MAX_PATTERNS 200
#define ABS( x ) ((x) < 0 ? -(x) : (x))
#define MIN( x, y ) ((x) < (y) ? (x) : (y))
#define MAX( x, y ) ((x) > (y) ? (x) : (y))

static filter fs[] = { sizeNormFilter, smoothFilter, pointDistanceNormFilter };
static Preprocessor *preprocessor;
static Pattern **models[3] = { NULL, NULL, NULL };
static int npatterns[] = { 0, 0, 0};
static BANK currentBank = NATURAL;
static Pattern **samples, *unknown, *currentModel;
static int **lookup = NULL, lookup_n;

/* Some local functions */
void loadPatterns();
Pattern *loadPattern( char *filename );
int distance( int sampleIndex );
void newLookup( int n, int m );
int maxWidth( Pattern *p );

/**
 * setBank
 * Guess what?
 **/
void
setBank( BANK b )
{
    currentBank = b;
}


/**
 * getBank
 * Guess what?
 **/
BANK
getBank()
{
    return currentBank;
}


/**
 * initWizard
 * Prepares the wizard for use
 **/
void 
initWizard()
{
    int i;
    preprocessor = newPreprocessor( fs, 3 );
    loadPatterns();
    for ( i = 0; i < 3; i++ )
	printf("Found %d patterns in %s bank\n", 
	       npatterns[i], 
	       (i == 0 ? "NATURAL" : (i == 1 ? "NUMERAL" : "EXTENDED")));
    setBank( NATURAL );
}


/**
 * initModels
 * Initiates models
 **/
int
initModels()
{
    int i, j;
    for ( i = 0; i < 3; i++ )
    {
	models[i] = (Pattern **) malloc( MAX_PATTERNS * sizeof(Pattern *));
	if ( models[i] == NULL )
	{
	    for ( j = 0; j < i; j++)
		free( models[i] );
	    return 1;
	}
	for ( j = 0; j < MAX_PATTERNS; j++ )
	    models[i][j] = NULL;
	npatterns[i] = 0;
    }
    return 0;
}


/**
 * freeModels
 * Guess what
 **/
void
freeModels()
{
    int i, j;
    for ( i = 0; i < 3; i++ )
    {
	for ( j = 0; j < MAX_PATTERNS; j++ )
	    if ( models[i][j] != NULL )
		deletePattern( models[i][j] );
	if ( models[i] != NULL )
	    free( models[i] );
	models[i] = NULL;
    }
}


/**
 * recognize
 * The magic! Given a pattern it returns the ascii that
 * it thinks is closest
 **/
unsigned char 
recognize( Pattern *p )
{
    unsigned char ascii;
    int mind, tmpd, i, nsamples;
    Pattern *np; 


    /**
     * Check for shift-symbol
     */
    if ( maxWidth( p ) < 6 )
	return '.';
    
    np = apply( preprocessor, p );
    calcSlopes( np );

    /*
     * Do the magic stuff
     */
    samples = models[ getBank() ];
    nsamples = npatterns[ getBank() ];
    unknown = np;
    ascii = samples[0]->ascii;
    mind = distance( 0 );

    for ( i = 1; i < nsamples; i++ )
    {
	tmpd = distance( i );
	if ( tmpd < mind )
	{
	    mind = tmpd;
	    ascii = samples[i]->ascii;
	}
    }
	
    /*
     * cleanup and return
     */
    deletePattern( np );
    return ascii;
}


/**
 * maxWidth
 * Returns the maximum width of the pattern
 **/
int
maxWidth( Pattern *p )
{
    int minx, maxx, miny, maxy, i;
    Point p1 = p->pv[0];
    minx = maxx = p1.x;
    miny = maxy = p1.y;

    for ( i = 1; i < p->nPoints; i++ )
    {
	Point p2 = p->pv[i];
	if (p2.x > maxx)
	    maxx = p2.x;
	if (p2.y > maxy)
	    maxy = p2.y;
	if (p2.x < minx)
	    minx = p2.x;
	if (p2.y < miny)
	    miny = p2.y;
    }
    return MAX( maxx - minx, maxy - miny );
}


/** 
 * A real function for returning the min of two values
 * a macro would not be good here, since arguments can be calculated twice then
 **/
int 
min( int x, int y )
{
    if ( x <= y )
	return x;
    return y;
}

/**
 * slopeDistance
 * The distance between two angular values
 **/
int 
slopeDistance( int a1, int a2 )
{
    int delta = ABS( a1 - a2 );
    return MIN( delta, ((int)( C * 2 * PI )) - delta );
}

/**
 * pointDistance
 * The diatance function between point i in the unknown pattern and
 * point j in the current model
 **/
int
pointDistance( int i, int j )
{
    Point p1 = unknown->pv[i];
    Point p2 = currentModel->pv[j];
/*    printf("SlopeDistance: %d (%d, %d)\n", 
	   slopeDistance( unknown->s[i], currentModel->s[j]),
	   unknown->s[i], currentModel->s[j]); 
	   printf("Pointdistance: %d\n", (p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y)); */
    return (p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y) 
	+ slopeDistance( unknown->s[i], currentModel->s[j] );
}

/**
 * freeLookup
 * Frees the lookup table
 **/
void 
freeLookup()
{
    int i;
    for ( i = 0; i < lookup_n; i++ )
	free( lookup[i] );
    free( lookup );
}


/**
 * newLookup
 * Initializes the lookup-table
 **/
void
newLookup( int n, int m )
{
    int i, j;
    if ( lookup != NULL )
    	freeLookup();

    lookup = (int **) malloc( (lookup_n = n) * sizeof(int *));
    for ( i = 0; i < lookup_n; i++ )
    {
	lookup[i] = (int *) malloc( m * sizeof(int) );
	for ( j = 0; j < m; j++ )
	    lookup[i][j] = -1;
    }

    lookup[0][0] = pointDistance(0, 0);
    for ( j = 1; j < m; j++ )
	lookup[0][j] = lookup[0][j-1] + pointDistance(0, j);
    for ( i = 1; i < n; i++ )
	lookup[i][0] = lookup[i-1][0] + pointDistance(i, 0);
}

/**
 * The actual Recursive Elastic Matching function
 * See my thesis for some nicely printed equations
 **/
int
D( int i, int j )
{
    if ( lookup[i][j] != -1 )
	return lookup[i][j];
    if ( j == 1 )
	return lookup[i][j] = pointDistance(i, 1)
	    + min(D(i-1, 1), D(i-1, 0));
    else
	return lookup[i][j] = pointDistance(i,j)
	    + min(D(i-1, j), min(D(i-1, j-1), D(i-1, j-2)));
}


/**
 * distance
 * Calculates the distance from then unknown pattern to sample number 
 * sampleIndex using elastic matching
 **/
int 
distance( int sampleIndex )
{
    currentModel = samples[ sampleIndex ];
    newLookup( unknown->nPoints, currentModel->nPoints );
    return D( unknown->nPoints - 1, currentModel->nPoints - 1 ) 
	/ currentModel->nPoints;
}


/**
 * loadPatterns
 * Loads the patterns from the directory specified in 
 * the environment variable MERLIN_HOME or in if not set
 * in the current directory
 **/
void
loadPatterns()
{
    char name[MAX_PATH], *ptr;
    DIR *dir;
    struct dirent *entry;
    char *patdir = getenv( "MERLIN_HOME" );
    BANK cb;
    
    if ( models[0] != NULL || models[1] != NULL || models[2] != NULL)
	freeModels();
    if ( initModels() )
    {
	fprintf( stderr, "Could not alloc mem for patterns\n" );
	return;
    }

    if ( patdir == NULL )
	patdir = DEFAULT_HOME;

    dir = opendir(patdir);
    if ( dir == NULL )
    {
	fprintf( stderr, "Could not open %s. No characters loaded\n",
		 patdir );
	return;
    }

    while ( (entry = readdir( dir )) != NULL )
    {
	sprintf( name, "%s/%s", patdir, entry->d_name );
	if ( strstr( entry->d_name, "natural" ) != NULL )
	    cb = NATURAL;
	else
	    if ( strstr( entry->d_name, "numeral" ) != NULL )
		cb = NUMERAL;
	    else
		cb = EXTENDED;
	ptr = name + strlen( name ) - 4;
	if ( ptr > name && strcmp( ptr, ".dat" ) == 0 )
	{
	    models[cb][npatterns[cb]] = loadPattern( name );
	    if ( models[cb][npatterns[cb]] == NULL )
	    {
		fprintf( stderr, "Warning. Could not load pattern: %s\n",
			 name );
		continue;
	    }
	    npatterns[cb]++;
	    if ( npatterns[cb] == MAX_PATTERNS )
	    {
		fprintf( stderr, "Warning. There are %d patterns or more in "
			 " %s. Only %d loaded.\n", 
			 MAX_PATTERNS, patdir, MAX_PATTERNS );
		break;
	    }
	}
    }
    closedir( dir );
}


/**
 * loadPattern
 * Loads a pattern from file filename
 * The format of the file is the format used by Rob Kassel
 * in his PhD-thesis: A comparison of Approaches to On-line
 * Handwritten Character Recognition
 **/
Pattern *
loadPattern( char *filename )
{
    FILE *fp;
    Point po;
    int lineno = 0, readingStroke = 0, foundAscii = 0, tmp;
    char line[80], ascii = 0, *ptr;
    Pattern *p = newPattern( 30, 'a' ), *tp;

    if ( p == NULL )
	return NULL;

    fp = fopen( filename, "r" );
    if ( fp == NULL )
	return NULL;
    
    while ( fgets( line, 80, fp ) != NULL )
    {
	lineno++;
	if ( strstr( line, ".COMMENT" ) != NULL && 
	     strstr( line, "Prompt" ) != NULL )
	{
	    ptr = strchr( line, '"' );
	    if ( ptr == NULL )
	    {
		fprintf( stderr, "Parse error in %s on line %d\n",
			 filename, lineno );
		continue;
	    }

	    ascii = ptr[1];
	    if ( ascii == '#' )
	    {
		sscanf( ptr + 2, "%d", &tmp );
		ascii = (char) tmp;
	    }
	    foundAscii = 1;
	}

	if ( !readingStroke )
	{
	    if ( strstr( line, ".PEN_DOWN" ) )
		readingStroke = 1;
	}
	else
	{
	    if ( strlen( line ) > 0 && !isdigit( line[0] ) )
		readingStroke = 0;
	    
	    if ( strlen( line ) > 0 && isdigit( line[0] ) )
	    {
		if ( sscanf( line, "%d %d", &po.x, &po.y ) != 2 )
		{
		    fprintf( stderr, "Parse error in %s on line %d\n",
			     filename, lineno );
		    continue;
		}   
		po.y = 3058 - po.y;
		addPoint( p, po );
	    }
	}
    }
    fclose( fp );
    if ( !foundAscii || p->nPoints == 0 )
    {
	deletePattern( p );
	p = NULL;
    }
    else
    {
	tp = apply( preprocessor, p );
	deletePattern( p );
	p = tp;
    	p->ascii = (unsigned char) ascii;
	calcSlopes( p );
    }
    return p;
}
