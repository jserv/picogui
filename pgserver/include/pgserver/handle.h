/* $Id: handle.h,v 1.6 2000/12/31 16:52:32 micahjd Exp $
 *
 * handle.h - Functions and data structures for allocating handles to
 *            represent objects, converting between handles and pointers,
 *            and deleting handles.
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micahjd@users.sourceforge.net>
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

#ifndef __HANDLE_H
#define __HANDLE_H

#include <pgserver/g_error.h>

/* Flags for the handle's type */
#define HFLAG_RED   0x80     /* This is a red-black tree */
#define HFLAG_NFREE 0x40     /* The pointer should _not_ be free'd when the
			        handle is destroyed (free'd in a method
			        depending on the data type */

/* Data type of a handle ID 
 * This is server-specific handle stuff,
 * the clients store handles in a long for
 * compatibility's sake.
 */
typedef unsigned short handle;
#define HANDLE_BITS     16
#define HANDLE_SIZE (1<<HANDLE_BITS)

struct handlenode {
  handle id;
   
  short int owner;        /* the connection that owns this handle */
  unsigned char type;     /* Most of this represents the data type
			   * that this handle points to. Upper 2 bits
			   * are for HFLAGs */
  signed char context;    /* Would usually be a short- trying char to
			   * make this structure pack better */

  /* 32-bit fields */
  unsigned long int payload;   /* Client-definable data */
  void *obj;
  struct handlenode *group;  /* Parent of this handle group */
  struct handlenode *left,*right,*parent;  /* For the red-black tree */
};

/* Allocates a new handle for obj 
 * Owner = -1, system owns it
 */
g_error mkhandle(handle *h,unsigned char type,int owner,void *obj);

/* Reads the handle, returns NULL if handle is invalid or if it
   doesn't match the required type and user. If owner is -1, we don't care */
g_error rdhandle(void **p,unsigned char reqtype,int owner,handle h);

/* Gets a pointer to the handle's payload, stores it in the variable
 * pointed to by pppayload
 */
g_error handle_payload(unsigned long **pppayload,int owner,handle h);

/* Given a pointer to an object, returns its handle.  Returns 0 if
   there is no matching handle. Places the owner in 'owner' if not null.
*/
handle hlookup(void *obj,int *owner);

/* Deletes the handle, and if HFLAG_NFREE is not set it frees the object.
 * Owner = -1, don't care
 */
g_error handle_free(int owner,handle h);

/* Deletes all handles from a specified owner (-1 for all handles) 
  that are in a context greater than or equal to 'context'
*/
void handle_cleanup(int owner,int context);

/* A fairly interesting function.  Destroys any data referenced by
   the destination handle, and transfers the data from the source
   handle to the destination handle.  The destination's ownership is
   retained, and the source becomes invalid */
g_error handle_bequeath(handle dest, handle src, int srcowner);

/* Changes the object pointer of a handle */
g_error rehandle(handle h, void *obj);

/* Add handle to another handle's group so they are freed at the same time */
g_error handle_group(int owner,handle from, handle to);

/* Call the resize() function on all widgets with handles */
void resizeall(void);

#endif /* __HANDLE_H */
/* The End */
