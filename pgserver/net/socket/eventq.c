/* $Id: eventq.c,v 1.5 2000/06/11 17:59:18 micahjd Exp $
 *
 * eventq.c - This implements the post_event function that the widgets
 *            use to send events to the client.  It stores these in a
 *            ring buffer and allows the application to:
 *                test for an event's presence
 *                poll events
 *                wait for events
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micah@homesoftware.com>
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

#include <pgnet.h>

/* Either the originating widget or the owner must be specified.
   If only 'from' is nonzero, the owner will be looked up, and the event
   sent from the widget.
   If owner is nonzero, it will be sent as a global event.
*/
void post_event(int event,struct widget *from,long param,int owner) {
  handle hfrom;

  /* Determine the owner of the originating widget */
  hfrom = hlookup(from,&owner);
  if (!(hfrom || owner)) return;

  /* Is the owner already waiting for an event? */
  if (FD_ISSET(owner,&evtwait)) {
    struct response_event rsp;

    FD_CLR(owner,&evtwait);
#ifdef DEBUG
    printf("Client (#%d) removed from waiting list\n",owner);
#endif

    rsp.type = htons(RESPONSE_EVENT);
    rsp.event = htons(event);
    rsp.from = htonl(hfrom);
    rsp.param = htonl(param);
    send(owner,&rsp,sizeof(rsp),0);
    return;
  }

  /* Store it in the queue */
}

struct event *get_event(int owner,int remove) {

  return NULL;
}

/* The End */









