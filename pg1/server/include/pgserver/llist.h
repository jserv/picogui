/* $Id$
 * 
 * llist.h - generic doubly-linked list. Note that this list includes
 * metadata which allow for fast lookup of start/end of list. API modeled
 * after glib's GLList.
 * 
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 * 
 * Contributors: 
 *   LList by Chuck Groom, cgroom@bluemug.com, Blue Mug, Inc. July
 *   2002. Used by textedid widget.
 */

#include <picogui/types.h>

#ifndef __LLIST_H__
#define __LLIST_H__

typedef struct _LList LList;
typedef void (*LListFunc)                 ( void * data,
                                            void * userdata );

/* Actions which manipulate llists */
g_error      llist_append                 ( LList ** new,
                                            LList * list,
                                            void * data );
g_error      llist_prepend                ( LList ** new,
                                            LList * list,
                                            void * data );
g_error      llist_insert_after           ( LList ** new,
                                            LList * list,
                                            void * data );
g_error      llist_insert                 ( LList ** new, 
                                            LList * list,
                                            void * data,
                                            s16 position );
void         llist_remove                 ( LList ** new, 
                                            LList * list );
void         llist_remove_data            ( LList ** new, 
                                            LList * list,
                                            void * data );
g_error      llist_remove_link            ( LList ** new, 
                                            LList * list,
                                            LList ** llink );
g_error      llist_alloc                  ( LList ** new );
g_error      llist_copy                   ( LList ** new_list,
                                            LList * list );
void         llist_concat                 ( LList ** new,
                                            LList * llist1, 
                                            LList * llist2 );
g_error      llist_split                  ( LList * llist,
                                            LList **before,
                                            LList **after);
void         llist_free                   ( LList * llist );


/* Actions which get/set llist info */
void         llist_foreach                ( LList * llist,
                                            LListFunc func, 
                                            void * user_data );
u16          llist_length                 ( LList * llist );
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
