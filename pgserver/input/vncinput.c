/* $Id: vncinput.c,v 1.1 2003/01/19 09:25:51 micahjd Exp $
 *
 * vncinput.h - input driver for VNC events
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
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
#include <pgserver/input.h>
#include <pgserver/configfile.h>
#include "../video/libvncserver/rfb.h"

extern rfbScreenInfoPtr vncserver_screeninfo;


/******************************************************** VNC Hooks */

void vncinput_clientGoneHook(struct _rfbClientRec* cl) {
  struct cursor *cursor = (struct cursor *) cl->clientData;
  pointer_free(-1, cursor);
}

enum rfbNewClientAction vncinput_newClientHook(struct _rfbClientRec* cl) {
  /* Create a cursor for this client, and arrange for it to be destroyed later */
  cl->clientData = NULL;
  cursor_new((struct cursor **) &cl->clientData, NULL, -1);
  cl->clientGoneHook = &vncinput_clientGoneHook;
  return RFB_CLIENT_ACCEPT;
}

void vncinput_kbdAddEvent(Bool down, KeySym keySym, struct _rfbClientRec* cl) {
}

void vncinput_kbdReleaseAllKeys(struct _rfbClientRec* cl) {
}

void vncinput_ptrAddEvent(int buttonMask, int x, int y, struct _rfbClientRec* cl) {
  struct cursor *cursor = (struct cursor *) cl->clientData;
  infilter_send_pointing(PG_TRIGGER_PNTR_STATUS, x, y, buttonMask, cursor);
}


/******************************************************** Public Methods */

g_error vncinput_init(void) {
  vncserver_screeninfo->newClientHook = &vncinput_newClientHook;
  vncserver_screeninfo->kbdAddEvent = &vncinput_kbdAddEvent;
  vncserver_screeninfo->kbdReleaseAllKeys = &vncinput_kbdReleaseAllKeys;
  vncserver_screeninfo->ptrAddEvent = &vncinput_ptrAddEvent;
  return success;
}

/* Merge libvncserver's fd_set with ours */
void vncinput_fd_init(int *n,fd_set *readfds,struct timeval *timeout) {
  int i;
  for (i=0;i<=vncserver_screeninfo->maxFd;i++)
    if (FD_ISSET(i,&vncserver_screeninfo->allFds))
      FD_SET(i,readfds);
  if (*n < vncserver_screeninfo->maxFd+1)
    *n = vncserver_screeninfo->maxFd+1;
}

void vncinput_poll(void) {
  rfbProcessEvents(vncserver_screeninfo,0);
}


/******************************************************** Registration */

g_error vncinput_regfunc(struct inlib *i) {
  i->init = &vncinput_init;
  i->fd_init = &vncinput_fd_init;
  i->poll = &vncinput_poll;
  return success;
}

/* The End */
