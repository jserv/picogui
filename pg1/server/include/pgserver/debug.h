#include <stdio.h>

#ifndef _DEBUG_H_
#define _DEBUG_H_

#if (defined(__APPLE__) && defined(__MACH__)) // Mac OS X and Darwin 
/* I can't get this to work under OS X, Apple really screwed up the 
 * preprocessor somehow. So for now, disable it.
 */
#define DBG
#else

#ifdef DEBUG_FILE
#define DBG(fmt, args...) printf( "%s: " fmt, __FUNCTION__ , ## args); fflush(stdout)
#else
#define DBG(fmt, args...)
#endif

#endif /* MacOS X */
#endif /* _DEBUG_H_ */

