/* linkedList.h -  linked list header
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

typedef char pgoNodeIdentifier[64];

typedef struct _pgoLinkedListNode{
  pgoNodeIdentifier identifier;
  void *data;
  int dataSize;
  void *nextNode;
} pgoLinkedListNode;

pgoLinkedListNode *pgoLLInit(void);

int pgoLLListLength(pgoLinkedListNode *topNode);

int pgoLLAddRecord(pgoLinkedListNode *topNode,
			pgoNodeIdentifier identifier,
			void *data,
			int  dataSize);

void *pgoLLGetRecord(pgoLinkedListNode *topNode,
		     pgoNodeIdentifier identifier,
		     int *dataSize);

int pgoLLDeleteRecord(pgoLinkedListNode *topNode,
			   pgoNodeIdentifier identifier);

pgoNodeIdentifier *pgoLLListIdentifiers(pgoLinkedListNode *topNode, 
					int *nodeCount);

