/* $Id: Ship.h,v 1.3 2002/11/26 19:18:07 micahjd Exp $
 *
 * Ship.h - Interface for the simple scriptable attack vessel
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

#ifndef _H_SHIP
#define _H_SHIP

#include "ScriptableObject.h"
#include "PGTexture.h"

class Ship : public ScriptableObject {
 public:
  Ship(PythonInterpreter *py);
  virtual ~Ship();

  void draw(void);

 private:
  PGTexture *shipTexture;
};

#endif /* _H_SHIP */
