/*
 * appmgr.h - All the window-manager-ish functionality, except we don't
 * do windows (X windows, that is?)
 * $Revision: 1.1 $
 * 
 * Micah Dowty <micah@homesoftware.com>
 * 
 * This file is released under the GPL. Please see the file COPYING that
 * came with this distribution.
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
