/* $Id: pgmain.h,v 1.3 2001/01/26 11:18:16 micahjd Exp $
 *
 * pgmain.h - just a few things related to the main loop
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micahjd@users.sourceforge.net>
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

#ifndef _H_PGMAIN
#define _H_PGMAIN

extern volatile int in_shutdown;
void request_quit(void);

/* Functions for initializing function pointer tables when they can't
 * be done at compile time (ucLinux, for example) */
#ifdef RUNTIME_FUNCPTR
void widgettab_init(void);
void drivertab_init(void);
void rqhtab_init(void);
#endif

#endif /* _H_PGMAIN */
/* The End */
