/* $Id: eventq.c,v 1.10 2001/02/17 05:18:41 micahjd Exp $
 *
 * eventq.c - This implements the post_event function that the widgets
 *            use to send events to the client.  It stores these in a
 *            ring buffer and allows the application to:
 *                test for an event's presence
 *                poll events
 *                wait for events
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000,2001 Micah Dowty <micahjd@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 * Contributors:
 * 
 * 
 * 
 */

#include <pgserver/common.h>
#include <pgserver/pgnet.h>

/* Either the originating widget or the owner must be specified.
   If only 'from' is nonzero, the owner will be looked up, and the event
   sent from the widget.
   If owner is nonzero, it will be sent as a global event.

   Normally data is NULL. To send a data event, set event to PG_WE_DATA,
   put the size in param, and point data at your cargo.
*/
void post_event(int event,struct widget *from,long param,int owner,char *data) {
  handle hfrom;
  struct conbuf *cb;

  /* Determine the owner of the originating widget */
  hfrom = hlookup(from,&owner);
  if (!(hfrom || owner)) return;

  /* Is the owner already waiting for an event? */
  if (FD_ISSET(owner,&evtwait)) {
    struct pgresponse_event rsp;

    FD_CLR(owner,&evtwait);
#if defined(DEBUG_NET) || defined(DEBUG_EVENT)
    printf("Client (#%d) removed from waiting list, sending event %d\n",owner,event);
#endif
     
    rsp.type = htons(PG_RESPONSE_EVENT);
    rsp.event = htons(event);
    rsp.from = htonl(hfrom);
    rsp.param = htonl(param);
    send(owner,&rsp,sizeof(rsp),0);
    if (event==PG_WE_DATA)
      send(owner,data,param,0);
  }
  else {
    /* Store it in the queue */
    cb = find_conbuf(owner);
    if (!cb) return;   /* Sanity check */
    
    cb->in->event = event;
    cb->in->from = hfrom;
    cb->in->param = param;
    cb->in->data = data;
    
    if ((++cb->in) >= (cb->q+EVENTQ_LEN))   /* Wrap around */
      cb->in = cb->q;
    
    if (cb->in == cb->out) {
      /* If the event queue overflows, keep only the newest items */

      if ((++cb->out) >= (cb->q+EVENTQ_LEN))   /* Wrap around */
	cb->out = cb->q;

#ifdef DEBUG_NET
      printf("*** Event queue overflow!\n");
#endif
    }
  }
}

struct event *get_event(int owner,int remove) {
  struct event *q;
  struct conbuf *cb;

  cb = find_conbuf(owner);
  if (!cb) return NULL;   /* Sanity check */

  /* Either we're messed up (overflowed) or (more likely)
     the queue is just empty */
  if (cb->in == cb->out) return NULL;

  q = cb->out;

  if (remove)
    if ((++cb->out) >= (cb->q+EVENTQ_LEN))   /* Wrap around */
      cb->out = cb->q;

  return q;
}

/* The End */









