/* $Id: llist.h,v 1.2 2002/10/05 11:21:04 micahjd Exp $
 * 
 * llist.h - generic doubly-linked list. Note that this list includes
 * metadata which allow for fast lookup of start/end of list. API modeled
 * after glib's GLList.
 * 
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2002 Micah Dowty <micahjd@users.sourceforge.net>
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
 *   LList by Chuck Groom, cgroom@bluemug.com, Blue Mug, Inc. July
 *   2002. Used by edittext widget.
 */

#include <picogui/types.h>

#ifndef __LLIST_H__
#define __LLIST_H__

typedef struct _LList LList;
typedef void (*LListFunc)                 ( void * data,
                                            void * userdata );

LList *      llist_append                 ( LList * llist,
                                            void * data );
LList *      llist_prepend                ( LList * llist,
                                            void * data );
LList *      llist_insert_after           ( LList * llist,
                                            void * data );
LList *      llist_insert                 ( LList * llist,
                                            void * data,
                                            s16 position );
LList *      llist_remove                 ( LList * llist );
LList *      llist_remove_data            ( LList * llist,
                                            void * data );
LList *      llist_remove_link            ( LList * llist,
                                            LList ** llink );
void         llist_free                   ( LList * llist ); 
LList *      llist_alloc                  ( void );
u16          llist_length                 ( LList * llist );
LList *      llist_copy                   ( LList * llist );
LList *      llist_concat                 ( LList * llist1, 
                                            LList * llist2 );
void         llist_split                  ( LList * llist,
                                            LList **before,
                                            LList **after);
void         llist_foreach                ( LList * llist,
                                            LListFunc func, 
                                            void * user_data );
LList *      llist_first                  ( LList * llist );
LList *      llist_last                   ( LList * llist );
LList *      llist_next                   ( LList * llist );
LList *      llist_prev                   ( LList * llist );
LList *      llist_nth                    ( LList * llist,
                                            u16 n );
void *       llist_nth_data               ( LList * llist,
                                            u16 n );
u16          llist_index                  ( LList * llist );
void *       llist_data                   ( LList * llist );
LList *      llist_find                   ( LList * llist,
                                            void * data );

#endif /* __LLIST_H__ */
