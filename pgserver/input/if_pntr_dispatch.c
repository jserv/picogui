/* $Id: if_pntr_dispatch.c,v 1.1 2002/05/22 09:26:32 micahjd Exp $
 *
 * if_pntr_dispatch.c - Dispatch mouse pointer events to widgets
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2002 Micah Dowty <micahjd@users.sourceforge.net>
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
#include <pgserver/input.h>

void infilter_pntr_dispatch_handler(u32 trigger, union trigparam *param) {

}

struct infilter infilter_pntr_dispatch = {
  accept_trigs: 0,
  absorb_trigs: 0,
  handler: &infilter_pntr_dispatch_handler,
};

/* The End */






