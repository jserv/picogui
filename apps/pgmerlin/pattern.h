/*********************************************************
 * pattern.h
 *
 * Copyright (c) Stefan Hellkvist
 *********************************************************/

#ifndef PATTERN_H
#define PATTERN_H

#include <point.h>


struct pattern
{
    /* Public part. Use freely */
    Point *pv;   /* A vector of points */
    int *s;    /* The slope at these points */
    int nPoints; /* The number of points in the vector and slopes */
    unsigned char ascii; /* The meaning of this pattern */

    /* Private part. Don't mess around */
    int size;    /* The size of the vectors */
};

typedef struct pattern Pattern;

/* Creates a new pattern */
Pattern *newPattern(int initialSize, unsigned char ascii);

/* Frees up a pattern */
void deletePattern(Pattern *p);

/* Adds a point to a pattern */
void addPoint(Pattern *p, Point po);

/* Calculates the slopes in a pattern */
void calcSlopes(Pattern *p);

#endif
