/*********************************************************
 * filter.h
 *
 * Copyright (c) Stefan Hellkvist
 *********************************************************/

#ifndef FILTER_H
#define FILTER_H

#include <pattern.h>

/*
 * sizeNormFilter: Normalizes the size of the pattern to a 100x100-square
 */
Pattern *sizeNormFilter( Pattern *p );

/*
 * pointDistanceNormFilter: Reduces unnessesary points, removing
 * points that are too close together.
 */
Pattern *pointDistanceNormFilter( Pattern *p );

/*
 * smoothFilter: Using averaging to reduce the amount of digitizing errors
 */ 
Pattern *smoothFilter( Pattern *p );

#endif
