/* $Id$
 *
 * handle.h - Functions and data structures for allocating handles to
 *            represent objects, converting between handles and pointers,
 *            and deleting handles.
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
 * 
 * 
 * 
 */

#ifndef __HANDLE_H
#define __HANDLE_H

#include <pgserver/g_error.h>

/* Flags for the handle's type */
#define HFLAG_RED   0x80     /* This is a red-black tree */
#define HFLAG_NFREE 0x40     /* The pointer should _not_ be free'd when the
			        handle is destroyed (free'd in a method
			        depending on the data type */

/* Data type of a handle ID 
 *
 * We don't use the full 32 bits because then things like (1<<HANDLE_BITS)
 * would overflow. This still allows about a billion handles, so it
 * it shouln't be a limitation :)
 */
typedef u32 handle;
#define HANDLE_BITS     30
#define HANDLE_SIZE     (1<<HANDLE_BITS)

struct handlenode {
   /* 
    * WARNING!!!!!!
    * If you add/remove members from this structure, update the node copy
    * in htree_delete()! If you fail to do so, demons will haunt pgserver
    * with evil evil bugs
    *
    */
   
  handle id;
   
  handle group;           /* Parent of this handle group */
  short int owner;        /* the connection that owns this handle */
  u8 type;                /* Most of this represents the data type
			   * that this handle points to. Upper 2 bits
			   * are for HFLAGs */
  s8 context;

  /* 32-bit fields */
  u32 payload;   /* Client-definable data */
  const void *obj;
  struct handlenode *left,*right,*parent;  /* For the red-black tree */

  /* 
   * WARNING!!!!!!
   * If you add/remove members from this structure, update the node copy
   * in htree_delete()! If you fail to do so, demons will haunt pgserver
   * with evil evil bugs
   *
   */
};

/* Global objects */
extern handle res[PGRES_NUM];
g_error globals_init();

/* Find the handlenode associated with a handle ID */
struct handlenode *htree_find(handle id);

/* Allocates a new handle for obj 
 * Owner = -1, system owns it
 */
g_error mkhandle(handle *h,unsigned char type,int owner, const void *obj);

/* Reads the handle, returns NULL if handle is invalid or if it
   doesn't match the required type and user. If owner is -1, we don't care */
g_error rdhandle(void **p,unsigned char reqtype,int owner,handle h);

/* Like rdhandle, but returns a pointer to the handlenode's pointer */
g_error rdhandlep(void ***p,unsigned char reqtype,int owner,handle h);

/* Gets a pointer to the handle's payload, stores it in the variable
 * pointed to by pppayload
 */
g_error handle_payload(u32 **pppayload,int owner,handle h);

/* Given a pointer to an object, returns its handle.  Returns 0 if
   there is no matching handle. Places the owner in 'owner' if not null.
*/
handle hlookup(void *obj,int *owner);

/* Deletes the handle, and if HFLAG_NFREE is not set it frees the object.
 * Owner = -1, don't care
 */
g_error handle_free(int owner,handle h);

/* Look up the handle associated with the pointer, and delete it safely */
g_error pointer_free(int owner, void *ptr);

/* Deletes all handles from a specified owner (-1 for all handles) 
  that are in a context equal to 'context' (or -1 for all contexts)
*/
void handle_cleanup(int owner,int context);

/* A fairly interesting function.  Destroys any data referenced by
   the destination handle, and transfers the data from the source
   handle to the destination handle.  The destination's ownership is
   retained, and the source becomes invalid */
g_error handle_bequeath(handle dest, handle src, int srcowner);

/* Changes the object pointer of a handle */
g_error rehandle(handle h, void *obj, u8 type);

/* Group a handle with the theme containing it. Puts the 'to' handle in the
 * group of the theme 'from'.
 * 
 * Also sets the owner of 'to' to new_owner. The theme code uses this
 * to set the handle's owner to -1 so it can be read by anyone.
 * FIXME: We need real handle permissions
 */
g_error handle_group(int owner,handle from, handle to,int new_owner);

/* Call the specified function on all handles of the specified type,
 * with a pointer to the handle's object pointer.
 * This allows a particular transformation to be applied to objects
 * in bulk.
 */
typedef g_error (*handle_iterator)(const void **pobj,void *extra);
g_error handle_iterate(u8 type,handle_iterator iterator, void *extra);

/*
 * Duplicate a handle (if it can be duplicated. Widgets, drivers,
 * and themes currently can't be duplicated)
 */
g_error handle_dup(handle *dest, int owner, handle src);

/*
 * Change the context of an existing handle by 'delta'.
 * Can be positive or negative
 */
g_error handle_chcontext(handle h, int owner, s16 delta);

/*
 * Set an extra mapping table used for handles with the high bit
 * set (from 0x80000000 to 0xFFFFFFFF)
 */
void handle_setmapping(handle *table, int num_entries);

#ifdef DEBUG_KEYS
/* Debugging function to dump handle tree to stdout on CTRL-ALT-H */
void handle_dump(void);
/* Dump strings on CTRL-ALT-S */
void string_dump(void);
#endif

/* Convert a handle that may be in a local handle space to the
 * canonical global representation.
 */
handle handle_canonicalize(handle h);

#endif /* __HANDLE_H */
/* The End */
