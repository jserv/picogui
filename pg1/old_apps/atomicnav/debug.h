#include <stdio.h>

#ifndef _DEBUG_H_
#define _DEBUG_H_

#ifdef DEBUG
#define DBG(fmt, args...) printf( "%s: " fmt, __FUNCTION__ , ## args); fflush(stdout)
#else
#define DBG(fmt, args...)
#endif

#endif /* _DEBUG_H_ */
