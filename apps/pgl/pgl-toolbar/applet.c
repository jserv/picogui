
#include <stdlib.h>
#include <picogui.h>

#include "applet.h"

struct pgmemdata pglBuildMessage(pglMessageType type, char *senderName, char *key, char *data){
  unsigned short messageSize = sizeof(pglMessage)+(strlen(senderName)+1)+(strlen(key)+1)+(strlen(data)+1);
  pglMessage *newMessage = malloc(messageSize);

  newMessage->messageType = type;
  newMessage->senderLen = strlen(senderName);
  newMessage->keyLen = strlen(key);
  newMessage->dataLen = strlen(data);
  newMessage->data = ((char *)newMessage) + sizeof(pglMessage);
  strcpy(&newMessage->data[0], senderName);
  strcpy(&newMessage->data[strlen(senderName)+1], key);
  strcpy(&newMessage->data[(strlen(senderName)+strlen(key))+2], data);
  return pgFromTempMemory(newMessage, messageSize);
}

pglMessage *alignMessageData(pglMessage *message){
 
  message->data = (char *)message + sizeof(pglMessage);
  return message;
}

char *pglGetMessageData(pglMessage *message, unsigned short offset){
  char *data;

  data = (char *)strdup(&message->data[offset]);

  return data;
}


