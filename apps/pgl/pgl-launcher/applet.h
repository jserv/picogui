/* 
 * applet.h -  PGL applet utility functions
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Daniel Jackson <carpman@voidptr.org>
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

#define PGL_STOREPREF 0
#define PGL_GETPREF 1
#define PGL_LOADPREFS 2
#define PGL_APPLETINSTALLED 3

typedef struct{
  unsigned short messageType;
  unsigned short senderLen, keyLen, dataLen;
  char data[0];
} pglMessage;

struct pgmemdata pglBuildMessage(unsigned short type, char *senderName, char *key, char *data);

pglMessage *pglDecodeMessage(pglMessage *message);

char *pglGetMessageData(pglMessage *message, unsigned short offset);
