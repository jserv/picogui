/* $Id: protocol.c,v 1.1 2002/01/07 06:28:31 micahjd Exp $
 *
 * protocol.c - Table of supported protocols
 */

#include <stdio.h>     /* For NULL */
#include "protocol.h"

extern struct protocol p_http;
extern struct protocol p_file;

struct protocol *supported_protocols[] = {
  &p_http,
  &p_file,
  NULL,
};
  
/* The End */









