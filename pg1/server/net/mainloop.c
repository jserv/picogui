/* $Id$
 *
 * mainloop.c - Process incoming stimuli from clients and drivers
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
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

#include <pgserver/common.h>
#include <pgserver/pgnet.h>
#include <pgserver/init.h>

/* Structures for managing the queue of child processes */
struct child_node {
  const char *cmdline;
  struct child_node *next;
} *childlist_head, *childlist_tail;

/* Number of child processes pending popping from the stack */
int childqueue_pending;

/* Actually pop the child process off the stack, 
 * instead of setting the above flag 
 */
g_error childqueue_pop_internal(void);


/******************************************************** Public functions **/

/* Start the main loop, run iterations while mainloop_is_running */
g_error pgserver_mainloop(void) {
  g_error e;

  pgserver_mainloop_start();
  while (pgserver_mainloop_is_running()) {
    e = pgserver_mainloop_iteration();
    errorcheck;
  }
  
  return success;
}

/* Functions to manipulate the running/stopped state of the main loop */
int mainloop_runflag;
void pgserver_mainloop_start(void) {
  mainloop_runflag = 1;
}
void pgserver_mainloop_stop(void) {
  mainloop_runflag = 0;
}
int pgserver_mainloop_is_running(void) {
  return mainloop_runflag;
}

/* Run one iteration of all pgserver's main loop activities,
 * including network, input, and rendering.
 */
g_error pgserver_mainloop_iteration(void) {
  g_error e;

  /* Run any child processes we have pending */
  while (childqueue_pending>0) {
    e = childqueue_pop_internal();
    errorcheck;
    childqueue_pending--;
  }
 
  /* FIXME: this net_iteration thing actually handles input
   *        too, it needs to be split up into an OS-specific
   *        main loop that drivers and network modules register with.
   */
  net_iteration();
  
  return success;
}

/* Push this command line onto a queue of child processes to run.
 * If 'name' is NULL, this is ignored. This does not make a copy
 * of 'name' since it is assumed that it's something persistent, like
 * a config database value or a constant!
 */
g_error childqueue_push(const char *cmdline) {
  g_error e;
  struct child_node *n;
  
  if (cmdline) {
    e = g_malloc((void**)&n, sizeof(struct child_node));
    errorcheck;
    n->cmdline = cmdline;
    n->next = NULL;
    
    if (childlist_head) {
      childlist_tail->next = n;
      childlist_tail = n;
    }
    else {
      childlist_head = n;
      childlist_tail = n;
    }
  }
  return success;
}

/* Free all the memory used by the child queue */
void childqueue_shutdown(void) {
  struct child_node *n,*dead;

  for (n=childlist_head;n;) {
    dead = n;
    n = n->next;
    g_free(dead);
  }
  childlist_head = childlist_tail = NULL;
}

/* Start the next child from the queue if there is one
 * Note that since this may be called from a signal handler,
 * it only makes a note to run the child later. The child process
 * isn't actually started until the next main loop iteration.
 */
void childqueue_pop(void) {
  childqueue_pending++;
}


/******************************************************** Internal utilities **/

/* Start the next child from the queue if there is one */
g_error childqueue_pop_internal(void) {
  g_error e;
  struct child_node *n;
  
  if (childlist_head) {
    n = childlist_head;
    childlist_head = childlist_head->next;
    if (!childlist_head)
      childlist_tail = NULL;

    os_child_run(n->cmdline);
    g_free(n);
  }
  return success;
}

int childqueue_is_empty(void) {
  return childlist_tail == NULL && childqueue_pending == 0;
}


/* The End */

