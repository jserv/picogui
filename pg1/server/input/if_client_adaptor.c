/* $Id$
 *
 * if_client_adaptor.c - Send events to clients, for client-side input filters
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 * 
 * Contributors:
 * 
 * 
 * 
 */

#include <pgserver/common.h>
#include <picogui/network.h>
#include <pgserver/input.h>

/*************************************************** Input filter *****/

void infilter_client_adaptor_handler(struct infilter *self, u32 trigger, union trigparam *param) {
  union pg_client_trigger cli_trig;
  int i;
  memset(&cli_trig,0,sizeof(cli_trig));

  /* Fill in the easy bits 
   */
  cli_trig.content.type = trigger;
  cli_trig.content.infilter_from = hlookup(self,NULL);

  /* Fill in trigger-specific params 
   */
  if (trigger & PG_TRIGGERS_KEY) {
    cli_trig.content.u.kbd.key     = param->kbd.key;
    cli_trig.content.u.kbd.mods    = param->kbd.mods;
    cli_trig.content.u.kbd.flags   = param->kbd.flags;
    cli_trig.content.u.kbd.consume = param->kbd.consume;
    cli_trig.content.u.kbd.divtree = param->kbd.divtree;
  }
  else if (trigger & PG_TRIGGERS_MOUSE) {
    cli_trig.content.u.mouse.x              = param->mouse.x;
    cli_trig.content.u.mouse.y              = param->mouse.y;
    cli_trig.content.u.mouse.btn            = param->mouse.btn;
    cli_trig.content.u.mouse.chbtn          = param->mouse.chbtn;
    cli_trig.content.u.mouse.pressure       = param->mouse.pressure;
    cli_trig.content.u.mouse.is_logical     = param->mouse.is_logical;
    cli_trig.content.u.mouse.cursor_handle  = hlookup(param->mouse.cursor,NULL); 
    cli_trig.content.u.mouse.ts_calibration = param->mouse.ts_calibration;
    cli_trig.content.u.mouse.divtree        = param->mouse.divtree;
  }
  else if (trigger & PG_TRIGGER_MOTIONTRACKER) {
    memcpy(&cli_trig.content.u.motion, &param->motion, sizeof(param->motion));
  }
  
  /* Convert it to network byte order
   */
  for (i=0;i<(sizeof(cli_trig.array)/sizeof(cli_trig.array[0]));i++)
    cli_trig.array[i] = htonl(cli_trig.array[i]);

  /* Send to the client's event queue
   */
  post_event(PG_NWE_INFILTER, NULL, sizeof(cli_trig), self->owner, &cli_trig);
}

struct infilter infilter_client_adaptor = {
  /*accept_trigs:  */0,
  /*absorb_trigs:  */0,
       /*handler:  */&infilter_client_adaptor_handler,
};

/*************************************************** Public Utilities *****/


/* Create an input filter to act as an adaptor between pgserver and
 * client-side input filters.
 */
g_error infilter_client_create(handle insertion, u32 accept_trigs, 
			       u32 absorb_trigs, handle *h, int owner) {
  struct infilter *insert_after;
  struct infilter **i;
  g_error e;

  /* Find the insertion point (it can be owned by anyone) 
   */
  e = rdhandle((void**)&insert_after,PG_TYPE_INFILTER,-1,insertion);
  errorcheck;
  if (insert_after)
    i = &insert_after->next;
  else
    i = &infilter_list;

  /* Insert the "adaptor" that sends events to the client 
   */
  e = infilter_insert(i, h, owner, &infilter_client_adaptor);
  errorcheck;

  (*i)->owner = owner;
  (*i)->accept_trigs = accept_trigs;
  (*i)->absorb_trigs = absorb_trigs;

  return success;
}


/* Given a pg_client_trigger in network byte order this will reconstruct
 * the trigparam union from it, and dispatch it to the proper input filter.
 */
g_error infilter_client_send(union pg_client_trigger *client_trig) {
  union trigparam tp;
  int i;
  struct infilter *from;
  g_error e;
  memset(&tp,0,sizeof(tp));

  /* Convert it all to host byte order 
   */
  for (i=0;i<(sizeof(client_trig->array)/sizeof(client_trig->array[0]));i++)
    client_trig->array[i] = ntohl(client_trig->array[i]);

  /* Reconstruct the trigparam union, depending on trigger type 
   */
  if (client_trig->content.type & PG_TRIGGERS_KEY) {
    tp.kbd.key     = client_trig->content.u.kbd.key;
    tp.kbd.mods    = client_trig->content.u.kbd.mods;
    tp.kbd.flags   = client_trig->content.u.kbd.flags;
    tp.kbd.consume = client_trig->content.u.kbd.consume;
    tp.kbd.divtree = client_trig->content.u.kbd.divtree;
  }
  else if (client_trig->content.type & PG_TRIGGERS_MOUSE) {
    tp.mouse.x          = client_trig->content.u.mouse.x;
    tp.mouse.y          = client_trig->content.u.mouse.y;
    tp.mouse.btn        = client_trig->content.u.mouse.btn;
    tp.mouse.chbtn      = client_trig->content.u.mouse.chbtn;
    tp.mouse.pressure   = client_trig->content.u.mouse.pressure;
    tp.mouse.is_logical = client_trig->content.u.mouse.is_logical;
    tp.mouse.divtree    = client_trig->content.u.mouse.divtree;
    e = rdhandle((void**)&tp.mouse.cursor, PG_TYPE_CURSOR, -1,
		 client_trig->content.u.mouse.cursor_handle);
    tp.mouse.ts_calibration = client_trig->content.u.mouse.ts_calibration;
    errorcheck;
  }
  else if (client_trig->content.type & PG_TRIGGER_MOTIONTRACKER) {
    memcpy(&tp.motion, &client_trig->content.u.motion, sizeof(tp.motion));
  }

  /* Get the source infilter */
  e = rdhandle((void**)&from,PG_TYPE_INFILTER,-1,client_trig->content.infilter_from);
  errorcheck;

  /* Send it out */
  infilter_send(from, client_trig->content.type, &tp);

  return success;
} 

/* The End */
