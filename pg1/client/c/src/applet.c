/* 
 * applet.c -  PGL applet utility functions
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

#include "clientlib.h"

struct pgmemdata pglBuildMessage(unsigned short type, char *senderName, char *key, char *data){
  unsigned short messageSize = sizeof(pglMessage)+(strlen(senderName)+1)+(strlen(key)+1)+(strlen(data)+1);
  pglMessage *newMessage = malloc(messageSize);

  newMessage->messageType = htons(type);
  newMessage->senderLen = htons(strlen(senderName));
  newMessage->keyLen = htons(strlen(key));
  newMessage->dataLen = htons(strlen(data));
  strcpy(&newMessage->data[0], senderName);
  strcpy(&newMessage->data[strlen(senderName)+1], key);
  strcpy(&newMessage->data[(strlen(senderName)+strlen(key))+2], data);
  return pgFromTempMemory(newMessage, messageSize);
}

pglMessage *pglDecodeMessage(pglMessage *message){
  
  message->messageType = ntohs(message->messageType);
  message->senderLen = ntohs(message->senderLen);
  message->keyLen = ntohs(message->keyLen);
  message->dataLen = ntohs(message->dataLen);

  return message;
}

char *pglGetMessageData(pglMessage *message, unsigned short offset){

  return &message->data[offset];
}


