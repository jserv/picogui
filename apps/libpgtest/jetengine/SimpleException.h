/* $Id: SimpleException.h,v 1.2 2002/11/26 19:18:07 micahjd Exp $
 *
 * SimpleException.h - Abstract base class for exceptions that can only
 *                     be dumped to stderr
 *
 * Copyright (C) 2002 Micah Dowty and David Trowbridge
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

#ifndef _H_SIMPLEEXCEPTION
#define _H_SIMPLEEXCEPTION

class SimpleException {
 public:
  virtual void show(void) = 0;
};

#endif /* _H_SIMPLEEXCEPTION */
