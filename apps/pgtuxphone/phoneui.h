/* $Id: phoneui.h,v 1.1 2001/10/30 01:32:28 micahjd Exp $
 *
 * phoneui.h - UI functions other than the basic phonecall info bar
 *
 * Copyright (C) 2001 Micah Dowty <micahjd@users.sourceforge.net>
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
 */

#ifndef _H_PG_PHONEUI
#define _H_PG_PHONEUI

#include <picogui.h>

int btnRedial(struct pgEvent *evt);
int btnHistory(struct pgEvent *evt);
int btnKeypad(struct pgEvent *evt);

#endif /* _H_PG_PHONEUI */

/* The End */
