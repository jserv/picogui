/* X-Chat
 * Copyright (C) 1998 Peter Zelezny.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "fe-gtk.h"
#include "../common/xchat.h"
#include "gtkutil.h"

#ifdef USE_GDK_PIXBUF
#include <gdk-pixbuf/gdk-pixbuf.h>
#endif

#ifdef USE_XLIB
#include <gdk/gdkx.h>
#endif

#include "../pixmaps/globe.xpm"
#include "../pixmaps/server.xpm"
#include "../pixmaps/admin.xpm"
#include "../pixmaps/op.xpm"
#include "../pixmaps/hop.xpm"
#include "../pixmaps/voice.xpm"
#ifdef USE_XLIB
#include "../pixmaps/xchat_mini.xpm"
#endif
#include "../pixmaps/xchat.xpm"


GdkPixmap *pix_globe;
GdkPixmap *pix_server;
GdkPixmap *pix_admin;
GdkPixmap *pix_op;
GdkPixmap *pix_hop;
GdkPixmap *pix_voice;
GdkPixmap *pix_xchat_mini;
GdkPixmap *pix_xchat;

GdkBitmap *mask_globe;
GdkBitmap *mask_server;
GdkBitmap *mask_admin;
GdkBitmap *mask_op;
GdkBitmap *mask_hop;
GdkBitmap *mask_voice;
GdkBitmap *mask_xchat_mini;
GdkBitmap *mask_xchat;



static void
pixmap_load_from_data (GdkPixmap **pix, GdkBitmap **mask, char **xpm_data)
{
#ifdef USE_XLIB
	*pix = gdk_pixmap_create_from_xpm_d (GDK_ROOT_PARENT(), mask, NULL, xpm_data);
#else
	*pix = gdk_pixmap_create_from_xpm_d (NULL, mask, NULL, xpm_data);
#endif
}

static GdkPixmap *
pixmap_load_from_file_real (char *file)
{
#ifdef USE_GDK_PIXBUF
	GdkPixbuf *img;
	GdkPixmap *pixmap;

	img = gdk_pixbuf_new_from_file(file);
	if (!img)
		return NULL;
	gdk_pixbuf_render_pixmap_and_mask(img, &pixmap, NULL, 128);
	gdk_pixbuf_unref(img);

	return pixmap;
#else

#ifdef USE_XLIB
	return gdk_pixmap_create_from_xpm (GDK_ROOT_PARENT(), NULL, NULL, file);
#else
	return gdk_pixmap_create_from_xpm (NULL, NULL, NULL, file);
#endif

#endif
}

GdkPixmap *
pixmap_load_from_file (char *filename)
{
	char buf[256];
	GdkPixmap *pix;

	if (filename[0] == '\0')
		return NULL;

	pix = pixmap_load_from_file_real (filename);
	if (pix == NULL)
	{
		strcpy (buf, "Cannot open:\n\n");
		strcpy (buf + 14, filename);
		gtkutil_simpledialog (buf);
	}

	return pix;
}

void
pixmaps_init (void)
{
	pixmap_load_from_data (&pix_globe,		&mask_globe,		globe_xpm);
	pixmap_load_from_data (&pix_server,		&mask_server,		server_xpm);
	pixmap_load_from_data (&pix_admin,		&mask_admin,		admin_xpm);
	pixmap_load_from_data (&pix_op,			&mask_op,			op_xpm);
	pixmap_load_from_data (&pix_hop,			&mask_hop,			hop_xpm);	
	pixmap_load_from_data (&pix_voice,		&mask_voice,		voice_xpm);
	pixmap_load_from_data (&pix_xchat,		&mask_xchat,		xchat_xpm);
#ifdef USE_XLIB
	pixmap_load_from_data (&pix_xchat_mini,&mask_xchat_mini, xchat_mini_xpm);
#endif
}

