/*
 *  Copyright (C) 1997, 1998 Olivetti & Oracle Research Laboratory
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

/*
 * vncviewer.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <X11/Xlib.h>
#include <X11/Xmd.h>
#include <picogui.h>

extern struct pgmodeinfo mi;

extern int endianTest;

#define Swap16IfLE(s) \
    (*(char *)&endianTest ? ((((s) & 0xff) << 8) | (((s) >> 8) & 0xff)) : (s))

#define Swap32IfLE(l) \
    (*(char *)&endianTest ? ((((l) & 0xff000000) >> 24) | \
			     (((l) & 0x00ff0000) >> 8)  | \
			     (((l) & 0x0000ff00) << 8)  | \
			     (((l) & 0x000000ff) << 24))  : (l))


/* args.c */

extern char *programName;
extern char hostname[];
extern Bool listenSpecified;
extern char *displayname;
extern Bool viewOnly;
extern char *geometry;
extern int wmDecorationWidth;
extern int wmDecorationHeight;
extern char *passwdFile;
extern int updateRequestPeriodms;
extern int updateRequestX;
extern int updateRequestY;
extern int updateRequestW;
extern int updateRequestH;
extern int rawDelay;
extern Bool debug;
extern Bool resurface;
extern Bool reconnect;

extern void processArgs(int argc, char **argv);
extern void usage();


/* rfbproto.c */

extern int rfbsock;
extern char *desktopName;
extern struct timeval updateRequestTime;
extern Bool sendUpdateRequest;

extern Bool ConnectToRFBServer(const char *hostname, int port);
extern Bool InitialiseRFBConnection();
extern Bool SetFormatAndEncodings();
extern Bool SendIncrementalFramebufferUpdateRequest();
extern Bool SendFramebufferUpdateRequest(int x, int y, int w, int h,
					 Bool incremental);
void HidePointer(void);
extern Bool SendPointerEvent(int x, int y, int buttonMask);
extern Bool SendKeyEvent(CARD32 key, Bool down);
extern Bool SendClientCutText(char *str, int len);
extern Bool HandleRFBServerMessage();


/* x.c */

enum edge_enum
{
  EDGE_EAST,
  EDGE_WEST,
  EDGE_NORTH,
  EDGE_SOUTH
};

extern enum edge_enum edge;
extern Display *dpy;
extern unsigned long BGR233ToPixel[];

extern Bool CreateXWindow();
extern void ShutdownX();
extern Bool HandleXEvents();
extern Bool AllXEventsPredicate(Display *dpy, XEvent *ev, char *arg);


/* sockets.c */

extern Bool errorMessageFromReadExact;

extern Bool ReadExact(int sock, char *buf, int n);
extern Bool WriteExact(int sock, char *buf, int n);
extern int ListenAtTcpPort(int port);
extern int ConnectToTcpAddr(unsigned int host, int port);
extern int AcceptTcpConnection(int listenSock);
extern int StringToIPAddr(const char *str, unsigned int *addr);
extern Bool SameMachine(int sock);


/* listen.c */

extern void listenForIncomingConnections();
