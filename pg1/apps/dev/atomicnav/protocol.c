/* $Id$
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









