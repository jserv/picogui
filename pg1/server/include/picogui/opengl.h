/* $Id$
 *
 * picogui/opengl.h - This is an extension to PicoGUI, supported by the sdlgl
 *                    driver, for including OpenGL commands in groplists.
 *                    Note that it is not part of the core of picogui, and you
 *                    must include this header file separately.
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

#ifndef _H_PG_OPENGL
#define _H_PG_OPENGL

/*! 
 * \file opengl.h
 * \brief OpenGL interface for PicoGUI
 * 
 * This file contains constants and macros to use OpenGL from PicoGUI clients and themes.
 *
 */

/*!
 * \defgroup opengl OpenGL Interface
 *
 * This is an extension to PicoGUI used by the sdlgl driver to facilitate
 * 3D and/or accelerated drawing operations by the client and themes.
 * Note that the driver provides OpenGL implementations of all standard
 * PicoGUI primitives, but this interface brings additional operations
 * for things like scaled blits, matrix transformations, and real 3D operations!
 *
 * Note that using anything in this file makes your program INCOMPATIBLE with anything
 * other than the sdlgl driver, and it should be used ONLY in themes designed
 * for sdlgl, or in applications that require 3D drawing!
 *
 * \{
 */

/****** Gropnode constants ******/

/*!
 * \defgroup glgrops Gropnode Constants
 *
 * Note that these have to include PG_GROP_USER, and follow the conventions
 * in constants.h for the gropnode type bitfield.
 *
 * A note on floating point:
 * Since PicoGUI's protocol was not designed to require floating point,
 * it assumes all gropnode parameters are 32-bit integers.
 * All places where OpenGL would use floating point numbers, this interface
 * uses 16:16 fixed point notation.
 *
 * A note on parameters:
 * Most of these nodes are marked nonvisual and unpositioned so that
 * all the parameters are stored in the gropnode params, and the gropnode
 * position is ignored. Some of them, such as glRotatef take more parameters.
 * In the case of glRotatef, it would take 4 floating point numbers. The three
 * gropnode parameters are used to specify the rotation axis, and the x,y are
 * used together to form the angle.
 *
 * \{
 */

#define PG_GROP_GL_BINDTEXTURE        0x3017   //!< params: bitmap handle (assumes GL_TEXTURE_2D)
#define PG_GROP_GL_ENABLE             0x3027   //!< params: integer constant
#define PG_GROP_GL_DISABLE            0x3037   //!< params: integer constant
#define PG_GROP_GL_DEPTHFUNC          0x3047   //!< params: integer constant
#define PG_GROP_GL_SHADEMODEL         0x3057   //!< params: integer constant
#define PG_GROP_GL_MATRIXMODE         0x3067   //!< params: integer constant
#define PG_GROP_GL_LOADIDENTITY       0x3073   //!< (no params)
#define PG_GROP_GL_PUSHMATRIX         0x3083   //!< (no params)
#define PG_GROP_GL_POPMATRIX          0x3093   //!< (no params) 
#define PG_GROP_GL_TRANSLATEF         0x30AF   //!< params: x,y,z (in fixed point)
#define PG_GROP_GL_ROTATEF            0x30BD   //!< params: x,y,z (in fixed point, angle is stored in x:y)
#define PG_GROP_GL_SCALEF             0x30CF   //!< params: x,y,z (in fixed point)
#define PG_GROP_GL_BEGIN              0x30D7   //!< params: integer constant
#define PG_GROP_GL_TEXCOORD2F         0x30EB   //!< params: x,y (in fixed point)
#define PG_GROP_GL_VERTEX3F           0x30FF   //!< params: x,y,z (in fixed point)
#define PG_GROP_GL_END                0x3103   //!< (no params) 
#define PG_GROP_GL_HINT               0x311B   //!< params: 2 integer constants
#define PG_GROP_GL_NORMAL3F           0x312F   //!< params: x,y,z (in fixed point)
#define PG_GROP_GL_LIGHTFV            0x313D   //!< params: x,y,z (in fixed point, 2 integers in x and y, fourth argument in w:h)
#define PG_GROP_GL_MATRIX_PIXELCOORD  0x3143   //!< (no params. Multiplies in PicoGUI's pixel coordinates matrix)
#define PG_GROP_GL_COLOR              0x3157   //!< params: one pgcolor
#define PG_GROP_GL_BLENDFUNC          0x316B   //!< params: 2 integer constants
#define PG_GROP_GL_FEEDBACK           0x317C   //!< params: filtering mode, source buffer, (mipmap flag:mipmap index)
#define PG_GROP_GL_MATERIALFV         0x318D   //!< params: x,y,z (in fixed point, 2 integers in x and y, fourth argument in w:h)
#define PG_GROP_GL_MATERIALI          0x319F   //!< params: side, parameter, value (integers)
#define PG_GROP_GL_TEXPARAMETERI      0x31AF   //!< params: texture, parameter, value
#define PG_GROP_GL_TEXGENFV           0x31BD   //!< params: x,y,z (in fixed point, 2 integers in x and y, fourth argument in w:h)
#define PG_GROP_GL_TEXGENI            0x31CF   //!< 3 integer params

//! \}
//! \}

#endif /* __H_PG_OPENGL */
/* The End */
