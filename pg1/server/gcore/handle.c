/* $Id$
 *
 * handle.c - Handles for managing memory. Provides a way to refer to an
 *            object such that a client can't mess up our memory
 *            Implemented with a red-black tree
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

#include <string.h>

#include <pgserver/common.h>

#include <pgserver/handle.h>
#include <pgserver/widget.h>
#include <pgserver/pgnet.h>
#include <pgserver/input.h>	/* unload_inlib */
#include <pgserver/svrwt.h>
#include <pgserver/paragraph.h>

#define NIL ((struct handlenode *)(&sentinel))
struct handlenode const sentinel = {0,0,0,0,0,0,NULL,NIL,NIL,NIL};

struct handlenode *htree = NIL;

/* Handle mapping, for translating handles with the high bit set */
handle *handle_mapping;
int handle_mapping_len;

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
    z->context = y->context;
    z->payload = y->payload;
    z->group = y->group;
    z->type &= HFLAG_RED;
    z->type |= y->type & ~HFLAG_RED;

#ifdef DEBUG_MEMORY
     printf("handlenode delete SPECIAL CASE: handle = 0x%08X,"
	    "nodes %p -> %p\n",z->id,y,z);
#endif
  }  

  if ((!(y->type & HFLAG_RED)) && x!=NIL)
    htree_deletefix(x);

  g_free(y);
}

/* Convert a handle that may be in a local handle space to the
 * canonical global representation.
 */
handle handle_canonicalize(handle h) {
  /* Mapping table lookup */
  if ((h & 0x80000000) && ((h & 0x7FFFFFFF) < handle_mapping_len))
    h = handle_mapping[h & 0x7FFFFFFF];

  return h;
}


struct handlenode *htree_find(handle id) {
  struct handlenode *current = htree;

  id = handle_canonicalize(id);

  while(current != NIL)
    if(id == current->id)
      return (current);
    else
      current = (id < current->id) ?
	current->left : current->right;
  return NULL;
}

/* This function generates a number for a new handle.
 * It must return a number appropriate to the handle size, as defined
 * in handle.h, and _never_ return zero (null handle) or the largest possible
 * value (unknown but very annoying reason? always causes handle error)
 * 
 * There have been many ways to do this. At first, I used a method that seemed
 * like a good idea at the time, but under most circumstances didn't really
 * help.
 * 
 * The best way would be to find 'holes' in the handle tree and create handle
 * numbers specifically to fill in the tree without rebalancing.
 * Almost as good might be using pseudorandom numbers.
 * 
 * The current implementation just uses steadily increasing numbers that wrap
 * around, checking them for Bad Things before returning them.
 * Might improve this later, but at least the current implementation is less
 * error-prone than the last
 * 
 * FIXME: There is still something wrong with the handle system. When
 *        handles are repeatedly deleted and created, for example when
 *        calling pgReplaceText in an idle handler, a particular handle
 *        ID will eventually trigger a handle error.
 * 
 */
handle newhandle(void) {
  static handle h = 1;

  do {
     h++;
     if (h >= (HANDLE_SIZE-1))
       h = 1;
  } while (htree_find(h));

/*   printf("New handle 0x%08X\n",h);  */
   
  return h;
}

/* Free any object in a handlenode */
void object_free(struct handlenode *n) {
#ifdef DEBUG_KEYS
  num_handles--;
#endif
#ifdef DEBUG_MEMORY
  printf("Enter object free of handle 0x%08X, type %d\n",n->id,n->type &
	 PG_TYPEMASK);
#endif
  if (!(n->type & HFLAG_NFREE)) {
    switch (n->type & PG_TYPEMASK) {

    case PG_TYPE_BITMAP:
      VID(bitmap_free) ((hwrbitmap)n->obj);
      break;

    case PG_TYPE_WIDGET:
      widget_remove((struct widget *)n->obj);
      break;

    case PG_TYPE_THEME:
      theme_remove((struct pgmemtheme *)n->obj);
      break;

    case PG_TYPE_DRIVER:
      unload_inlib((struct inlib *)n->obj);
      break;

    case PG_TYPE_WT:
      wt_free((struct pgmemwt *)n->obj);
      break;

    case PG_TYPE_INFILTER:
      infilter_delete((struct infilter *)n->obj);
      break;

    case PG_TYPE_CURSOR:
      cursor_delete((struct cursor *)n->obj);
      break;

    case PG_TYPE_PGSTRING:
      pgstring_delete((struct pgstring *)n->obj);
      break;

    case PG_TYPE_FONTDESC:
      font_descriptor_destroy((struct font_descriptor *)n->obj);
      break;

      /* Object types that are memory-managed independently of their handles */
    case PG_TYPE_DIVTREE:
    case PG_TYPE_PARAGRAPH:
      break;

    default:
      g_free(n->obj);
    }
  }
#ifdef DEBUG_MEMORY
  printf("Leave object free of handle 0x%08X, type %d\n",n->id,n->type &
	 PG_TYPEMASK);
#endif
}

/************ Public functions */

#ifdef DEBUG_KEYS

/* Dump the handle tree to stdout */
void r_handle_dump(struct handlenode *n,int level) {
   int i;
   static char *typenames[] = {
     "BITMAP","WIDGET","FONTDESC","STRING","THEME","FILLSTYLE",
     "ARRAY", "DRIVER", "PALETTE", "WT", "INFILTER", "CURSOR"
   };
   
   if (!n) return;
   if (n==NIL) return;
   r_handle_dump(n->left,level+1);
   for (i=0;i<level;i++)
     printf(" ");
   printf("0x%04X : node %p obj %p grp 0x%04X pld 0x%08lX own %d ctx %d red %d type %s",
	  n->id,n,n->obj,n->group,(unsigned long)n->payload,n->owner,n->context,
	  n->type & PG_TYPEMASK,typenames[(n->type & PG_TYPEMASK)-1]);
   if ((n->type & PG_TYPEMASK) == PG_TYPE_PGSTRING) {
     printf(" = \"");
     pgstring_print((struct pgstring *)n->obj);
     printf("\"\n");
   }
   else if ((n->type & PG_TYPEMASK) == PG_TYPE_WIDGET)
     printf(" numcursors %d\n", ((struct widget *)n->obj)->numcursors);
   else
     printf("\n");
   r_handle_dump(n->right,level+1);
}
void handle_dump(void) {
   printf("---------------- Begin handle tree dump\n");
   r_handle_dump(htree,0);
   printf("---------------- End handle tree dump\n");
}


/* Dump strings to stdout */
void r_string_dump(struct handlenode *n) {
   if (!n) return;
   if (n==NIL) return;
   r_string_dump(n->left);
   if ((n->type & PG_TYPEMASK)==PG_TYPE_PGSTRING) {
     printf("0x%04X : ",n->id);
     pgstring_print((struct pgstring *)n->obj);
     printf("\n");
   }
   r_string_dump(n->right);
}
void string_dump(void) {
   printf("---------------- Begin string dump\n");
   r_string_dump(htree);
   printf("---------------- End string dump\n");
}
#endif

/* Allocates a new handle for obj */
g_error mkhandle(handle *h,unsigned char type,int owner,const void *obj) {
  struct handlenode *n;
  g_error e;
  int context = -1;
  struct conbuf *owner_conbuf;

  /* Find the owner's current context */
  if ((owner!=-1) && (owner_conbuf = find_conbuf(owner)))
    context = owner_conbuf->context;

  if (!h) return mkerror(PG_ERRT_INTERNAL,25);
  if (obj==NULL) {
    *h = 0;
    return success;
  }
#ifdef DEBUG_KEYS
  num_handles++;
#endif
  e = g_malloc((void **) &n,sizeof(struct handlenode));
  errorcheck;
  /* HAHAHA! Must initialize memory so 'group' doesn't have a random
   * value!  I think this was the source of MUCH trouble and TORMENT!!
   */
  memset(n,0,sizeof(struct handlenode));
  n->id = newhandle();
  n->type = type;
  n->context = context;
  n->obj = obj;
  n->owner = owner;
  htree_insert(n);
  *h = n->id;
   
#ifdef DEBUG_MEMORY
  printf("mkhandle(%d,0x%08X): node=%p\n",owner,*h,n);
#endif
   
  return success;
}

/* Group a handle with the theme containing it. Puts the 'to' handle in the
 * group of the theme 'from'.
 * 
 * Also sets the owner of 'to' to new_owner. The theme code uses this
 * to set the handle's owner to -1 so it can be read by anyone.
 * FIXME: We need real handle permissions
 */
g_error handle_group(int owner,handle from, handle to, int new_owner) {
  /* First, validate both handles */
  struct handlenode *f = htree_find(from);
  struct handlenode *t = htree_find(to);
  if (!(f && t && from && to)) return mkerror(PG_ERRT_HANDLE,92);
  if (owner>=0 && ((t->owner != owner) || (f->owner !=owner))) 
    return mkerror(PG_ERRT_HANDLE,27);
  t->group = f->id;
  t->owner = new_owner;
  return success;
}

g_error rdhandle(void **p,unsigned char reqtype,int owner,handle h) {
  void **x;
  g_error e;
  e = rdhandlep(&x,reqtype,owner,h);
#ifdef DEBUG_MEMORY
  if (iserror(e))
     printf("rdhandle error (0x%08X)\n",h);
#endif
  errorcheck;
  if (x)
     *p = *x;
   else
     *p = NULL;
  return success;
}
   
/* Reads the handle, returns NULL if handle is invalid or if it
   doesn't match the required type */
g_error rdhandlep(void ***p,unsigned char reqtype,int owner,handle h) {
  struct handlenode *n;
  if (!h) {
    *p = NULL;
    return success;
  }
  n = htree_find(h);
  if (!n) return mkerror(PG_ERRT_HANDLE,26);
  if ((n->type & PG_TYPEMASK) != reqtype) 
    return mkerror(PG_ERRT_HANDLE,28);

  /*
   * Allow the system to read any handle. Any application can read a
   * system handle. Applications can read their own handles, but not
   * other apps' handles.
   *
   */
  if (owner>=0 && n->owner>=0 && n->owner != owner) 
    return mkerror(PG_ERRT_HANDLE,27);

  *p = (void**)&n->obj;
  return success;
}

/* Deletes all handles owned by owner with a context == 'context',
   (all handles if owner is -1, all contexts if context is -1)
   Traverses seperately to find each handle because the tree could
   be rearranged by deletion
*/
int r_handle_cleanup(struct handlenode *n,int owner,int context,
		     handle group) {
  struct handlenode ncopy;

  if ((!n) || (n==NIL)) return 0;

  if ( ((owner<0) || (owner==n->owner)) && 
       ((context)<0 || (n->context==context)) &&
       ((!group) || group==n->group)) {

    /* Same sequence as handle_free()
       Remove from the handle tree BEFORE deleting the object itself */

#ifdef DEBUG_MEMORY
     printf("   handle cleanup 0x%08X: grp 0x%08X own %d ctx %d\n",
	    n->id,group,owner,context);
#endif
     
    ncopy = *n;
    htree_delete(n);
    object_free(&ncopy);

    /* See if this node had any group members that need to be expunged */
    while (r_handle_cleanup(htree,-1,-1,ncopy.id));

    return 1;
  }

  if (r_handle_cleanup(n->left,owner,context,group)) return 1;
  if (r_handle_cleanup(n->right,owner,context,group)) return 1;
  return 0;
}
void handle_cleanup(int owner,int context) {
#ifdef DEBUG_MEMORY
  printf("enter handle_cleanup(%d,%d)\n",owner,context);
#endif
  while (r_handle_cleanup(htree,owner,context,0));
#ifdef DEBUG_MEMORY
  printf("leave handle_cleanup(%d,%d)\n",owner,context);
#endif
}

/* Deletes the handle, and if HFLAG_NFREE is not set it frees the object */
g_error handle_free(int owner,handle h) {
  struct handlenode *n = htree_find(h);
  struct handlenode ncopy;

#ifdef DEBUG_MEMORY
  printf("handle_free(%d,0x%08X): node=%p\n",owner,h,n);
#endif

  /* Already gone - don't complain */
  if (!h) return success;
  if (!n) return success;

  if (owner>=0 && n->owner != owner) 
    return mkerror(PG_ERRT_HANDLE,27);

  /* Remove from the handle tree BEFORE deleting the object itself */
  ncopy = *n;
  htree_delete(n);
  object_free(&ncopy);

  /* See if this node had any group members that need to be expunged */
  while (r_handle_cleanup(htree,-1,-1,ncopy.id));

  return success;
}

/* A fairly interesting function.  Destroys any data referenced by
   the destination handle, and transfers the data from the source
   handle to the destination handle.  The destination's ownership is
   retained, and the source becomes invalid */
g_error handle_bequeath(handle dest, handle src, int srcowner) {
  /* First, validate both handles */
  struct handlenode *s = htree_find(src);
  struct handlenode *d = htree_find(dest);

#ifdef DEBUG_MEMORY
  printf("handle_bequeath(0x%08X,0x%08X,%d)\n",dest,src,srcowner);
#endif   

  if (!(src && s && dest && d)) return mkerror(PG_ERRT_HANDLE,26);
  if (srcowner>=0 && s->owner != srcowner) 
    return mkerror(PG_ERRT_HANDLE,27);
  if ((s->type & PG_TYPEMASK) !=
      (d->type & PG_TYPEMASK))
    return mkerror(PG_ERRT_HANDLE,28);

  object_free(d);
  d->obj = s->obj;
  htree_delete(s);
  return success;
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
g_error rehandle(handle h, void *obj, u8 type) {
  struct handlenode *hn = htree_find(h);
  if (!hn) return mkerror(PG_ERRT_HANDLE,26);
  hn->obj = obj;
  hn->type &= ~PG_TYPEMASK;
  hn->type |= type;
  return success;
}

/* Gets a pointer to the handle's payload, stores it in the variable
 * pointed to by pppayload
 */
g_error handle_payload(u32 **pppayload,int owner,handle h) {
  struct handlenode *n = htree_find(h);
  if (!n) return mkerror(PG_ERRT_HANDLE,26);
  if (owner>=0 && n->owner != owner) 
    return mkerror(PG_ERRT_HANDLE,27);
  *pppayload = &n->payload;
  return success;
}

/* Recursive part of handle_iterate() */
g_error r_iterate(struct handlenode *n,u8 type,handle_iterator iterator, void *extra) {
   g_error e;
   
   if ((!n) || (n==NIL)) return success;
   if ((n->type & PG_TYPEMASK)==type) {
     e = (*iterator)(&n->obj,extra);
     errorcheck;
   }
   e = r_iterate(n->left,type,iterator,extra);
   errorcheck;
   return r_iterate(n->right,type,iterator,extra);
}
g_error handle_iterate(u8 type,handle_iterator iterator, void *extra) {
   return r_iterate(htree,type,iterator,extra);
}

/*
 * Duplicate a handle and it's associated object.
 * (if it can be duplicated. Widgets, drivers,
 * and themes currently can't be duplicated)
 */
g_error handle_dup(handle *dest, int owner, handle src) {
  struct handlenode *n;
  void *newobj;
  g_error e;
  u32 sz;

  /* Find the handle */
  n = htree_find(src);
  /* make sure it exists */
  if (!n) return mkerror(PG_ERRT_HANDLE,26);
  /* Same permissions as rdhandle() */
  if (owner>=0 && n->owner>=0 && n->owner != owner) 
    return mkerror(PG_ERRT_HANDLE,27);
  
  /* Check the type */
  switch (n->type & PG_TYPEMASK) {

  case PG_TYPE_PGSTRING:
    e = pgstring_dup((struct pgstring **)&newobj, (struct pgstring *) n->obj);
    errorcheck;
    e = mkhandle(dest,n->type & PG_TYPEMASK,owner,newobj);
    errorcheck;
    break;

  case PG_TYPE_WT:
    e = wt_instantiate(dest, (struct pgmemwt *) n->obj, src, owner);
    errorcheck;
    break;

  case PG_TYPE_ARRAY:
  case PG_TYPE_PALETTE:
    e = g_malloc(&newobj, (((u32*)n->obj)[0] + 1) * sizeof(u32));
    errorcheck;
    e = mkhandle(dest,n->type & PG_TYPEMASK,owner,newobj);
    errorcheck;
    memcpy(newobj, n->obj, (((u32*)n->obj)[0] + 1) * sizeof(u32));
    break;

  default:
    return mkerror(PG_ERRT_BADPARAM,45);   /* Can't duplicate object */
  }
 
  return success;
}

/*
 * Change the context of an existing handle by 'delta'.
 * Can be positive or negative
 */
g_error handle_chcontext(handle h, int owner, s16 delta) {
  struct handlenode *n = htree_find(h);
  if (!n) return mkerror(PG_ERRT_HANDLE,26);
  if (owner>=0 && n->owner != owner) 
    return mkerror(PG_ERRT_HANDLE,27);
  n->context += delta;
  return success;
}

/*
 * Set an extra mapping table used for handles with the high bit
 * set (from 0x80000000 to 0xFFFFFFFF)
 */
void handle_setmapping(handle *table, int num_entries) {
  handle_mapping = table;
  handle_mapping_len = num_entries;
}

/* Look up the handle associated with the pointer, and delete it safely */
g_error pointer_free(int owner, void *ptr) {
  return handle_free(owner, hlookup(ptr,NULL));
}


/* The End */




