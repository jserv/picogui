/*
 *
 * list.c - Holder of pghandles.  Scrollable too.
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000,2001 Micah Dowty <micahjd@users.sourceforge.net>
 * Copyright (C) 2001 RidgeRun Inc.
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
 * Marcus Smith
 * 
 * 
 */

#include <pgserver/common.h>
#include <pgserver/widget.h>
#include <picogui/list.h>

#define DEBUG
#ifndef DEBUG
#define printf(x,...)
#endif

/*
 * Structure to record the pghandle and row position in this list
 */
typedef struct list {
   struct list *next;
   struct list *prev;
   pghandle handle;
   int position;
}listnode;

static listnode *list_locate_row(listnode *start, listnode *end, int pos, int numnodes);

/* This widget has extra data we can't store in the divnodes themselves */
struct listdata {
   int marginOverride;    // Difference between container and scrollbar height
   listnode *widgetlist;
   int widgetlistsize;
   int selectedPosition;
   int viewWindowTopY;
   int viewWindowBottomY;
};

#define DATA ((struct listdata *)(self->data))
#define MARGIN_OVERRIDE (DATA->marginOverride)

void list_resize(struct widget *self) {
   int m;
   
   if (!MARGIN_OVERRIDE) {

     /* Transparent boxen have the same margin as a button
      * (necessary for grids to look good) */
     if (self->in->div->build)
       m = theme_lookup(self->in->div->state,PGTH_P_MARGIN);
     else
       m = theme_lookup(PGTH_O_BUTTON,PGTH_P_SPACING) >> 1;
     
     self->in->div->split = m;
   }
}

g_error list_install(struct widget *self) {
  g_error e;

  e = g_malloc(&self->data,sizeof(struct listdata));
  errorcheck;
  memset(self->data,0,sizeof(struct listdata));
  MARGIN_OVERRIDE = 0;

  DATA->widgetlist = NULL;
  DATA->widgetlistsize = 0;
  DATA->selectedPosition = -1;
    
  e = newdiv(&self->in,self);
  errorcheck;
  self->in->flags |= PG_S_TOP;
  e = newdiv(&self->in->div,self);
  errorcheck;
  self->in->div->flags |= DIVNODE_SPLIT_BORDER;
  self->in->div->flags &= ~DIVNODE_SIZE_AUTOSPLIT;
  self->in->div->build = &build_bgfill_only;
  self->in->div->state = PGTH_O_BOX;

  self->out = &self->in->next;
  self->sub = &self->in->div->div;
  self->trigger_mask = TRIGGER_STREAM | TRIGGER_KEYUP | TRIGGER_KEYDOWN;
  
  return sucess;
}

void list_remove(struct widget *self) {

   listnode *current, *next;
   if ( DATA->widgetlist ) {

      current = DATA->widgetlist;
      next = DATA->widgetlist->next;
      while ( next != DATA->widgetlist ) {
         g_free(current);
         current = next;
         next = current->next;
      }
      g_free(current);
      
   }   
   g_free(DATA);
   
  if (!in_shutdown)
    r_divnode_free(self->in);
}

g_error list_set(struct widget *self,int property, glob data) {
   listnode *node;
   g_error e;
   struct widget *w, *ws;
   
  switch (property) {
    
  case PG_WP_TRANSPARENT:
    self->in->div->build = data ? NULL : (&build_bgfill_only);
    break;	
    
  case PG_WP_MARGIN:
    self->in->div->split = MARGIN_OVERRIDE;
    MARGIN_OVERRIDE = 1;       /* Prevent automatic setting of margins */
    break;
    
  case PG_WP_SELECTED:
     
     /*
      * Need to locate the row cooresponding to selectedRow and call widget_set(struct widget *, PG_WP_HILIGHTED, 1)
      */
     node = list_locate_row(DATA->widgetlist, DATA->widgetlist->prev, data, DATA->widgetlistsize);
     if ( node->position == data ) {
        printf(__FUNCTION__": PG_WP_SELECTED - found desired row => %d\n", data);
        e = rdhandle((void**) &w,PG_TYPE_WIDGET,-1,node->handle);
        errorcheck;

        //
        // Turn off previous selected row, if set
        //
        if ( DATA->selectedPosition >= 0 ) {
           node = list_locate_row(DATA->widgetlist, DATA->widgetlist->prev, DATA->selectedPosition, DATA->widgetlistsize);
           if ( !node ) {
              printf(__FUNCTION__": PG_WP_SELECTED - can't find prior selected row!\n");
              return mkerror(PG_ERRT_INTERNAL, 1);
           }

           e = rdhandle((void **) &ws, PG_TYPE_WIDGET, -1, node->handle);
           errorcheck;
           widget_set(ws, PG_WP_HILIGHTED, 0);
           DATA->selectedPosition = -1;
        }

        //
        // Turn on the desired row
        //
        widget_set(w, PG_WP_HILIGHTED, 1);
        DATA->selectedPosition = data;
     }
     else {
        printf(__FUNCTION__": PG_WP_SELECTED - Did not find desired row => %d\n", data);
     }
     
     break;
     
  default:
    return mkerror(ERRT_PASS,0);
  }
  return sucess;
}

glob list_get(struct widget *self,int property) {
  switch (property) {

  case PG_WP_MARGIN:
    return self->in->div->split;

  case PG_WP_SELECTED:
     return DATA->selectedPosition;

  case PG_WP_SELECTED_HANDLE:
  {
     listnode *node = list_locate_row(DATA->widgetlist, DATA->widgetlist->prev, DATA->selectedPosition, DATA->widgetlistsize);
     if ( node )
        return node->handle;
     else
        return mkerror(PG_ERRT_BADPARAM,102);
  }
  }
  return 0;
}

static void list_remove_row(struct widget *self, listnode *rnode, int pos) {
   g_error e;
   struct widget *w;
   listnode *elem = NULL;
   
   //
   // Do the positions match, if so remove the widget at rnode
   //
   if ( rnode->position == pos ) {

      printf(__FUNCTION__": Position match!\n");
      
      //
      // Are we removing the first element in the widget list
      //
      if ( rnode == DATA->widgetlist ) {

         printf(__FUNCTION__": Removing the first widget\n");
         elem = DATA->widgetlist;
         if ( DATA->widgetlistsize == 1 ) {
            printf(__FUNCTION__": Removing the only widget in the widgetlist\n");
            DATA->widgetlist = NULL;
         }
         else {
            printf(__FUNCTION__": Removing the first widget in a multi member widgetlist\n");
            DATA->widgetlist = elem->next;
            elem->prev->next = elem->next;
            DATA->widgetlist->prev = elem->prev;
         }
      }
      else {

         //
         // Removing something other than the rnode widget
         //
         printf(__FUNCTION__": Removing a widget in the middle somewhere\n");
         elem = rnode;
         elem->prev->next = elem->next;
         elem->next->prev = elem->prev;
      }

      //
      // Have the element to remove.  Remove it from the list, but don't deallocate it!
      // Just detach and deallocate the list storage.
      //
      --DATA->widgetlistsize;
      g_free(elem);

      //
      // If we've removed the selected row, record this fact in DATA
      //
      if ( pos == DATA->selectedPosition )
         DATA->selectedPosition = -1;
      
   }  // End of if position match
   else {
      printf(__FUNCTION__": start == end, but start->position != pos - not removing!\n");
   }
   
}  // End of list_remove_row

//
// list_locate_row()
// This function will locate a position in the list of rows.  A pointer to this position will be returned.
//
static listnode *list_locate_row(listnode *start, listnode *end, int pos, int numnodes) {
   g_error e;
   listnode *entry;
   struct widget *parent;
   
   //
   // Does start equal end?  If so, do the insertion here
   //
   if ( start == end ) {

      //
      // Return this listnode node
      //
      return start;
   }
   else {

      //
      // If start does not equal end, split the list and iterate again.
      //
      int midpoint = numnodes / 2 - 1;
      listnode *s, *e, *rowIterator = start;
      while ( midpoint > 0 && rowIterator != end ) {
         rowIterator = rowIterator->next;
         --midpoint;
      }
      
      //
      // Set the start and end row for the recursive call
      //
      if ( rowIterator->position == pos )
         return rowIterator;
      
      else if ( rowIterator->position < pos ) {
         s = (rowIterator == end) ? end : rowIterator->next;
         e = end;
         return list_locate_row(s, e, pos, numnodes / 2);
      }
      else {
         s = start;
         e = (rowIterator == start) ? start : rowIterator->prev;
         return list_locate_row(s, e, pos, numnodes / 2);
      }

   }

}  // End of list_locate_row

// 
// list_insert_row()
// This function is used to insert a widget row at the given location.
//
static int list_insert_row(struct widget *self, listnode *rnode, struct widget *w, int pos, pghandle handle, unsigned char preserve_colliding_positions) {

   g_error e;
   listnode *entry;
   struct widget *parent;

   //
   // Build the new node
   //
   e = g_malloc((void **)&entry,sizeof(listnode));
   printf(__FUNCTION__": just allocated 0x%x\n", entry);
   errorcheck;

   //
   // Initialize what we can of entry
   //
   entry->position = pos;
   entry->handle = handle;
      
   //
   // Three cases here.  Greater than, derive after the widget at rnode.
   // Less than, derive before the widget at rnode.
   // Equal to, add before the row at rnode, then blow away rnode.
   //
   if ( pos < rnode->position ) {
      
      printf(__FUNCTION__": pos < rnode->position %d, %d\n", pos, rnode->position);
        
      //
      // Is this entry the first entry in the list?  If so, doctor up the head and last->next
      //
      if ( rnode == DATA->widgetlist ) {
         DATA->widgetlist->prev->next = entry;
         DATA->widgetlist = entry;
      }

      //
      // Insert the new entry into the list
      //
      entry->prev = rnode->prev;
      entry->next = rnode;
      rnode->prev = entry;
      if ( rnode->next == rnode ) rnode->next = entry;
         
      //
      // Insertion in the list OK.  Now derive the widget
      //
      e = rdhandle((void**) &parent,PG_TYPE_WIDGET,w->owner,rnode->handle);
      widget_derive(&w, w->type, parent, rnode->handle, PG_DERIVE_BEFORE, w->owner);
      ++DATA->widgetlistsize;
   }

   //
   // Next case, pos greater than one at rnode
   //
   else if ( pos > rnode->position ) {

      printf(__FUNCTION__": pos > rnode->position %d, %d\n", pos, rnode->position);
         
      //
      // Is this entry the last entry in the list?  If so, doctor up the head->prev
      //
      if ( rnode->next == DATA->widgetlist ) {
         DATA->widgetlist->prev = entry;
      }

      //
      // Insert the new entry into the list
      //
      entry->next = rnode->next;
      rnode->next = entry;
      entry->prev = rnode;
      if ( rnode->prev == rnode ) rnode->prev = entry;
         
      //
      // Derive the widget.
      //
      e = rdhandle((void**) &parent,PG_TYPE_WIDGET,w->owner,rnode->handle);
      widget_derive(&w, w->type, parent, rnode->handle, PG_DERIVE_AFTER, w->owner);
      ++DATA->widgetlistsize;
         
   }  // End of pos > rnode->position

   //
   // Last case, pos is equal.  Insert new element, then delete old & remove it from the listnodes list
   //
   else {
         
      pghandle oldhandle;
      pghandle ph;

      if ( preserve_colliding_positions ) {
         listnode *prev, *cur;
            
         printf(__FUNCTION__": colliding positions - Insert colliding element\n");            

         //
         // Is this entry the last entry in the list?  If so, doctor up the head->prev
         //
         if ( rnode->next == DATA->widgetlist ) {
            DATA->widgetlist->prev = entry;
         }

         entry->next = rnode->next;
         rnode->next = entry;
         entry->prev = rnode;
         if ( rnode->prev == rnode ) rnode->prev = entry;
         
         //
         // Derive the widget.
         //
         e = rdhandle((void**) &parent,PG_TYPE_WIDGET,w->owner,rnode->handle);
         widget_derive(&w, w->type, parent, rnode->handle, PG_DERIVE_AFTER, w->owner);
         ++DATA->widgetlistsize;

         //
         // Adjust positions from rnode->>next thru the end of the list
         //
         for ( prev = rnode, cur = rnode->next;
               (cur != DATA->widgetlist) && ((cur->position - prev->position) == 0); 
               prev = cur, cur = cur->next ) {

            cur->position++;

         }
      }
      else {
         printf(__FUNCTION__": pos = rnode->position, replacing it %d, %d\n", pos, rnode->position);
         
         //
         // Copy over current widget entry
         //
         oldhandle = rnode->handle;
         rnode->handle = handle;

         //
         // Get the widgets struct widget pointer
         //
         e = rdhandle((void**) &parent,PG_TYPE_WIDGET,w->owner,oldhandle);
         widget_derive(&w, w->type, parent, oldhandle, PG_DERIVE_BEFORE, w->owner);
         widget_remove(parent);
         g_free(entry);
      }
   }

}  // End of list_insert_row()
   
static g_error list_command(struct widget *self, unsigned short command, 
		                   unsigned short numparams,signed long *params) {
   int i;
   g_error e;
   struct widget *w, *parent;
   handle ph;
   int owner;
   
   switch (command) {

   case PGLIST_INSERT_AT:
   {
      listnode *insertPosition;
      
      printf(__FUNCTION__": widget => %d, pos = %d\n", *params, *(params+1));
      e = rdhandle((void**) &w,PG_TYPE_WIDGET,-1,*params);
      errorcheck;
      printf(__FUNCTION__": widgetlistsize = %d\n", DATA->widgetlistsize);
      if ( DATA->widgetlistsize == 0 && !DATA->widgetlist ) {

         printf(__FUNCTION__": adding to start of widgetlist\n");
         e = g_malloc((void **)&DATA->widgetlist,sizeof(listnode));
         errorcheck;
         DATA->widgetlist->next = DATA->widgetlist;
         DATA->widgetlist->prev = DATA->widgetlist;
         DATA->widgetlist->handle = *params;
         DATA->widgetlist->position = params[1];
         ph = hlookup(self, &owner);
         widget_derive(&w, w->type,self, ph, PG_DERIVE_INSIDE, owner);
         ++DATA->widgetlistsize;
      }
      else {
         listnode *rnode = list_locate_row(DATA->widgetlist, DATA->widgetlist->prev, params[0], DATA->widgetlistsize);
         list_insert_row(self, rnode, w, params[1], params[0], 0);
      }
      break;
   }
      
   case PGLIST_INSERT_AT_PARENT:
   {
      listnode *iterator;
      
      printf(__FUNCTION__"PGLIST_INSERT_AT_PARENT: widget => %d, parent = %d, rship = %d\n", *params, *(params+1), *(params+2));

      //
      // There really is not any fluffy algorithm I can use to get the row position, given the parent.
      // Start a linear search for the widget.
      //
      if ( DATA->widgetlist ) {

         iterator = DATA->widgetlist;

         //
         // Check the first one
         //
         do {
            if ( iterator->handle == params[1] )
               break;

         } while ( iterator && (iterator = iterator->next) != DATA->widgetlist );

         //
         // Two cases here.  Either iterator->handle is the one we're looking for, or
         // iterator == DATA->widgetlist
         //
         if ( iterator->handle == params[1] ) {
            printf(__FUNCTION__": PGLIST_INSERT_AT_PARENT: iterator valid, derive = %d\n", params[2]);
            e = rdhandle((void**) &w,PG_TYPE_WIDGET,-1,*params);
            errorcheck;
         
            //
            // Support only two relationships, namely PG_DERIVE_AFTER and PG_DERIVE_BEFORE.
            //
            if ( params[2] == PG_DERIVE_AFTER ) {
               printf(__FUNCTION__": PGLIST_INSERT_AT_PARENT: rship = PG_DERIVE_AFTER\n");
               list_insert_row(self, iterator, w, iterator->position+1, params[0], 1);
               
            }
            else if ( params[2] == PG_DERIVE_BEFORE ) {
               printf(__FUNCTION__": PGLIST_INSERT_AT_PARENT: rship = PG_DERIVE_BEFORE\n");
               if ( (iterator->position - 1) < 0 ) {
                  printf(__FUNCTION__": PGLIST_INSERT_AT_PARENT: previous position from parent is < 0!\n");
               }
               else {
                  list_insert_row(self, iterator, w, iterator->position-1, params[0], 1);
               }
            }
            else {
               printf(__FUNCTION__": PGLIST_INSERT_AT_PARENT: invalid rship = %d\n", params[2]);
            }

         }  // End of found parent handle
         else {
            printf(__FUNCTION__": PGLIST_INESERT_AT_PARENT: Error - Can't find parent handle = %d\n", params[1]);
         }

      }
      else {
         printf(__FUNCTION__": PGLIST_INSERT_AT_PARENT: Error - iterator = NULL - Can't find parent!\n");
      }
      break;
   }
   
   case PGLIST_REMOVE_AT:
   {
      printf(__FUNCTION__": PGLIST_REMOVE_AT: pos = %d\n", *params);

      //
      // If we're removing the first element
      //
      if ( DATA->widgetlist ) {
         listnode *entryToRemove;
         
         printf(__FUNCTION__": PGLIST_REMOVE_AT: DATA->widgetlist = 0x%x\n", DATA->widgetlist);
         entryToRemove = list_locate_row(DATA->widgetlist, DATA->widgetlist->prev, params[0], DATA->widgetlistsize);
         list_remove_row(self, entryToRemove, params[0]);
            
      }  // End of DATA->widgetlist check
      break;
   }
   
   case PGLIST_REMOVE:
   {
      listnode *iterator;
      printf(__FUNCTION__": PGLIST_REMOVE: widget = %d\n", *params);
      
      //
      // There really is not any fluffy algorithm I can use to get the row position, given the parent.
      // Start a linear search for the widget.
      //
      if ( DATA->widgetlist ) {

         iterator = DATA->widgetlist;

         //
         // Check the first one
         //
         do {
            if ( iterator->handle == params[0] )
               break;

         } while ( iterator && (iterator = iterator->next) != DATA->widgetlist );

         //
         // Two cases here.  Either iterator->handle is the one we're looking for, or
         // iterator == DATA->widgetlist
         //
         if ( iterator->handle == params[1] ) {

            listnode *entryToRemove;
            printf(__FUNCTION__": PGLIST_REMOVE: Found element to remove\n");
            list_remove_row(self, iterator, iterator->position);
         }
         else {
            printf(__FUNCTION__": PGLIST_REMOVE: Can't find widget handle %d in widgetlist\n", params[0]);
         }
      }
      else {
         printf(__FUNCTION__": PGLIST_REMOVE: Error - DATA->widgetlist is NULL\n");
      }
      break;
   }

   }  // End of switch

   return sucess;
   
}  // End of list_command
      
void list_trigger(struct widget *self,long type,union trigparam *param) {

   printf(__FUNCTION__": Enter\n");
   
   //
   // Figure out what command (either from the input driver or client) was received.
   //
   switch (type) {

   case TRIGGER_STREAM:
   {
      struct pgcommand *cmd;
      char *buffer = param->stream.data;
      unsigned long remaining = param->stream.size;
      int i;
      signed long *params;
      
      printf(__FUNCTION__": TRIGGER_STREAM - param.data = 0x%x, param.size = %d\n", param->stream.data, param->stream.size);      
      //
      // Process all the commands received.
      //
      while (remaining) {
	 
         /* Out of space? */
         if (remaining < sizeof(struct pgcommand))
            return;
         
         cmd = (struct pgcommand *) buffer;
         cmd->command = ntohs(cmd->command);
         cmd->numparams = ntohs(cmd->numparams);
	 
         params = (signed long *) (buffer + sizeof(struct pgcommand));
         buffer += sizeof(struct pgcommand) + 
         cmd->numparams * sizeof(signed long);
         remaining -= sizeof(struct pgcommand) + 
         cmd->numparams * sizeof(signed long);

         //
         // Finished processing?
         //
         if (remaining < 0)
            break;
	 
         /* Convert parameters */
         for (i=0;i<cmd->numparams;i++)
            params[i] = ntohl(params[i]);
	 
         list_command(self,cmd->command,cmd->numparams,params);
	 
      } // End of remaining
   }
   break;
   
     //
     // Keyboard trigger stuff
     //
  case TRIGGER_KEYUP:
  {
     struct widget *rw, *sw;
     listnode *selectedNode;
     int pos;
     
     printf(__FUNCTION__": TRIGGER_KEYUP - param.key = %d\n", param->kbd.key);
     
     if ( param->kbd.key == PGKEY_UP ) {

        printf(__FUNCTION__": TRIGGER_KEYUP - PGKEY_UP\n");
        
        pos = list_get(self, PG_WP_SELECTED) - 1;
        selectedNode = list_locate_row(DATA->widgetlist, DATA->widgetlist->prev, pos, DATA->widgetlistsize);
        if ( selectedNode && selectedNode->position == pos && !iserror(rdhandle((void **)&rw, PG_TYPE_WIDGET, -1, selectedNode->handle)) &&  !iserror(rdhandle((void **)&sw, PG_TYPE_WIDGET, -1, self->scrollbind)) ) {

           printf(__FUNCTION__": TRIGGER_KEYUP - PGKEY_UP - selectedNode = 0x%x, pos = %d\n", selectedNode, pos);
           printf(__FUNCTION__": TRIGGER_KEYUP - PGKEY_UP - struct widget * found => 0x%x\n", rw);
           
           //
           // If the current row's y + height is outside of the current view window, move the view window.
           //
           if ( rw->in->div->y < DATA->viewWindowTopY ) {

              printf(__FUNCTION__": DANGER!!!!!!! - Scrolling does not work!!!!!!!\n");
              printf(__FUNCTION__": NOT GONNA DO IT\n");

           }
           else {

              printf(__FUNCTION__": TRIGGER_KEYUP - PGKEY_UP - moving one row up\n");
              list_set(self, PG_WP_SELECTED, pos);
              update(NULL, 1);              
           }
        }
        else {
           printf(__FUNCTION__": TRIGGER_KEYUP - could not locate listnode for current selected row!\n");
        }

     }
     else if ( param->kbd.key == PGKEY_DOWN ) {

        printf(__FUNCTION__": TRIGGER_KEYUP - PGKEY_DOWN\n");

        //
        // Get a handle to the selected widget
        //
        pos = list_get(self, PG_WP_SELECTED) + 1;
        selectedNode = list_locate_row(DATA->widgetlist, DATA->widgetlist->prev, pos, DATA->widgetlistsize);
        if ( selectedNode && selectedNode->position == pos && !iserror(rdhandle((void **)&rw, PG_TYPE_WIDGET, -1, selectedNode->handle)) &&  !iserror(rdhandle((void **)&sw, PG_TYPE_WIDGET, -1, self->scrollbind)) ) {

           printf(__FUNCTION__": TRIGGER_KEYUP - PGKEY_DOWN - selectedNode = 0x%x, pos = %d\n", selectedNode, pos);
           printf(__FUNCTION__": TRIGGER_KEYUP - PGKEY_DOWN - struct widget * found => 0x%x\n", rw);

           //
           // If the current row's y + height is outside of the current view window, move the view window.
           //
           if ( (rw->in->div->y + rw->in->div->h) > DATA->viewWindowBottomY ) {

              printf(__FUNCTION__": DANGER!!!!!!! - Scrolling does not work!!!!!!!\n");
              printf(__FUNCTION__": NOT GONNA DO IT\n");

           }
           else {

              printf(__FUNCTION__": TRIGGER_KEYUP - PGKEY_UP - moving one row down\n");
              list_set(self, PG_WP_SELECTED, pos);
              update(NULL, 1);
           }
        }
        else {
           printf(__FUNCTION__": TRIGGER_KEYUP - could not locate listnode for current selected row!\n");
        }
     }
     else if ( param->kbd.key == PGKEY_KP_ENTER ) {

        printf(__FUNCTION__": TRIGGER_KEYUP - PGKEY_KP_ENTER - Received selection key\n");

        /*
         * Send an event to the client side.  Hope someone is listening
         */
        post_event(PG_WE_KBD_KEYUP,self, param->kbd.key, self->owner, NULL);
     }
     else {
        printf(__FUNCTION__": TRIGGER_KEYUP - unrecognized key = %d\n", param->kbd.key);
     }
  }
  break;
     
  case TRIGGER_KEYDOWN:
  {
     int pos;
     struct widget *sw;
     
     printf(__FUNCTION__": TRIGGER_KEYUP - param.key = %d\n", param->kbd.key);
     pos = list_get(self, PG_WP_SELECTED);
     if ( pos == 0  &&  !iserror(rdhandle((void **)&sw, PG_TYPE_WIDGET, -1, self->scrollbind)) ) {
        DATA->viewWindowTopY = sw->in->y;
        DATA->viewWindowBottomY = self->in->h;
     }
     
  }
  break;

   }  // End of switch

}

/* The End */




