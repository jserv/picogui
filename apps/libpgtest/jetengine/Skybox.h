/* $Id: Skybox.h,v 1.4 2002/11/26 19:18:07 micahjd Exp $
 *
 * Skybox.h - Interface for a simple skybox using ScriptableObject and PGTexture
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

#ifndef _H_SKYBOX
#define _H_SKYBOX

#include "ScriptableObject.h"
#include "PGTexture.h"

class Skybox : public ScriptableObject {
  public:
    Skybox(PythonInterpreter *py);
    virtual ~Skybox();

    void draw(void);

  private:
    // always left-right-front-back-top-bottom
    PGTexture	*sides[6];
    int 	lists[6];
};

#endif /* _H_SKYBOX */
