/* $Id$
 * 
 * textedit_llist.c - generic doubly-linked list. Note that this list
 * includes metadata which allow for fast lookup of start/end of
 * list. API modeled after glib's GLList.
 *
 * NOTE: This is bundled with the textedit widget right now because
 *       it's the only part of pgserver using it. When other parts of
 *       pgserver start using this, it should be moved to gcore/llist.c
 *
 * This is an early version used by the edittext widget. API not used by
 * this widget has not been extensively tested, and may be buggy. 
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
 *   Chuck Groom, cgroom@bluemug.com, Blue Mug, Inc. July 2002. Used by
 *   edittext widget. 
 */

#include <pgserver/common.h>
#include <pgserver/g_malloc.h>
#include <pgserver/llist.h>
#include <assert.h>

typedef struct _LListMeta LListMeta;

struct _LList {
    LListMeta * meta;
    LList * next, * prev;
    void * data;
};


struct _LListMeta {
    LList * head, * tail;
    u16 len;
};


/* Append a new node containing 'data' to the list 'list', set 'new'
   to the start of the resultant list. */
g_error llist_append ( LList ** new,
                       LList * list,
                       void * data ) {
    g_error e;    
    assert(data);
    if (list == NULL) {
        e = llist_alloc(&list);
        errorcheck;
    }
    
    if (list->meta->len == 0) {
        /* LList consists of one empty node */
        list->data = data;
    } else {
        e = g_malloc((void **) new, sizeof(LList));
        errorcheck;

        list = llist_last(list);
        (*new)->meta = list->meta;
        (*new)->data = data;
        (*new)->next = NULL;
        (*new)->prev = list;
        list->next = *new;
        list->meta->tail = *new;
    }
    list->meta->len++;
    *new = llist_first(list);
    return success;
}


g_error llist_prepend ( LList ** new,
                        LList * list,
                        void * data ) {
    g_error e;

    if (list == NULL) {
        e = llist_alloc(&list);
        errorcheck;
    }    

    if (list->meta->len == 0) {
        /* A list is empty if list->data is NULL. */ 
        list->data = data;
    } else {
        e = g_malloc((void **) new, sizeof(LList));
        errorcheck;

        list = llist_first(list);
        (*new)->meta = list->meta;
        (*new)->data = data;
        (*new)->next = list;
        (*new)->prev = NULL;
        list->prev = (*new);
        list->meta->head = (*new);
    } 
    list->meta->len++;
    *new = llist_first(list);
    return success;
}


/**
 * Insert an element into the list after the item currently in 'list'.
 * Returns the head of the resulting list.
 */
g_error llist_insert_after ( LList ** new,
                             LList * list, 
                             void * data ) {
    g_error e;

    assert(data);
    if ((!list) || (!list->data) || (!list->next)) 
        return llist_append(new, list, data);
    
    e = g_malloc((void **) new, sizeof(LList));
    errorcheck;

    (*new)->meta = list->meta;
    (*new)->data = data;
    (*new)->prev = list;
    (*new)->next = list->next;
    list->next->prev = *new;
    list->next = *new;
    list->meta->len++;

    *new = llist_first(list);
    return success;
}


/**
 * Insert an element into the list at the given position.  If this is
 * negative, or is larger than the number of elements in the list, the
 * new element is added on to the end of the list.  */
g_error llist_insert ( LList ** new,
                       LList * list, 
                       void * data, 
                       s16 position ) { 
    u16 count;
    LList * current = NULL;

    assert(list && data);
    
    if (position >= 0) {
        for (current = llist_first(current), count = 0; 
             current && (count < position); 
             count++, current = llist_next(current))
            ;
    }
    
    if (current) 
        return llist_insert_after(new, current, data);
    return llist_append(new, llist_last(list), data);
}


/* Remove an element containing data from the list, returns the head
 * of the list. */
void llist_remove_data ( LList ** new, 
                         LList * list,
                         void * data ) {
    LList * victim;
    
    if (victim = llist_find(list, data)) 
        llist_remove(new, victim);
    else 
        *new = llist_first(list);
}


/* Remove the link at list, set 'new' to the head of the resulting list. */
void llist_remove ( LList ** new,
                    LList * list ) {
    LListMeta * meta;

    assert(list);
    meta = list->meta;
    if (meta->len == 1) {
        g_free(list);
        g_free(meta);
        new = NULL;
        return;
    } else {
        if (list->prev)
            list->prev->next = list->next;
        if (list->next)
            list->next->prev = list->prev;       
        if (!list->prev)
            meta->head = list->next;
        if (!list->next)
            meta->tail = list->prev;
        g_free(list);
    }
    meta->len--;
    *new = meta->head;
}


/* Remove an element from the list, set it to a new one-element list.
 * Set 'new' to 'list' minus llink. */
g_error llist_remove_link ( LList ** new,
                            LList * list,
                            LList ** llink ) {
    g_error e;
    
    e = llist_alloc(llink);
    errorcheck;
    
    e = llist_append (llink, *llink, list->data);
    errorcheck;
    
    llist_remove(new, list);
    return success;
}


/* Frees an entire list. Does NOT free associated data */
void llist_free ( LList * list ) {
    LList * current, * victim;
    LListMeta * meta;
    if (!list)
        return;
    meta = list->meta;
    assert(meta);
    for (current = llist_first (list); current; ) {
        victim = current;
        current = llist_next(current);
        g_free(victim);
    }
    g_free(meta);
} 


/* Frees an entire list, calling g_free on associated data */
void llist_free_data ( LList * list ) {
    LList * current;
    assert(list);
    for (current = llist_first (list); current; current = llist_next(current)) 
        g_free(llist_data(current));
    llist_free(list);
}


g_error llist_alloc ( LList ** new ) {
    g_error e;
    LListMeta * meta;

    e = g_malloc((void **) &meta, sizeof(LListMeta));
    errorcheck;
   
    e = g_malloc((void **) new, sizeof(LList));
    errorcheck;
    
    (*new)->meta = meta;
    (*new)->meta->len = 0;
    (*new)->meta->head = (*new);
    (*new)->meta->tail = (*new);
    (*new)->data = NULL;
    (*new)->next = NULL;
    (*new)->prev = NULL;
    return success;
}


g_error llist_copy ( LList ** new, 
                     LList * list ) {
    g_error e;
    
    assert(list);
    e = llist_alloc(new);
    errorcheck;

    for (list = llist_first(list); list; list = llist_next(list)) {
        e = llist_append(new, *new, list->data);
        errorcheck;
    }
    return success;
}


/* Concatenate lists 1 and 2 (1 ahead of 2), altering both to return
 * the head of the new list */
void llist_concat ( LList ** new,
                    LList * list1,
                    LList * list2 ) {
    LList * current;
    LListMeta * m1, *m2;

    m1 = list1->meta;
    m2 = list2->meta;
    
    if (m1->len == 0) {
        llist_free(list1);
        new = &list2;
        return;
    } 
    if (m2->len == 0) {
        llist_free(list2);
        new = &list1;
        return;
    } 
    m1->len += m2->len;
    m1->tail->next = m2->head;
    m2->head->prev = m1->tail;
    
    for (current = m2->head; current; current = llist_next(current)) 
        current->meta = m1;
    m1->tail = m2->tail;
    g_free(m2);
    *new = llist_first(list1);
}


/* Break the list apart into two lists, 'before' (non-inclusive) and
 * 'after' (includes 'list' element, which is the new head of after).
 * 'before' is set to the start of the new list. */
g_error llist_split ( LList *  list,
                      LList ** before,
                      LList ** after ) {
    g_error e;
    LList * current;
    LListMeta * meta;
    int len;

    if ((list->meta->len == 0) || (list->prev == NULL)) {
        *before = NULL;
        *after = list;
        return success;
    }

    *before = llist_first(list);
    *after = list;

    e = g_malloc((void **) &meta, sizeof(LListMeta));
    errorcheck;

    /* Scan over elements in new list to change meta information and 
     * count elements. */
    for (len = 0, current = *before; 
         current != list; 
         current = llist_next(current), len++) {
        current->meta = meta;
    }
    meta->len = len;
    meta->head = (*before);
    meta->tail = list->prev;
    meta->tail->next = NULL;
    (*after)->meta->len -= len;
    (*after)->meta->head = list;
    (*after)->meta->head->prev = NULL;
    return success;
}


u16 llist_length ( LList * list ) {
    return list->meta->len;
}


void llist_foreach ( LList * list,
                     LListFunc func,
                     void * user_data ) {
    if (list == NULL)
        return;
    for (list = llist_first(list); list; list = llist_next(list)) {
        (func) (llist_data(list), user_data);
    }
}


LList * llist_first ( LList * list ) {
    if (list == NULL)
        return NULL;
    return list->meta->head;
}


LList * llist_last ( LList * list ) {
    if (list == NULL)
        return NULL;
    return list->meta->tail;
}


LList * llist_next ( LList * list ) {
    if (list == NULL)
        return NULL;
    return list->next;
}


LList * llist_prev ( LList * list ) {
    if (list == NULL)
        return NULL;
    return list->prev;
}


LList * llist_nth ( LList * list,
                  u16 n ) {
    u16 count;
    if (list == NULL)
        return NULL;
    for (list = llist_first(list), count = 0; 
         (count < n) && list;
         count++, list = llist_next(list)) 
        ;
    return list;
}


void * llist_nth_data ( LList * list,
                        u16 n ) {
    if (list == NULL)
        return NULL;
    return llist_data(llist_nth (list, n));
}


u16 llist_index ( LList * list ) {
    LList * start;
    u16 count;
    assert(list);
    for (count = 0, start = llist_first(list); 
         start; 
         count++, start = llist_next(start)) {
        if (start == list)
            return count;
    }
    assert(start);
}


void * llist_data ( LList * list ) {
    if (list)
        return list->data;
    else
        return NULL;
}


LList * llist_find ( LList * list,
                     void * data ) {
    if (list == NULL)
        return NULL;
    for (list = llist_first(list); list; list = llist_next(list)) {
        if (llist_data(list) == data)
            return list;
    }
    return NULL;
}
