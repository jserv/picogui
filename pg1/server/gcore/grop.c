/* $Id$
 *
 * grop.c - grop-list management
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
#include <pgserver/appmgr.h>
#include <pgserver/render.h>
#include <pgserver/divtree.h>

/******************* Zombie gropnode management */

/* This is a linked list of 'undead' gropnodes. When a gropnode is deleted,
 * it goes here. When a new node is needed, the below functions
 * look here first. The zombies get killed off if this list goes above
 * the limit CONFIG_MAX_ZOMBIEGROPS
 */
struct gropnode *grop_zombie_list;   
s32 grop_zombie_count;

g_error gropnode_alloc(struct gropnode **n) {
#ifdef DEBUG_KEYS
   num_grops++;
#endif
#ifdef DEBUG_MEMORY 
  fprintf(stderr, "gropnode_alloc : ");
#endif

   if (grop_zombie_list) {
#ifdef DEBUG_MEMORY
   fprintf(stderr, "using zombie list\n");
#endif

      /* re-use a zombie grop */
      grop_zombie_count--;
      *n = grop_zombie_list;
      grop_zombie_list = (*n)->next;
      return success;
   }
      
#ifdef DEBUG_MEMORY
   fprintf(stderr, "allocating memory\n");
#endif
  return g_malloc((void**)n,sizeof(struct gropnode));
};

void gropnode_free(struct gropnode *n) {
#ifdef DEBUG_KEYS
   num_grops--;
#endif
#ifdef DEBUG_MEMORY 
  fprintf(stderr, "gropnode_free(%p) : ",n);
#endif
      
   /* Can we just stick it in the zombie list? */
   if (grop_zombie_count < CONFIG_MAX_ZOMBIEGROPS) {
#ifdef DEBUG_MEMORY
   fprintf(stderr, "adding to zombie list\n");
#endif
   
      n->next = grop_zombie_list;
      grop_zombie_list = n;
      grop_zombie_count++;
      return;
   }
    
#ifdef DEBUG_MEMORY
   fprintf(stderr, "freeing memory\n");
#endif
   
   /* Do it the old-fasioned way */
   g_free(n);
}

/* For cleanup time, delete all gropnodes in the zombie list */
void grop_kill_zombies(void) {
   struct gropnode *p, *condemn;
   
   if (!grop_zombie_list) return;
   p = grop_zombie_list;
   while (p) {
      condemn = p;
      p = p->next;
#ifdef DEBUG_KEYS
   num_grops--;
#endif
      g_free(condemn);
   }
   grop_zombie_list = NULL;
   grop_zombie_count = 0;
}

/******************* Gropnode utilities */

/* Add a new gropnode to the context. Caller fills in
   all the grop's parameters afterwards. */
g_error addgrop(struct gropctxt *ctx, u16 type) {
  struct gropnode *node;
  g_error e;

  /* ctx == NULL is legal, used to disable actual output */
  if (!ctx) return success;

  e = gropnode_alloc(&node);
  errorcheck;
  node->type = type;
  node->next = NULL;
  node->flags = ctx->defaultgropflags;

  if (!ctx->current)
    *ctx->headpp = ctx->current = node;
  else {
    ctx->current->next = node;
    ctx->current = node;
  }

  ctx->n++;   /* There goes another gropnode! */

  return success;
}

g_error addgropsz(struct gropctxt *ctx, u16 type,s16 x,s16 y,s16 w,s16 h) {
   g_error e;
   e = addgrop(ctx,type);
   ctx->current->r.x = x;
   ctx->current->r.y = y;
   ctx->current->r.w = w;
   ctx->current->r.h = h;
   return success;
}

/* Delete the whole list */
void grop_free(struct gropnode **headpp) {
  struct gropnode *p,*condemn;

#ifdef DEBUG_MEMORY
  static int lock = 0;
  fprintf(stderr, "-> grop_free(%p = &%p)\n",headpp,*headpp);
  if (lock)
    fprintf(stderr, "     -- grop_free lock triggered --\n");
  lock = 1;
#endif

  if ((!headpp)) return;
  p = *headpp;
  while (p) {
    condemn = p;
    p = p->next;
    gropnode_free(condemn);
  }
  *headpp = NULL;

#ifdef DEBUG_MEMORY
  fprintf(stderr, "<- grop_free()\n");
  lock = 0;
#endif
}

/* Set up a grop context for rendering to a divnode */
void gropctxt_init(struct gropctxt *ctx, struct divnode *div) {
  struct gropnode *p;

  memset(ctx,0,sizeof(struct gropctxt));
  ctx->headpp = &div->grop;
  /* set current to the last gropnode */
  p=*ctx->headpp;
  if(p)
   {
    while(p->next)
      p=p->next;
    ctx->current=p;
   }
  ctx->r.w = div->r.w;
  ctx->r.h = div->r.h;
  ctx->owner = div;
}

/* The End */

