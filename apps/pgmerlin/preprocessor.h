#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include <pattern.h>

typedef Pattern * (*filter) ( Pattern * );

struct preprocessor
{
    filter *fs;
    int nFilters;
};
typedef struct preprocessor Preprocessor;

/* Creates a new preprocessor with nFilters filters */
Preprocessor *newPreprocessor( filter *fs, int nFilters );

/* Destroys a preprocessor */
void deletePreprocessor( Preprocessor *p );

/* 
 * Applies a preprocessor to a pattern, generating a new pattern
 * The old pattern is not destroyed.
 */
Pattern *apply( Preprocessor *p, Pattern *pat );

#endif
