/* $Id: appmgr.h,v 1.2 2000/04/24 02:38:36 micahjd Exp $
 *
 * appmgr.h - All the window-manager-ish functionality, except we don't
 * do windows (X windows, that is?)
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micah@homesoftware.com>
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

#ifndef __H_APPMGR
#define __H_APPMGR

/* Global objects */
extern handle defaultfont;

/* Init */
g_error appmgr_init(struct dtstack *m_dts);

/* Pass it a bitmap handle, or NULL to restore default background */
g_error appmgr_setbg(int owner,handle bitmap);

#endif /* __H_APPMGR */
/* The End */
