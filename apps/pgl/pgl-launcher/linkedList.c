/* linkedList.c -  linked list
 *
 * pgOrganizer modular organizer for picoGUI
 * Copyright (C) 2001 Daniel Jackson <carpman@voidptr.org>
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
 * 
 * 
 * 
 */

#include <stdlib.h>
#include <string.h>

#include "linkedList.h"

pgoLinkedListNode *pgoLLInit(void){
  pgoLinkedListNode *thisNewTopNode = malloc(sizeof(pgoLinkedListNode));
  
  thisNewTopNode->nextNode = NULL;
  
  return thisNewTopNode;
}

int pgoLLListLength(pgoLinkedListNode *topNode){
  pgoLinkedListNode *currentNode = topNode;
  int nodeCount = 0;;

  while(currentNode->nextNode != NULL){
    nodeCount++;
    currentNode = currentNode->nextNode;
  }

  return nodeCount;
}

int pgoLLAddRecord(pgoLinkedListNode *topNode,
		   char identifier[64],
		   void *data,
		   int  dataSize)
{
  pgoLinkedListNode *currentNode = topNode;
  pgoLinkedListNode *newEndNode = malloc(sizeof(pgoLinkedListNode));

  newEndNode->nextNode = NULL;

  while(currentNode->nextNode != NULL){
    currentNode = currentNode->nextNode;
  }

  currentNode->nextNode = newEndNode;
  strcpy(currentNode->identifier, identifier);
  currentNode->data = data;
  currentNode->dataSize = dataSize;

  return 1;
}

void *pgoLLGetRecord(pgoLinkedListNode *topNode,
		     char identifier[64],
		     int  *dataSize)
{
  pgoLinkedListNode *currentNode = topNode;

  while((currentNode->nextNode != NULL) && 
	strcmp(identifier, currentNode->identifier)){
    currentNode = currentNode->nextNode;
  }

  if(currentNode->nextNode == NULL){
    return NULL;
  }else{
    printf("Got: %s\n", currentNode->identifier);
    if(dataSize)
      *dataSize = currentNode->dataSize;
    return currentNode->data;
  }
  return NULL;
}

int pgoLLDeleteRecord(pgoLinkedListNode *topNode,
		      char identifier[64])
{

  pgoLinkedListNode *currentNode = topNode;
  pgoLinkedListNode *prevNode = NULL;
  pgoLinkedListNode *newNext = NULL;

  while((currentNode->nextNode != NULL) &&
	(strcmp(currentNode->identifier, identifier))){
    prevNode = currentNode;
    currentNode = currentNode->nextNode;
  }
  
  if(prevNode == NULL && currentNode != NULL){
    newNext = currentNode->nextNode;
    free(currentNode->data);
    free(currentNode->identifier);
    free(currentNode);
    topNode = newNext;
    return 1;
  }else if(currentNode != NULL){
    newNext = currentNode->nextNode;
    free(currentNode->data);
    free(currentNode->identifier);
    free(currentNode);
    prevNode->nextNode = newNext;
    return 1;
  }else{
    return NULL;
  }
  return 1;
}

pgoNodeIdentifier *pgoLLListIdentifiers(pgoLinkedListNode *topNode, 
					int *nodeCount)
{
  pgoLinkedListNode *currentNode = topNode; 
  pgoNodeIdentifier *identifiers;
  int copyLoop = 0;

  if(nodeCount)
    *nodeCount = pgoLLListLength(topNode);

  identifiers = malloc(64*(pgoLLListLength(topNode)));
  
  while(currentNode->nextNode != NULL){
    strcpy(identifiers[copyLoop], currentNode->identifier);
    currentNode = currentNode->nextNode;
    copyLoop++;
  }

  return identifiers;
}

  
  
