/* $Id: eventq.c,v 1.18 2002/01/18 11:14:34 lonetech Exp $
 *
 * eventq.c - This implements the post_event function that the widgets
 *            use to send events to the client.  It stores these in a
 *            ring buffer and allows the application to:
 *                test for an event's presence
 *                poll events
 *                wait for events
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2002 Micah Dowty <micahjd@users.sourceforge.net>
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

   Normally data is NULL. To send a data event, set event to something using
   PG_EVENTCODING_DATA, put the size in param, and point data at your cargo.
*/
void post_event(int event,struct widget *from,long param,int owner,char *data) {
  handle hfrom;
  struct conbuf *cb;

  /* Is there a callback to absorb this event? */
  if (from && from->callback && from->callback(event,from,param,owner,data))
    return;

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
    if ((event & PG_EVENTCODINGMASK) == PG_EVENTCODING_DATA)
      send(owner,data,param,0);
  }
  else {
    /* Store it in the queue */
    cb = find_conbuf(owner);
    if (!cb) return;   /* Sanity check */
    
    /* Free any data that might be left in this node
     * (in case we had an overflow)
     */
    if (cb->in->data)
      g_free(cb->in->data);

    /* If this is a data event, duplicate the data for queueing */
    if ((event & PG_EVENTCODINGMASK) == PG_EVENTCODING_DATA) {
      if (iserror(g_malloc( (void**)&cb->in->data, param )))
	return;
      memcpy(cb->in->data,data,param);
    }
    else
      cb->in->data = NULL;

    cb->in->event = event;
    cb->in->from = hfrom;
    cb->in->param = param;
    
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
  struct widget *w;
  int badwidget = 0;

  cb = find_conbuf(owner);
  if (!cb) return NULL;   /* Sanity check */

  /* Return if the queue's empty */
  if (cb->in == cb->out) 
    return NULL;

  /* Get the next event */
  q = cb->out;

  /* Make sure the widget that sent the queued event still exists! */
  badwidget = iserror(rdhandle((void **) &w,PG_TYPE_WIDGET,owner,q->from));

  /* Remove it from the queue */
  if (remove || badwidget)
    if ((++cb->out) >= (cb->q+EVENTQ_LEN))   /* Wrap around */
      cb->out = cb->q;

  /* If that widget was bad, try again */
  if (badwidget)
    return get_event(owner,remove);

  return q;
}

/* Returns the number of pending events for a particular connection */
int check_event(int owner) {
  struct conbuf *cb;

  /* Find the connection buffer */
  cb = find_conbuf(owner);
  if (!cb) return 0;   /* Sanity check */

  /* The queue is empty? */
  if (cb->in == cb->out) 
    return 0;

  /* Has cb->in wrapped around but cb->out has not? */
  if (cb->in < cb->out)
    return EVENTQ_LEN + (cb->in - cb->out); 

  /* Nope */
  return cb->in - cb->out; 
}

/* The End */









