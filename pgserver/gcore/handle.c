/* $Id: handle.c,v 1.9 2000/08/01 06:31:39 micahjd Exp $
 *
 * handle.c - Handles for managing memory. Provides a way to refer to an
 *            object such that a client can't mess up our memory
 *            Implemented with a red-black tree
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micah@homesoftware.com>
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

#include <g_malloc.h>
#include <handle.h>
#include <widget.h>

#define NIL &sentinel
struct handlenode sentinel = {0,0,0,NIL,NIL,0};

struct handlenode *htree = NIL;

/************ Internal functions */

/* Standard red-black tree implementation for the handle tree   :-O  */

void htree_rotleft(struct handlenode *x) {
  struct handlenode *y = x->right;
  
  /* establish x->right link */
  x->right = y->left;
  if (y->left != NIL) y->left->parent = x;
  
  /* establish y->parent link */
  if (y != NIL) y->parent = x->parent;
  if (x->parent) {
    if (x == x->parent->left)
      x->parent->left = y;
    else
      x->parent->right = y;
  } else {
    htree = y;
  }
  
  /* link x and y */
  y->left = x;
  if (x != NIL) x->parent = y;
}

void htree_rotright(struct handlenode *x) {
  struct handlenode *y = x->left;
  
  /* establish x->left link */
  x->left = y->right;
  if (y->right != NIL) y->right->parent = x;
  
  /* establish y->parent link */
  if (y != NIL) y->parent = x->parent;
  if (x->parent) {
    if (x == x->parent->right)
      x->parent->right = y;
    else
      x->parent->left = y;
  } else {
    htree = y;
  }
  
  /* link x and y */
  y->right = x;
  if (x != NIL) x->parent = y;
}

void htree_insertfix(struct handlenode *x) {
  /* Rebalance the tree */
  
  /* check Red-Black properties */
  while (x != htree && (x->parent->type & HFLAG_RED)) {
    /* we have a violation */
    if (x->parent == x->parent->parent->left) {
      struct handlenode *y = x->parent->parent->right;
      if (y->type & HFLAG_RED) {
	
	/* uncle is RED */
	x->parent->type &= ~HFLAG_RED;
	y->type &= ~HFLAG_RED;
	x->parent->parent->type |= HFLAG_RED;
	x = x->parent->parent;
      } else {
	
	/* uncle is BLACK */
	if (x == x->parent->right) {
	  /* make x a left child */
	  x = x->parent;
	  htree_rotleft(x);
	}
	
	/* recolor and rotate */
	x->parent->type &= ~HFLAG_RED;
	x->parent->parent->type |= HFLAG_RED;
	htree_rotright(x->parent->parent);
      }
    } else {
      
      /* mirror image of above code */
      struct handlenode *y = x->parent->parent->left;
      if (y->type & HFLAG_RED) {
	
	/* uncle is RED */
	x->parent->type &= ~HFLAG_RED;
	y->type &= ~HFLAG_RED;
	x->parent->parent->type |= HFLAG_RED;
	x = x->parent->parent;
      } else {
	
	/* uncle is BLACK */
	if (x == x->parent->left) {
	  x = x->parent;
	  htree_rotright(x);
	}
	x->parent->type &= ~HFLAG_RED;
	x->parent->parent->type |= HFLAG_RED;
	htree_rotleft(x->parent->parent);
      }
    }
  }
  htree->type &= ~HFLAG_RED;
}

void htree_insert(struct handlenode *x) {
  struct handlenode *current, *parent;
  
  /* find where node belongs */
  current = htree;
  parent = 0;
  while (current != NIL) {
    parent = current;
    current = (x->id < current->id) ?
      current->left : current->right;
  }
  
  x->parent = parent;
  x->left = NIL;
  x->right = NIL;
  x->type |= HFLAG_RED;
  
  /* insert node in tree */
  if(parent) {
    if(x->id < parent->id)
      parent->left = x;
    else
      parent->right = x;
  } else {
    htree = x;
  }

  htree_insertfix(x);
}

void htree_deletefix(struct handlenode *x) {
  while (x != htree && (!(x->type & HFLAG_RED))) {
    if (x == x->parent->left) {
      struct handlenode *w = x->parent->right;
      if (w==NIL) return;
      if (w->type & HFLAG_RED) {
	w->type &= ~HFLAG_RED;
	x->parent->type |= HFLAG_RED;
	htree_rotleft(x->parent);
	w = x->parent->right;
	if (w==NIL) return;
      }
      if ((!(w->left->type & HFLAG_RED)) && (!(w->right->type & HFLAG_RED))) {
	w->type |= HFLAG_RED;
	x = x->parent;
      } else {
	if (!(w->right->type & HFLAG_RED)) {
	  w->left->type &= ~HFLAG_RED;
	  w->type |= HFLAG_RED;
	  htree_rotright(w);
	  w = x->parent->right;
	  if (w==NIL) return;
	}
	w->type &= ~HFLAG_RED;
	w->type |= x->parent->type & HFLAG_RED;
	x->parent->type &= ~HFLAG_RED;
	w->right->type &= ~HFLAG_RED;
	htree_rotleft(x->parent);
	x = htree;
      }
    } else {
      struct handlenode *w = x->parent->left;
      if (w==NIL) return;
      if (w->type |= HFLAG_RED) {
	w->type &= ~HFLAG_RED;
	x->parent->type |= HFLAG_RED;
	htree_rotright(x->parent);
	w = x->parent->left;
	if (w==NIL) return;
      }
      if ((!(w->left->type & HFLAG_RED)) && (!(w->right->type & HFLAG_RED))) { 
	w->type |= HFLAG_RED;
	x = x->parent;
      } else {
	if (!(w->left->type & HFLAG_RED)) {
	  w->right->type &= ~HFLAG_RED;
	  w->type |= HFLAG_RED;
	  htree_rotleft(w);
	  w = x->parent->left;
	  if (w==NIL) return;
	}
	w->type &= ~HFLAG_RED;
	w->type |= x->parent->type & HFLAG_RED;
	x->parent->type &= ~HFLAG_RED;
	w->left->type &= ~HFLAG_RED;
	htree_rotright(x->parent);
	x = htree;
      }
    }
  }
  x->type &= ~HFLAG_RED;
}

void htree_delete(struct handlenode *z) {
  struct handlenode *x, *y;
  
  if (!z || z == NIL) return;
    
  if (z->left == NIL || z->right == NIL) {
    /* y has a NIL node as a child */
    y = z;
  } else {
    /* find tree successor with a NIL node as a child */
    y = z->right;
    while (y->left != NIL) y = y->left;
  }
  
  /* x is y's only child */
  if (y->left != NIL)
    x = y->left;
  else
    x = y->right;
  
  /* remove y from the parent chain */
  if (x != NIL)
    x->parent = y->parent;
  if (y->parent)
    if (y == y->parent->left)
      y->parent->left = x;
    else
      y->parent->right = x;
  else
    htree = x;
  
  if (y != z) {
    z->id = y->id;   /* y passes its identity on to z */
    z->owner = y->owner;
    z->obj = y->obj;
    z->type &= HFLAG_RED;
    z->type |= y->type & ~HFLAG_RED;
  }  

  if ((!(y->type & HFLAG_RED)) && x!=NIL)
    htree_deletefix(x);

  g_free(y);
}

struct handlenode *htree_find(handle id) {
  struct handlenode *current = htree;
  while(current != NIL)
    if(id == current->id)
      return (current);
    else
      current = (id < current->id) ?
	current->left : current->right;
  return NULL;
}

/* Returns a sequence of handle number ordered from left to right on the
   layers of a perfectly balanced binary tree.  This eliminates the need
   for balancing a tree when a new item is added (initially) 
   and gives a nicerly-balanced tree in general

   Might find a new way to do this later.  Handle numbers are completely
   arbitrary so i could analyze the tree to figure out what the easiest
   place to put a node would be...
 */
handle newhandle(void) {
  static int layer = 0;
  static handle remaining = 0;
  static handle skip;
  static handle value;
  handle ret;

  do {
    if (layer > HANDLE_BITS) {
      layer = remaining = 0;
    }
    
    if (!remaining) {
      remaining = 1 << layer; 
      skip = HANDLE_SIZE >> layer;
      value = HANDLE_SIZE >> ++layer;
    }    
    
    ret = value;
    remaining--;
    value += skip;

  } while (htree_find(ret));
  
  return ret;
}

/* Free any object in a handlenode */
void object_free(struct handlenode *n) {
  if (!(n->type & HFLAG_NFREE)) {
    switch (n->type & ~(HFLAG_RED|HFLAG_NFREE)) {
    case TYPE_BITMAP:
      hwrbit_free(n->obj);
      break;
    case TYPE_WIDGET:
      widget_remove(n->obj);
      break;
    default:
      g_free(n->obj);
    }
  }
}

/************ Public functions */

/* Allocates a new handle for obj */
g_error mkhandle(handle *h,unsigned char type,int owner,void *obj) {
  struct handlenode *n;
  g_error e;
  int context = -1;
  struct conbuf *owner_conbuf;

  /* Find the owner's current context */
  if ((owner!=-1) && (owner_conbuf = find_conbuf(owner)))
    context = owner_conbuf->context;

  if (!h) return mkerror(ERRT_BADPARAM,"mkhandle - NULL handle pointer");
  if (obj==NULL) {
    *h = 0;
    return sucess;
  }
  e = g_malloc((void **) &n,sizeof(struct handlenode));
  if (e.type != ERRT_NONE) return e;
  n->id = newhandle();
  n->type = type;
  n->context = context;
  n->obj = obj;
  n->owner = owner;
  htree_insert(n);
  *h = n->id;
  return sucess;
}

/* Reads the handle, returns NULL if handle is invalid or if it
   doesn't match the required type */
g_error rdhandle(void **p,unsigned char reqtype,int owner,handle h) {
  struct handlenode *n;
  if (!p) return mkerror(ERRT_HANDLE,"rdhandle - p == NULL");
  if (!h) {
    *p = NULL;
    return sucess;
  }
  n = htree_find(h);
  if (!n) return mkerror(ERRT_HANDLE,"rdhandle - nonexistant handle");
  if ((n->type & ~(HFLAG_RED|HFLAG_NFREE)) != reqtype) return mkerror(ERRT_HANDLE,
						       "rdhandle - incorrect type");
  if (owner>=0 && n->owner != owner) 
    return mkerror(ERRT_HANDLE,"rdhandle - permission denied");;
  *p = n->obj;
  return sucess;
}

/* Deletes the handle, and if HFLAG_NFREE is not set it frees the object */
g_error handle_free(int owner,handle h) {
  struct handlenode *n = htree_find(h);
  struct handlenode ncopy = *n;

#ifdef DEBUG
  printf("handle_free(%d,0x%08X): node=0x%08X\n",owner,h,n);
#endif

  if (!h) return mkerror(ERRT_HANDLE,"handle_free - null handle");
  if (!n) return mkerror(ERRT_HANDLE,"handle_free - invalid handle");
  if (owner>=0 && n->owner != owner) 
    return mkerror(ERRT_HANDLE,"handle_free - permission denied");
  htree_delete(n);    /* Remove from the handle tree BEFORE deleting the object itself */
  object_free(&ncopy);

  return sucess;
}

/* Deletes all handles owned by owner with a context >= 'context',
   (all handles if owner is -1)
   Traverses seperately to find each handle because the tree could
   be rearranged by deletion
*/
int r_handle_cleanup(struct handlenode *n,int owner,int context) {
  if ((!n) || (n==NIL)) return 0;
  if (((owner<0) || (owner==n->owner)) && (n->context>=context)) {
    object_free(n);
    htree_delete(n);
    return 1;
  }
  if (r_handle_cleanup(n->left,owner,context)) return 1;
  if (r_handle_cleanup(n->right,owner,context)) return 1;
  return 0;
}
void handle_cleanup(int owner,int context) {
  while (r_handle_cleanup(htree,owner,context));
}

/* A fairly interesting function.  Destroys any data referenced by
   the destination handle, and transfers the data from the source
   handle to the destination handle.  The destination's ownership is
   retained, and the source becomes invalid */
g_error handle_bequeath(handle dest, handle src, int srcowner) {
  /* First, validate both handles */
  struct handlenode *s = htree_find(src);
  struct handlenode *d = htree_find(dest);
  if (!src) return mkerror(ERRT_HANDLE,
			   "handle_bequeath - null source handle");
  if (!s) return mkerror(ERRT_HANDLE,
			 "handle_bequeath - invalid source handle");
  if (!dest) return mkerror(ERRT_HANDLE,
			    "handle_bequeath - null dest handle");
  if (!d) return mkerror(ERRT_HANDLE,
			 "handle_bequeath - invalid dest handle");
  if (srcowner>=0 && s->owner != srcowner) 
    return mkerror(ERRT_HANDLE,"handle_bequeath - permission denied");
  if ((s->type & ~(HFLAG_RED|HFLAG_NFREE)) !=
      (d->type & ~(HFLAG_RED|HFLAG_NFREE)))
    return mkerror(ERRT_HANDLE,"handle_bequeath - incompatible handle types");

  object_free(d);
  d->obj = s->obj;
  htree_delete(s);
  return sucess;
}

/* This uses a recursive function to do its acual work */
handle r_hlookup(struct handlenode *n,void *obj,int *owner) {
  static handle h;

  if ((!n) || (n==NIL)) return 0;
  if (obj==n->obj) {
    if (owner) *owner = n->owner;
    return n->id;
  }
  if ((h = r_hlookup(n->left,obj,owner)) || 
      (h = r_hlookup(n->right,obj,owner)))
    return h;
  return 0;
}
handle hlookup(void *obj,int *owner) {
  return r_hlookup(htree,obj,owner);
}

/* Changes the object pointer of a handle */
g_error rehandle(handle h, void *obj) {
  struct handlenode *hn = htree_find(h);
  if (!hn) return mkerror(ERRT_HANDLE,"rehandle() - invalid handle");
  hn->obj = obj;
  return sucess;
}


/* The End */




