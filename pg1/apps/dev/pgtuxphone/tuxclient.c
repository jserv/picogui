/* $Id$
 *
 * tuxclient.c - Network interface to tuxphone
 *
 * Copyright (C) 2001 Micah Dowty <micahjd@users.sourceforge.net>
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
 */


/* Gobs of headers for networking */
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <netdb.h>
#include <stdlib.h>
#include <picogui.h>
#include <time.h>

#include "phonecall.h"
#include "tuxphone.h"
#include "tuxclient.h"

int phone_fd;

/* Keep track of the time the phone last ringed */
time_t last_ring;

/* Time after the last ring when a call is considered unanswered */
#define RING_TIMEOUT 6

int phone_open(void) {
  int fd;
  struct sockaddr_in server;
  struct hostent *he;
  const char *hostname;

  hostname = getenv("tuxphone");
  if (!hostname)
    hostname = "127.0.0.1";
  he = gethostbyname(hostname);
  if (!he)
    return -1;

  fd = socket(AF_INET,SOCK_STREAM,0);

  server.sin_family = AF_INET;
  server.sin_port = htons(TUXPHONE_PORT);
  server.sin_addr = *((struct in_addr *) he->h_addr);

  memset(&(server.sin_zero),0,8);
  if (connect(fd,(struct sockaddr *) &server,sizeof(server))<0)
    return -1;
  
  return fd;
}

void phone_register_events(int fd, long mask) {
  CLIENT_PACKET pkt;
  pkt.header.opcode = htons(OPCODE_REGISTER_EVENTS);
  pkt.reg.mode = htons(MODE_REGISTER_EVENT);
  pkt.reg.events = htonl(mask);
  write(fd,&pkt,sizeof(pkt));
}

void phone_dial(int fd, const char *number) {
  CLIENT_PACKET pkt;
  pkt.header.opcode = htons(OPCODE_DIAL);
  strncpy(pkt.dial.digits,number,32);
  write(fd,&pkt,sizeof(pkt));
}

void phone_get_event(int fd) {
  static PKT_EVENT evt;
  static int hook_s = 1, hook_h = 1;
  static union pg_client_trigger trig;

  recv(fd,&evt,sizeof(evt),0);
  if (ntohs(evt.header.opcode) == OPCODE_CLIENT_EVENT)
    switch (ntohs(evt.header.type)) {
      
    case RING_EVENT:
      if (ntohs(evt.ring.ring_state)) {
	/* Make sure we've marked this as an incoming call */
	if (current_call->status != CALL_INCOMING)
	  show_call_info(new_call(CALL_INCOMING));
      }
      last_ring = time(NULL);
      break;
      
    case CID_EVENT:
      /* Caller ID code not tested!
       * (I don't have caller ID)
       */
      set_call_id(current_call,evt.cid.name,evt.cid.number);
      break;
      
    case HOOK_EVENT:
      /* Maintain hook status for speakerphone and handset */
      evt.hook.state = ntohs(evt.hook.state);
      if (ntohs(evt.hook.destination))
	hook_s = evt.hook.state;
      else
	hook_h = evt.hook.state;

      /* If we just changed state, update the GUI */
      if ( (hook_s && hook_h) == evt.hook.state) 
	if (evt.hook.state) {
	  /* End a call */
	  hide_call_info();
	  set_call_status(current_call,CALL_COMPLETED);
	  archive_call(current_call);
	}
	else {
	  /* Begin/answer a call */
	  if (current_call && current_call->status == CALL_INCOMING)
	    set_call_status(current_call,CALL_INPROGRESS);
	  else
	    show_call_info(new_call(CALL_INPROGRESS));
	}
      break;

      /* Convert on-hook dialing into keyboard events */      
    case ONHOOK_DIAL_EVENT:
      trig.content.u.kbd.key = evt.ohdial.key;
      if (ntohs(evt.ohdial.state)) {
	/* Press */
	trig.content.type = PG_TRIGGER_KEYDOWN;
	pgInFilterSend(&trig);
	trig.content.type = PG_TRIGGER_CHAR;
	pgInFilterSend(&trig);
      }
      else {
	/* Release */
	trig.content.type = PG_TRIGGER_KEYDOWN;
	pgInFilterSend(&trig);
      }
      break;
      
    case DIAL_EVENT:
      if (ntohs(evt.dial.state) == KEY_PRESS)
	call_dial(current_call,evt.dial.key);
      break;
      
    }
}

void phone_check_ring_timeout(void) {
  if (current_call && current_call->status == CALL_INCOMING && 
      time(NULL) > last_ring + RING_TIMEOUT) {
    hide_call_info();
    set_call_status(current_call,CALL_COMPLETED);

    /* FIXME: indicate that there was an unanswered call, activate a message
     *        machine, etc. What about the possibility that another phone
     *        answered the call? How can that be checked?
     */
  }
}
