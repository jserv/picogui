/*
 * eventq.c - This implements the post_event function that the widgets
 *            use to send events to the client.  It stores these in a
 *            ring buffer and allows the application to:
 *                test for an event's presence
 *                poll events
 *                wait for events
 * $Revision: 1.1 $
 * 
 * Micah Dowty <micah@homesoftware.com>
 * 
 * This file is released under the GPL. Please see the file COPYING that
 * came with this distribution.
 */

#include <request.h>

void post_event(int event,struct widget *from,long param) {
  handle hfrom;
  int owner;

  /* Determine the owner of the originating widget */
  hfrom = hlookup(from,&owner);
  if (!(hfrom && owner)) return;

  /* Is the owner already waiting for an event? */
  if (FD_ISSET(owner,&evtwait)) {
    struct response_event rsp;
    rsp.type = htons(RESPONSE_EVENT);
    rsp.event = htons(event);
    rsp.from = htonl(hfrom);
    rsp.param = htonl(param);
    send(owner,&rsp,sizeof(rsp),0);
    FD_CLR(owner,&evtwait);
    return;
  }

  /* Store it in the queue */
}

struct event *get_event(int owner,int remove) {

  return NULL;
}

/* The End */









