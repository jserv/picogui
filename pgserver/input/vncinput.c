/* $Id: vncinput.c,v 1.2 2003/01/19 10:41:07 micahjd Exp $
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

extern rfbScreenInfoPtr vncserver_screeninfo;      /* VNC server's main structure */
int vncinput_pipe[2];                              /* Pipe for sending events from the VNC server
						    * thread to the input driver in this thread.*/

/* The structure sent across our pipe */
struct vncinput_packet {
  bool createCursor, destroyCursor;
  struct cursor **pcursor;
  struct cursor *cursor;
  u32 trigger;
  union trigparam param;
};


/******************************************************** VNC Hooks */

void vncinput_clientGoneHook(struct _rfbClientRec* cl) {
  struct vncinput_packet pkt;
  memset(&pkt, 0, sizeof(pkt));
  pkt.pcursor = (struct cursor**) &cl->clientData;
  pkt.cursor = (struct cursor*) cl->clientData;

  pkt.destroyCursor = 1;

  write(vncinput_pipe[1], &pkt, sizeof(pkt));
}

enum rfbNewClientAction vncinput_newClientHook(struct _rfbClientRec* cl) {
  struct vncinput_packet pkt;
  memset(&pkt, 0, sizeof(pkt));
  pkt.pcursor = (struct cursor**) &cl->clientData;
  pkt.cursor = (struct cursor*) cl->clientData;

  pkt.createCursor = 1;

  write(vncinput_pipe[1], &pkt, sizeof(pkt));
  cl->clientGoneHook = &vncinput_clientGoneHook;
}

void vncinput_kbdAddEvent(Bool down, KeySym keySym, struct _rfbClientRec* cl) {
}

void vncinput_kbdReleaseAllKeys(struct _rfbClientRec* cl) {
}

void vncinput_ptrAddEvent(int buttonMask, int x, int y, struct _rfbClientRec* cl) {
  struct vncinput_packet pkt;
  memset(&pkt, 0, sizeof(pkt));
  pkt.pcursor = (struct cursor**) &cl->clientData;
  pkt.cursor = (struct cursor*) cl->clientData;

  pkt.trigger = PG_TRIGGER_PNTR_STATUS;
  pkt.param.mouse.x = x;
  pkt.param.mouse.y = y;
  pkt.param.mouse.btn = buttonMask;

  write(vncinput_pipe[1], &pkt, sizeof(pkt));
}


/******************************************************** Public Methods */

g_error vncinput_init(void) {
  /* Create a pipe for sending events from the VNC server thread to the input driver */
  pipe(vncinput_pipe);

  vncserver_screeninfo->newClientHook = &vncinput_newClientHook;
  vncserver_screeninfo->kbdAddEvent = &vncinput_kbdAddEvent;
  vncserver_screeninfo->kbdReleaseAllKeys = &vncinput_kbdReleaseAllKeys;
  vncserver_screeninfo->ptrAddEvent = &vncinput_ptrAddEvent;
  return success;
}

void vncinput_close(void) {
  close(vncinput_pipe[0]);
  close(vncinput_pipe[1]);
}

void vncinput_fd_init(int *n,fd_set *readfds,struct timeval *timeout) {
  if ((*n)<(vncinput_pipe[0]+1))
    *n = vncinput_pipe[0]+1;
  if (vncinput_pipe[0]>0)
    FD_SET(vncinput_pipe[0],readfds);
}

int vncinput_fd_activate(int fd) {
  struct vncinput_packet pkt;

  if (fd != vncinput_pipe[0])
    return 0;
  read(vncinput_pipe[0], &pkt, sizeof(pkt));

  if (pkt.createCursor)
    cursor_new(pkt.pcursor, NULL, -1);

  if (pkt.destroyCursor) {
    pointer_free(-1, pkt.cursor);
  }

  if (pkt.trigger & PG_TRIGGERS_MOUSE)
    pkt.param.mouse.cursor = pkt.cursor;

  if (pkt.trigger)
    infilter_send(NULL, pkt.trigger, &pkt.param);

  return 1;
}


/******************************************************** Registration */

g_error vncinput_regfunc(struct inlib *i) {
  i->init = &vncinput_init;
  i->close = &vncinput_close;
  i->fd_init = &vncinput_fd_init;
  i->fd_activate = &vncinput_fd_activate;
  return success;
}

/* The End */
