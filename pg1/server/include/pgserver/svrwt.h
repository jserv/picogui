/* $Id$
 * 
 * svrwt.h - Server-side in-memory representation of widget templates
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

#ifndef __SVRWT_H
#define __SVRWT_H

#include <pgserver/handle.h>

/*
 * This structure is what's actually referred to by a widget template handle,
 * after it's loaded. Any handles loaded as a consequence of loading the widget
 * template (not instantiating it) are attached to the template using handle groups.
 * Handles created by instantiating it are the responsibility of the client to deal with.
 *
 * Note that by the time this structure is relevant, all load-time requests have been
 * run and discarded, and this structure should only contain the info necessary
 * to instantiate the template. Therefore it needs the handle table and the instantiation
 * requests.
 */
struct pgmemwt {
  handle *htable;
  int num_handles;
  u8 *requests;
  int num_instance;
  int requests_length;
};

/* High-level functions to manage widget templates */
g_error wt_load(handle *wt, int owner, const u8 *template, u32 template_len);
g_error wt_instantiate(handle *instance, struct pgmemwt *wt, handle wt_h, int owner);
void wt_free(struct pgmemwt *wt);

/* Run a block of requests, using the given handle table */
g_error wt_run_requests(handle group, int owner, const u8 **requests, int *requests_len, 
			handle *htable, int num_handles, int num_requests);

/* Zlib's CRC-32 algorithm */
u32 crc32(u32 crc, const u8 *buf, int len);

#endif /* __SVRWT_H */
