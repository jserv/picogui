/* $Id: picogui_client.c,v 1.1 2000/09/16 01:38:02 micahjd Exp $
 *
 * picogui.c - 
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 * Contributors: Philippe Ney <philippe.ney@smartdata.ch>
 * 
 * 
 * 
 */


/***************************** Inclusions *************************************/
#include "picogui.h"



/* Open a connection to the server, parsing PicoGUI commandline options
  if they are present
*/  
void pgInit(int argc, char **argv)
{
  int /*sockfd,*/ numbytes;
  struct pghello buf, ServerInfo;
  struct hostent *he;
  struct sockaddr_in server_addr; /* connector's address information */
  
  /* Initialise the global id */
  id = 0;

  if (argc != 2) {
    if ((he=gethostbyname(PG_REQUEST_SERVER)) == NULL) {  /* get the host info */
      herror("gethostbyname");
      exit(1);
    }
  }

  else {
    if((he=gethostbyname(argv[1])) == NULL) {  /* get the host info */
    herror("gethostbyname");
    exit(1);
    }
  }

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    exit(1);
  }

  server_addr.sin_family = AF_INET;                 /* host byte order */
  server_addr.sin_port = htons(PG_REQUEST_PORT);    /* short, network byte order */
  server_addr.sin_addr = *((struct in_addr *)he->h_addr);
  bzero(&(server_addr.sin_zero), 8);                /* zero the rest of the struct */

  if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
    perror("connect");
    exit(1);
  }

  if ((numbytes=recv(sockfd, &buf, sizeof(buf), 0)) == -1) {
    perror("recv");
    exit(1);
  }

/*  buf[numbytes] = '\0'; */
  ServerInfo.magic = ntohl(buf.magic);
  ServerInfo.protover = ntohs(buf.protover);
  ServerInfo.dummy = ntohs(buf.dummy);

  if(ServerInfo.magic != PG_REQUEST_MAGIC) {
    printf("PicoGUI - incorrect magic number (%i -> %i)\n",PG_REQUEST_MAGIC,ServerInfo.magic);
    exit(1);
    }
  if(ServerInfo.protover != PG_PROTOCOL_VER) {
    printf("PicoGUI - protocol version not supported\n");
    exit(1);
    }

  printf("Received:\n");
  printf("Magic: %i\n",ServerInfo.magic);
  printf("Protover: %i\n",ServerInfo.protover);
  printf("Dummy: %i\n",ServerInfo.dummy);

}



/* Flushes the buffer of packets - if there's only one, it gets sent as is
  More than one packet is wrapped in a batch packet
*/
long _flushpackets(const void *in_pgr,int pgr_len,
                   const void *in_data,int data_len,
		   struct pgreturn *in_pgret)
{
  int numbytes;
  char* msg = NULL;
  short msg_len,ret_code;


  if(send_info(sockfd,in_pgr,pgr_len)) exit(1);

  if(in_data != NULL) {
printf("In des DATA a sender: %i\n",data_len);
    if(send_info(sockfd,in_data,data_len)) exit(1);
  }

/* Read the response type */
  if ((numbytes=recv(sockfd, &ret_code, sizeof(ret_code), 0)) == -1) {
    printf("PicoGUI - Error reading response code");
    exit(1);
  }

  printf("Received:\n");
  printf("Numb: %i\n",ntohs(ret_code));

  
  if(ntohs(ret_code)==PG_RESPONSE_ERR) {
    /* Error */
    struct pgresponse_err pg_err;
    
    if((numbytes=recv(sockfd, &pg_err, sizeof(pg_err), 0)) == -1) {
      printf("PicoGUI - Error reading response code");
      exit(1);
    }
    msg_len = ntohs(pg_err.msglen);
    if(msg) free(msg);
    msg = malloc(msg_len);
    if((numbytes=recv(sockfd, msg, msg_len, 0)) == -1) {
      printf("PicoGUI - Error reading error string");
      exit(1);
    }
    if(ntohs(pg_err.id)!=id) {
      printf("PicoGUI - incorrect packet ID (%i -> %i)\n",ntohs(pg_err.id),id);
    }
    printf("PicoGUI - Server error of type %i: %s\n",ntohs(pg_err.errt),msg);
  }

  if(ntohs(ret_code)==PG_RESPONSE_RET) {
    /* Return code */
    struct pgresponse_ret pg_ret;

    if((numbytes=recv(sockfd,&pg_ret,sizeof(pg_ret), 0)) == -1) {
      printf("PicoGUI - Error reading return value");
      exit(1);
    }
    if(ntohs(pg_ret.id)!=id) {
      printf("PicoGUI - incorrect packet ID (%i -> %i)\n",ntohs(pg_ret.id),id);
    }
    
    pgret.l1 = (ntohl(pg_ret.data));
  }

  if(ntohs(ret_code)==PG_RESPONSE_EVENT) {
    struct pgresponse_event pg_ev;
    
    if((numbytes=recv(sockfd, &pg_ev, sizeof(pg_ev), 0)) == -1) {
      printf("PicoGUI - Error reading event");
      exit(1);
    }
    pgret.s1 = pg_ev.event;
    pgret.l1 = pg_ev.from;
    pgret.l2 = pg_ev.param;
  }

  if(ntohs(ret_code)==PG_RESPONSE_DATA) {
    struct pgresponse_data pg_data;
    
    if((numbytes=recv(sockfd, &pg_data, sizeof(pg_data), 0)) == -1) {
      printf("PicoGUI - Error reading return data header");
      exit(1);
    }
    if(ntohs(pg_data.id)!=id) {
      printf("PicoGUI - incorrect packet ID (%i -> %i)\n",ntohs(pg_data.id),id);
    }
    msg_len = ntohl(pg_data.size);
    if(msg) free(msg);
    msg = malloc(msg_len);
    if((numbytes=recv(sockfd, msg, msg_len, 0)) == -1) {
      printf("PicoGUI - Error reading return data");
      exit(1);
    }
    pgret.data = malloc(msg_len);
    pgret.data = msg;
  }
  
/*  printf("PicoGUI - Unexpected response type (%i)\n",ntohs(ret_code)); */

}

/* Like send, but with some error checking stuff.  Returns nonzero
   on error.
*/
int send_info(int to,const void *data,int len) {
printf("In send_info\n");

  if (send(to,data,len,0)!=len) {

    printf("Error in send()\n");     

    close(to);
    return 1;
  }
  return 0;
}



/********************* Functions for each type of request *********************/
void Update(void) {
  struct pgrequest pgr;

  id++;
  pgr.type = htons(1);
  pgr.id = htons(id);
  pgr.size = htonl(0);

/*  _flushpackets(&pgr,sizeof(pgr),NULL,0,&pgret);*/
}

void _wait(void) {
  struct pgrequest pgr;

  id++;
  pgr.type = htons(13);
  pgr.id = htons(id);
  pgr.size = htonl(0);
printf("In _wait\n");
printf("Size of pgr struc: %i\n",sizeof(pgr));
  _flushpackets(&pgr,sizeof(pgr),NULL,0,&pgret);
}

void _mkpopup(short in_x,short in_y,short in_w,short in_h) {
  struct pgrequest pgr;
  struct pgreqd_mkpopup pgmkpopup;

  id++;
  pgmkpopup.x = htons(in_x);
  pgmkpopup.y = htons(in_y);
  pgmkpopup.w = htons(in_w);
  pgmkpopup.h = htons(in_h);

  pgr.type = htons(16);
  pgr.id = htons(id);
  pgr.size = htonl(sizeof(pgmkpopup));
printf("In _mkpopup BEFORE _flushpackets\n");

  _flushpackets(&pgr,sizeof(pgr),&pgmkpopup,sizeof(pgmkpopup),&pgret);
}


void NewPopup(short in_x,short in_y,short in_w,short in_h) {
/*    my $self = {-root => 1};  */

/*    # If there were only two args, it was width and height. 
    # Just center it instead.
    if (scalar(@_)<4) {
	$w = $x;
	$h = $y;
	$x = $y = -1;
    }      Not availlable in C */

/*    bless $self;
    $self->{'h'} =   */

  _mkpopup(in_x,in_y,in_w,in_h);

  /* Default is inside this widget */
  default_rship = PG_DERIVE_INSIDE;
/*    $default_parent = $self->{'h'}; */

/*    return $self;  */
}


/* This is called after the app finishes it's initialization.
   It waits for events, and dispatches them to the functions they're
   bound to.
*/
void pgEventLoop(void) {
/*    my $fromobj = {};  */

printf("In eventloop BEFORE _wait\n");
  eventloop_on = 1;

  /* Good place for an update...  (probably the first update) */
  Update();

  while (eventloop_on) {
    _wait();
/*    ($event, $from, $param) = _wait();
	
    # Package the 'from' handle in an object
    bless $fromobj;
    $fromobj->{'h'} = $from;

    # Call the code reference
    $r = $bindings{$from.':'.$event};
    &$r($fromobj,$param) if (defined $r);
*/    }

}



/* The End */
