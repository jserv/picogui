#include <stdio.h>

#ifndef _DEBUG_H_
#define _DEBUG_H_

#if (defined(__APPLE__) && defined(__MACH__)) // Mac OS X and Darwin 
/* I can't get this to work under OS X, Apple really screwed up the 
 * preprocessor somehow. So for now, disable it.
 */
#define DBG
#else

#ifndef _MSC_VER
#ifdef DEBUG_FILE
#include <stdio.h>
#define DBG(fmt, args...) fprintf(stderr, "%s: " fmt, __FUNCTION__ , ## args); fflush(stderr)
#else
#define DBG
#endif
#else
_inline void DBG(fmd, ...) {}
#endif /* _MSC_VER */

#endif /* MacOS X */
#endif /* _DEBUG_H_ */

