/*
 *
 * list.h - Low-level write operations on list objects.
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micahjd@users.sourceforge.net>
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
 * Contributors: Philippe Ney <philippe.ney@smartdata.ch>
 * 
 * 
 * 
 */

/*
 * This file contains operations that one can make to a list object.  For now, you can
 * insert things to a list, remove things from a list.  Event registration (like when
 * a select or enter key pressed) occurs with a simple pgSetWidget(PG_WP_BIND) on the list
 * object.
 */
#ifndef _LIST_H_
#define _LIST_H_

/*
 * Define the insert command.  This command will insert a widget into a particular location,
 * kinda like insertAt(pghandle widget_to_insert, int position)
 */
#define PGLIST_INSERT_AT 1

/*
 * Define another insert command.  This command will insert a widget with respect to a parent and
 * a relationship.  insertAtParent(pghandle widget_to_insert, pghandle parent, int rship)
 */
#define PGLIST_INSERT_AT_PARENT 2

/*
 * Define remove command.  This command will remove a widget at some location in the list.
 * removeAt(int position_to_remove)
 */
#define PGLIST_REMOVE_AT 3

/*
 * Define another remove command.  This command will remove a widget with some handle.
 * removeWithHandle(pghandle widget_to_remove)
 */
#define PGLIST_REMOVE 4

#endif // _LIST_H_
