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
 * =========================================================================
 *
 * xtext, the text widget used by X-Chat.
 *
 * By Peter Zelezny <zed@linux.com>.
 * Some functions used from Zvt and Eterm (transparency stuff).
 *
 */

#define XCHAT							/* using xchat */
#define REFRESH_TIMEOUT 20
#define WORDWRAP_LIMIT 24
#define TINT_VALUE 195				/* 195/255 of the brightness. */
#define MOTION_MONITOR 1			/* URL hilights. */
#define MARGIN 2						/* dont touch. */
#define SMOOTH_SCROLL

#ifdef XCHAT
#include "../../config.h"			/* can define USE_XLIB here */
#else
#define USE_XLIB
#endif

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkselection.h>

#ifdef USE_XLIB
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#endif

#include "xtext.h"

#ifdef USE_GDK_PIXBUF
#include <gdk-pixbuf/gdk-pixbuf.h>
#endif

#undef GTK_WIDGET
#define GTK_WIDGET(n) ((GtkWidget*)n)
#undef GTK_OBJECT
#define GTK_OBJECT(n) ((GtkObject*)n)
#undef GTK_OBJECT_CLASS
#define GTK_OBJECT_CLASS(n) ((GtkObjectClass*)n)

#ifdef WIN32
#define charlen(str) g_utf8_skip[*(guchar *)(str)]
#else
#define charlen(str) mblen(str, MB_CUR_MAX)
#endif

/* is delimiter */
#define is_del(c) \
	(c == ' ' || c == '\n' || c == ')' || c == '(' || \
	 c == '>' || c == '<' || c == ATTR_RESET || c == ATTR_BOLD || c == 0)

static GtkWidgetClass *parent_class = NULL;

enum
{
	WORD_CLICK,
	LAST_SIGNAL
};

/* values for selection info */
enum
{
	TARGET_STRING,
	TARGET_TEXT,
	TARGET_COMPOUND_TEXT
};

static guint xtext_signals[LAST_SIGNAL] = { 0 };

#ifdef XCHAT
char *nocasestrstr (char *text, char *tofind);	/* util.c */
int get_stamp_str (time_t, char *, int);
#endif
static void gtk_xtext_render_page (GtkXText * xtext);
static void gtk_xtext_calc_lines (GtkXText * xtext, int);
#ifdef USE_XLIB
static void gtk_xtext_load_trans (GtkXText * xtext);
static void gtk_xtext_free_trans (GtkXText * xtext);
#endif
static textentry *gtk_xtext_nth (GtkXText * xtext, textentry * start_ent,
											int line, int *subline);
static gint gtk_xtext_selection_kill (GtkWidget * widget,
												  GdkEventSelection * event);
static void gtk_xtext_selection_get (GtkWidget * widget,
												 GtkSelectionData * selection_data_ptr,
												 guint info, guint time);
static int gtk_xtext_text_width (GtkXText * xtext, unsigned char *text,
											int len, int *mb_ret);
static void gtk_xtext_adjustment_changed (GtkAdjustment * adj,
														GtkXText * xtext);
static void gtk_xtext_draw_sep (GtkXText * xtext, int height);
static void gtk_xtext_render_ents (GtkXText * xtext, textentry *, textentry *,
											  int);
static void gtk_xtext_recalc_widths (GtkXText * xtext, int);
static void gtk_xtext_fix_indent (GtkXText * xtext);
static char *gtk_xtext_conv_color (unsigned char *text, int len, char *outbuf, int *newlen, int fonttype);
static guint gtk_xtext_get_type (void);

/* some utility functions first */

#ifndef XCHAT	/* xchat has this in util.c */

static char *
nocasestrstr (char *s, char *wanted)
{
   register const size_t len = strlen (wanted);

   if (len == 0)
     return (char *)s;
   while (toupper(*s) != toupper(*wanted) || strncasecmp (s, wanted, len))
     if (*s++ == '\0')
       return (char *)NULL;
   return (char *)s;   
}

#endif

static void
xtext_set_fg (GdkGC *gc, gulong pixel)
{
	GdkColor col;

	col.pixel = pixel;
	gdk_gc_set_foreground (gc, &col);
}

static void
xtext_set_bg (GdkGC *gc, gulong pixel)
{
	GdkColor col;

	col.pixel = pixel;
	gdk_gc_set_background (gc, &col);
}

static void
gtk_xtext_init (GtkXText * xtext)
{
	xtext->old_value = -1;
	xtext->pixmap = NULL;
	xtext->text_first = NULL;
	xtext->text_last = NULL;
	xtext->last_ent_start = NULL;
	xtext->last_ent_end = NULL;
	xtext->io_tag = 0;
	xtext->add_io_tag = 0;
	xtext->scroll_tag = 0;
/*   xtext->frozen = 0;*/
	xtext->num_lines = 0;
	xtext->max_lines = 0;
	xtext->col_back = 19;
	xtext->col_fore = 18;
	xtext->nc = 0;
	xtext->pixel_offset = 0;
	xtext->scrollbar_down = TRUE;
	xtext->bold = FALSE;
	xtext->underline = FALSE;
	xtext->reverse = FALSE;
	xtext->time_stamp = FALSE;
	xtext->font = NULL;
	xtext->error_function = NULL;
	xtext->urlcheck_function = NULL;
	xtext->color_paste = FALSE;
	xtext->skip_fills = FALSE;
	xtext->skip_border_fills = FALSE;
	xtext->skip_stamp = FALSE;
	xtext->do_underline_fills_only = FALSE;
	xtext->jump_out_early = FALSE;
	xtext->recycle = FALSE;
	xtext->tint_red = xtext->tint_green = xtext->tint_blue = TINT_VALUE;

	xtext->adj = (GtkAdjustment *) gtk_adjustment_new (0, 0, 0, 1, 0, 0);
	gtk_object_ref ((GtkObject *) xtext->adj);
	gtk_object_sink ((GtkObject *) xtext->adj);

	gtk_signal_connect (GTK_OBJECT (xtext->adj), "value_changed",
							  GTK_SIGNAL_FUNC (gtk_xtext_adjustment_changed), xtext);
	gtk_signal_connect (GTK_OBJECT (xtext), "selection_clear_event",
							  GTK_SIGNAL_FUNC (gtk_xtext_selection_kill), xtext);
	{
		static const GtkTargetEntry targets[] = {
			{ "STRING", 0, TARGET_STRING },
			{ "TEXT",   0, TARGET_TEXT }, 
			{ "COMPOUND_TEXT", 0, TARGET_COMPOUND_TEXT }
		};
		static const gint n_targets = sizeof (targets) / sizeof (targets[0]);

		gtk_selection_add_targets (GTK_WIDGET (xtext), GDK_SELECTION_PRIMARY,
											targets, n_targets);
		gtk_selection_add_targets (GTK_WIDGET (xtext),
											gdk_atom_intern ("CLIPBOARD", FALSE),
											targets, n_targets);
	}
	gtk_signal_connect (GTK_OBJECT (xtext), "selection_get",
							  GTK_SIGNAL_FUNC (gtk_xtext_selection_get), xtext);
}

static void
gtk_xtext_adjustment_set (GtkXText * xtext, int fire_signal)
{
	GtkAdjustment *adj = xtext->adj;

	adj->lower = 0;
	adj->upper = xtext->num_lines;

	adj->page_size =
		(GTK_WIDGET (xtext)->allocation.height -
		 xtext->font->descent) / xtext->fontsize;
	adj->page_increment = adj->page_size;

	if (adj->value > adj->upper - adj->page_size)
		adj->value = adj->upper - adj->page_size;

	if (fire_signal)
		gtk_adjustment_changed (adj);
}

static gint
gtk_xtext_adjustment_timeout (GtkXText * xtext)
{
	gtk_xtext_render_page (xtext);
	xtext->io_tag = 0;
	return 0;
}

static void
gtk_xtext_adjustment_changed (GtkAdjustment * adj, GtkXText * xtext)
{
/*   if (xtext->frozen)
      return;*/

#ifdef SMOOTH_SCROLL
	if (xtext->old_value != xtext->adj->value)
#else
	if ((int) xtext->old_value != (int) xtext->adj->value)
#endif
	{
		if (xtext->adj->value >= xtext->adj->upper - xtext->adj->page_size)
			xtext->scrollbar_down = TRUE;
		else
			xtext->scrollbar_down = FALSE;

		if (xtext->adj->value + 1 == xtext->old_value ||
			 xtext->adj->value - 1 == xtext->old_value)	/* clicked an arrow? */
		{
			if (xtext->io_tag)
			{
				g_source_remove (xtext->io_tag);
				xtext->io_tag = 0;
			}
			gtk_xtext_render_page (xtext);
		} else
		{
			if (!xtext->io_tag)
				xtext->io_tag = g_timeout_add (REFRESH_TIMEOUT,
															(GSourceFunc)
															gtk_xtext_adjustment_timeout,
															xtext);
		}
	}
	xtext->old_value = adj->value;
}

GtkWidget *
gtk_xtext_new (int indent, int separator)
{
	GtkXText *xtext;

	xtext = gtk_type_new (gtk_xtext_get_type ());
	xtext->indent = indent;
	xtext->separator = separator;
	xtext->wordwrap = FALSE;

	return GTK_WIDGET (xtext);
}

static void
gtk_xtext_destroy (GtkObject * object)
{
	GtkXText *xtext = GTK_XTEXT (object);
	textentry *ent, *next;

	if (xtext->add_io_tag)
	{
		g_source_remove (xtext->add_io_tag);
		xtext->add_io_tag = 0;
	}

	if (xtext->scroll_tag)
	{
		g_source_remove (xtext->scroll_tag);
		xtext->scroll_tag = 0;
	}

	if (xtext->io_tag)
	{
		g_source_remove (xtext->io_tag);
		xtext->io_tag = 0;
	}

	if (xtext->pixmap)
	{
#ifdef USE_XLIB
		if (xtext->transparent)
			gtk_xtext_free_trans (xtext);
		else
#endif
			gdk_pixmap_unref (xtext->pixmap);
		xtext->pixmap = NULL;
	}

	if (xtext->font)
	{
		gdk_font_unref (xtext->font);
		xtext->font = NULL;
	}

	if (xtext->adj)
	{
		gtk_signal_disconnect_by_data (GTK_OBJECT (xtext->adj), xtext);
		gtk_object_unref (GTK_OBJECT (xtext->adj));
		xtext->adj = NULL;
	}

	if (xtext->bgc)
	{
		gdk_gc_destroy (xtext->bgc);
		xtext->bgc = NULL;
	}

	if (xtext->fgc)
	{
		gdk_gc_destroy (xtext->fgc);
		xtext->fgc = NULL;
	}

	if (xtext->light_gc)
	{
		gdk_gc_destroy (xtext->light_gc);
		xtext->light_gc = NULL;
	}

	if (xtext->dark_gc)
	{
		gdk_gc_destroy (xtext->dark_gc);
		xtext->dark_gc = NULL;
	}

	if (xtext->hand_cursor)
	{
		gdk_cursor_destroy (xtext->hand_cursor);
		xtext->hand_cursor = NULL;
	}

	ent = xtext->text_first;
	while (ent)
	{
		next = ent->next;
		free (ent);
		ent = next;
	}
	xtext->text_first = NULL;

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(*GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
gtk_xtext_realize (GtkWidget * widget)
{
	GtkXText *xtext;
	GdkWindowAttr attributes;
	GdkGCValues val;
	GdkColor col;
	GdkColormap *cmap;

	GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
	xtext = GTK_XTEXT (widget);

	attributes.x = widget->allocation.x;
	attributes.y = widget->allocation.y;
	attributes.width = widget->allocation.width;
	attributes.height = widget->allocation.height;
	attributes.wclass = GDK_INPUT_OUTPUT;
	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.event_mask = gtk_widget_get_events (widget) |
		GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK
#ifdef MOTION_MONITOR
		| GDK_POINTER_MOTION_MASK | GDK_LEAVE_NOTIFY_MASK;
#else
		| GDK_POINTER_MOTION_MASK;
#endif

	cmap = gtk_widget_get_colormap (widget);
	attributes.colormap = cmap;
	attributes.visual = gtk_widget_get_visual (widget);

	widget->window = gdk_window_new (widget->parent->window, &attributes,
												GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL |
												GDK_WA_COLORMAP);

	gdk_window_set_user_data (widget->window, widget);

	xtext->depth = gdk_window_get_visual (widget->window)->depth;

	val.subwindow_mode = GDK_INCLUDE_INFERIORS;
	val.graphics_exposures = 0;

	xtext->bgc = gdk_gc_new_with_values (widget->window, &val,
													 GDK_GC_EXPOSURES | GDK_GC_SUBWINDOW);
	xtext->fgc = gdk_gc_new_with_values (widget->window, &val,
													 GDK_GC_EXPOSURES | GDK_GC_SUBWINDOW);
	xtext->light_gc = gdk_gc_new_with_values (widget->window, &val,
											GDK_GC_EXPOSURES | GDK_GC_SUBWINDOW);
	xtext->dark_gc = gdk_gc_new_with_values (widget->window, &val,
											GDK_GC_EXPOSURES | GDK_GC_SUBWINDOW);

	/* for the separator bar (light) */
	col.red = 0xffff; col.green = 0xffff; col.blue = 0xffff;
	gdk_color_alloc (cmap, &col);
	gdk_gc_set_foreground (xtext->light_gc, &col);

	/* for the separator bar (dark) */
	col.red = 0x8e38; col.green = 0x8e38; col.blue = 0x9f38;
	gdk_color_alloc (cmap, &col);
	gdk_gc_set_foreground (xtext->dark_gc, &col);

	if (xtext->fonttype != FONT_SET && xtext->font != NULL)
		gdk_gc_set_font (xtext->fgc, xtext->font);

	xtext_set_fg (xtext->fgc, xtext->palette[18]);
	xtext_set_bg (xtext->fgc, xtext->palette[19]);
	xtext_set_fg (xtext->bgc, xtext->palette[19]);

#ifdef USE_XLIB
	if (xtext->transparent)
	{
		gtk_xtext_load_trans (xtext);
	} else if (xtext->pixmap)
	{
		gdk_gc_set_tile (xtext->bgc, xtext->pixmap);
		gdk_gc_set_ts_origin (xtext->bgc, 0, 0);
		gdk_gc_set_fill (xtext->bgc, GDK_TILED);
	}
#else
	if (xtext->pixmap)
	{
		gdk_gc_set_tile (xtext->bgc, xtext->pixmap);
		gdk_gc_set_ts_origin (xtext->bgc, 0, 0);
		gdk_gc_set_fill (xtext->bgc, GDK_TILED);
	}
#endif

	xtext->hand_cursor = gdk_cursor_new (GDK_HAND1);

	gdk_window_set_back_pixmap (widget->window, NULL, FALSE);

	/* draw directly to window */
	xtext->draw_buf = widget->window;

	if (xtext->auto_indent)
		xtext->indent = 1;
}

static void
gtk_xtext_size_request (GtkWidget * widget, GtkRequisition * requisition)
{
	requisition->width = GTK_XTEXT (widget)->fontwidth['Z'] * 20;
	requisition->height = (GTK_XTEXT (widget)->fontsize * 10) + 3;
}

static void
gtk_xtext_size_allocate (GtkWidget * widget, GtkAllocation * allocation)
{
	GtkXText *xtext = GTK_XTEXT (widget);

	if (allocation->width == widget->allocation.width &&
		 allocation->height == widget->allocation.height)
		return;

	widget->allocation = *allocation;
	if (GTK_WIDGET_REALIZED (widget))
	{
		gdk_window_move_resize (widget->window, allocation->x, allocation->y,
										allocation->width, allocation->height);
		gtk_xtext_calc_lines (xtext, FALSE);
#ifdef USE_XLIB
		if (xtext->transparent && xtext->shaded)
		{
			gtk_xtext_free_trans (xtext);
			gtk_xtext_load_trans (xtext);
		}
#endif
	}
}

static gint
gtk_xtext_idle (GtkXText *xtext)
{
	xtext->skip_exposure = FALSE;
	return FALSE;
}

static void
gtk_xtext_draw (GtkWidget * widget, GdkRectangle * area)
{
	GtkXText *xtext = GTK_XTEXT (widget);
#ifdef USE_XLIB
	int x, y;

	if (xtext->transparent)
	{
		gdk_window_get_origin (widget->window, &x, &y);
		/* update transparency only if it moved */
		if (xtext->last_win_x != x || xtext->last_win_y != y)
		{
			xtext->last_win_x = x;
			xtext->last_win_y = y;
			if (xtext->shaded)
			{
				xtext->recycle = TRUE;
				gtk_xtext_load_trans (xtext);
				xtext->recycle = FALSE;
			} else
			{
				gtk_xtext_free_trans (xtext);
				gtk_xtext_load_trans (xtext);
			}
		}
	}
#endif

	gtk_xtext_render_page (xtext);
	if (xtext->scrollbar_down)
		gtk_adjustment_set_value (xtext->adj,
											xtext->adj->upper - xtext->adj->page_size);
	xtext->skip_exposure = TRUE;
	gtk_idle_add ((GtkFunction)gtk_xtext_idle, xtext);
}

static int
gtk_xtext_selection_clear (GtkXText * xtext)
{
	textentry *ent;
	int ret = 0;

	ent = xtext->last_ent_start;
	while (ent)
	{
		if (ent->mark_start != -1)
			ret = 1;
		ent->mark_start = -1;
		ent->mark_end = -1;
		if (ent == xtext->last_ent_end)
			break;
		ent = ent->next;
	}

	return ret;
}

static int inline
get_char_width (GtkXText *xtext, unsigned char *str, int *mbl_ret, int is_mb)
{
	int mbl, width;

	switch (xtext->fonttype)
	{
	case FONT_1BYTE:
		width = xtext->fontwidth[(int)*str];
		mbl = 1;
		break;
	case FONT_2BYTE:
		width = gdk_text_width (xtext->font, str, 2);
		mbl = 2;
		break;
	default: /* FONT_SET: */
		if (!is_mb)
		{
			width = xtext->fontwidth[(int)*str];
			mbl = 1;
		} else
		{
			mbl = charlen (str);
			if (mbl == 1)
				width = xtext->fontwidth[(int)*str];
			else if (mbl > 0)
				width = gdk_text_width (xtext->font, str, mbl);
			else
				width = 0;
		}
		break;
	}

	*mbl_ret = mbl;
	return width;
}

static int
find_x (GtkXText *xtext, textentry *ent, unsigned char *text, int x, int indent)
{
	int xx = indent;
	int i = 0;
	int col = FALSE;
	int nc = 0;
	unsigned char *orig = text;
	int mbl;

	while (*text)
	{
		mbl = 1;
		if ((col && isdigit (*text) && nc < 2) ||
			 (col && *text == ',' && isdigit (*(text+1)) && nc < 3))
		{
			nc++;
			if (*text == ',')
				nc = 0;
			text++;
		} else
		{
			col = FALSE;
			switch (*text)
			{
			case ATTR_COLOR:
				col = TRUE;
				nc = 0;
			case ATTR_BEEP:
			case ATTR_RESET:
			case ATTR_REVERSE:
			case ATTR_BOLD:
			case ATTR_UNDERLINE:
				text++;
				break;
			default:
				xx += get_char_width (xtext, text, &mbl, ent->mb);
				text += mbl;
				if (xx >= x)
					return i + (orig - ent->str);
			}
		}

		i += mbl;
		if (text - orig >= ent->str_len)
			return ent->str_len;
	}

	return ent->str_len;
}

static int
gtk_xtext_find_x (GtkXText * xtext, int x, textentry * ent, int subline,
						int line, int *out_of_bounds)
{
	int indent;
	unsigned char *str;

	if (subline < 1)
		indent = ent->indent;
	else
		indent = xtext->indent;

	if (line > xtext->adj->page_size || line < 0)
		return 0;

	if (xtext->grid_offset[line] > ent->str_len)
		return 0;

	if (xtext->grid_offset[line] < 0)
		return 0;

	str = ent->str + xtext->grid_offset[line];

	if (x < indent)
	{
		*out_of_bounds = 1;
		return (str - ent->str);
	}

	*out_of_bounds = 0;

	return find_x (xtext, ent, str, x, indent);
}

static textentry *
gtk_xtext_find_char (GtkXText * xtext, int x, int y, int *off,
							int *out_of_bounds)
{
	textentry *ent;
	int line;
	int subline;
	int win_width;

	gdk_window_get_size (GTK_WIDGET (xtext)->window, &win_width, 0);
	win_width -= MARGIN;

	line = (y - xtext->font->descent + xtext->pixel_offset) / xtext->fontsize;

	subline = xtext->pagetop_subline;
	ent = gtk_xtext_nth (xtext, xtext->pagetop_ent, line, &subline);
	if (!ent)
		return 0;

	if (off)
		*off = gtk_xtext_find_x (xtext, x, ent, subline, line, out_of_bounds);

	return ent;
}

static gint
gtk_xtext_expose (GtkWidget * widget, GdkEventExpose * event)
{
	GtkXText *xtext = GTK_XTEXT (widget);
	textentry *ent_start, *ent_end;

	if (xtext->skip_exposure)
	{
		xtext->skip_exposure = FALSE;
		return FALSE;
	}

	if (event->area.x == 0 && event->area.y == 0 &&
		 event->area.height == widget->allocation.height &&
		 event->area.width == widget->allocation.width)
	{
		gtk_xtext_render_page (xtext);
		return FALSE;
	}

	gdk_draw_rectangle (xtext->draw_buf, xtext->bgc, 1,
							  event->area.x, event->area.y,
							  event->area.width, event->area.height);

	ent_start = gtk_xtext_find_char (xtext, event->area.x, event->area.y,
												NULL, NULL);
	ent_end = gtk_xtext_find_char (xtext, event->area.x + event->area.width,
						event->area.y + event->area.height + xtext->font->descent,
						NULL, NULL);

	xtext->skip_fills = TRUE;
	xtext->skip_border_fills = TRUE;

	gtk_xtext_render_ents (xtext, ent_start, ent_end, TRUE);

	xtext->skip_fills = FALSE;
	xtext->skip_border_fills = FALSE;

	gtk_xtext_draw_sep (xtext, -1);

	return FALSE;
}

static void
gtk_xtext_selection_render (GtkXText *xtext, textentry *start_ent,
									 int start_offset, textentry *end_ent,
									 int end_offset)
{
	textentry *ent, *ent2;

	xtext->skip_border_fills = TRUE;
	xtext->skip_stamp = TRUE;

	/* marking downward? */
	if (xtext->last_ent_start == start_ent &&
			xtext->last_offset_start == start_offset)
	{
		ent = start_ent;
		while (ent)
		{
			if (ent == xtext->last_ent_end)
			{
				gtk_xtext_render_ents (xtext, ent, end_ent, TRUE);
				break;
			}
			if (ent == end_ent)
			{
				gtk_xtext_render_ents (xtext, ent, xtext->last_ent_end, TRUE);
				break;
			}
			ent = ent->next;
		}
	}
	/* marking upward? */
	else if (xtext->last_ent_end == end_ent &&
				xtext->last_offset_end == end_offset)
	{
		ent = start_ent;
		ent2 = xtext->last_ent_start;
		do
		{
			if (ent == xtext->last_ent_start)
			{
				gtk_xtext_render_ents (xtext, start_ent, ent, TRUE);
				break;
			}
			if (ent2 == start_ent)
			{
				gtk_xtext_render_ents (xtext, xtext->last_ent_start, start_ent, TRUE);
				break;
			}
			if (ent)
				ent = ent->next;
			if (ent2)
				ent2 = ent2->next;
		}
		while (ent || ent2);
	}
	else	/* cross-over mark */
	{
		gtk_xtext_render_ents (xtext, xtext->last_ent_start, xtext->last_ent_end, TRUE);
		gtk_xtext_render_ents (xtext, start_ent, end_ent, TRUE);
	}

	xtext->last_ent_start = start_ent;
	xtext->last_ent_end = end_ent;
	xtext->last_offset_start = start_offset;
	xtext->last_offset_end = end_offset;

	xtext->skip_border_fills = FALSE;
	xtext->skip_stamp = FALSE;
}

static void
gtk_xtext_selection_draw (GtkXText * xtext, GdkEventMotion * event)
{
	textentry *ent;
	textentry *ent_end;
	textentry *ent_start;
	int offset_start;
	int offset_end;
	int low_x;
	int low_y;
	int high_x;
	int high_y;
	int tmp;

	if (xtext->select_start_y > xtext->select_end_y)
	{
		low_x = xtext->select_end_x;
		low_y = xtext->select_end_y;
		high_x = xtext->select_start_x;
		high_y = xtext->select_start_y;
	} else
	{
		low_x = xtext->select_start_x;
		low_y = xtext->select_start_y;
		high_x = xtext->select_end_x;
		high_y = xtext->select_end_y;
	}

	ent_start = gtk_xtext_find_char (xtext, low_x, low_y, &offset_start, &tmp);
	ent_end = gtk_xtext_find_char (xtext, high_x, high_y, &offset_end, &tmp);

	if (ent_start && !ent_end)
	{
		ent_end = xtext->text_last;
		offset_end = ent_end->str_len;
	}

	if (!ent_start || !ent_end)
	{
		if (xtext->adj->value != xtext->old_value)
			gtk_xtext_render_page (xtext);
		return;
	}

	gtk_xtext_selection_clear (xtext);

	/* marking less than a complete line? */
	if (ent_start == ent_end)
	{
		ent_start->mark_start = MIN (offset_start, offset_end);
		ent_start->mark_end = MAX (offset_end, offset_start);
		if (offset_start == offset_end)
		{
			if (xtext->fonttype == FONT_SET)
			{
				int mbl;

				mbl = charlen (ent_start->str + offset_start);
				if (0 < mbl)
					ent_start->mark_end += mbl;
			} else
			{
				ent_start->mark_end++;
			}
		}
	} else
	{
		ent_start->mark_start = offset_start;
		ent_start->mark_end = ent_start->str_len;

		if (offset_end != 0)
		{
			ent_end->mark_start = 0;
			ent_end->mark_end = offset_end;
		}
	}

	if (ent_start != ent_end)
	{
		ent = ent_start->next;
		while (ent && ent != ent_end)
		{
			ent->mark_start = 0;
			ent->mark_end = ent->str_len;
			ent = ent->next;
		}
	}

	/* has the selection changed? Dont render unless necessary */
	if (xtext->last_ent_start == ent_start &&
		 xtext->last_ent_end == ent_end &&
		 xtext->last_offset_start == offset_start &&
		 xtext->last_offset_end == offset_end)
		return;

	gtk_xtext_selection_render (xtext, ent_start, offset_start, ent_end, offset_end);
}

static gint
gtk_xtext_scrolldown_timeout (GtkXText * xtext)
{
	int p_y, win_height;

	gdk_window_get_pointer (GTK_WIDGET (xtext)->window, 0, &p_y, 0);
	gdk_window_get_size (GTK_WIDGET (xtext)->window, 0, &win_height);

	if (p_y > win_height &&
		 xtext->adj->value < (xtext->adj->upper - xtext->adj->page_size))
	{
		xtext->adj->value++;
		gtk_adjustment_changed (xtext->adj);
		gtk_xtext_render_page (xtext);
		return 1;
	}

	xtext->scroll_tag = 0;
	return 0;
}

static gint
gtk_xtext_scrollup_timeout (GtkXText * xtext)
{
	int p_y;

	gdk_window_get_pointer (GTK_WIDGET (xtext)->window, 0, &p_y, 0);

	if (p_y < 0 && xtext->adj->value > 0.0)
	{
		xtext->adj->value--;
		gtk_adjustment_changed (xtext->adj);
		gtk_xtext_render_page (xtext);
		return 1;
	}

	xtext->scroll_tag = 0;
	return 0;
}

static void
gtk_xtext_selection_update (GtkXText * xtext, GdkEventMotion * event, int p_y)
{
	int win_height;
	int moved;

	gdk_window_get_size (GTK_WIDGET (xtext)->window, 0, &win_height);

	/* selecting past top of window, scroll up! */
	if (p_y < 0 && xtext->adj->value >= 0)
	{
		if (!xtext->scroll_tag)
			xtext->scroll_tag = g_timeout_add (100,
															(GSourceFunc)
														 	gtk_xtext_scrollup_timeout,
															xtext);
		return;
	}

	/* selecting past bottom of window, scroll down! */
	if (p_y > win_height &&
		 xtext->adj->value < (xtext->adj->upper - xtext->adj->page_size))
	{
		if (!xtext->scroll_tag)
			xtext->scroll_tag = g_timeout_add (100,
															(GSourceFunc)
															gtk_xtext_scrolldown_timeout,
															xtext);
		return;
	}

	moved = xtext->adj->value - xtext->select_start_adj;
	xtext->select_start_y -= (moved * xtext->fontsize);
	xtext->select_start_adj = xtext->adj->value;
	gtk_xtext_selection_draw (xtext, event);
}

static char *
gtk_xtext_get_word (GtkXText * xtext, int x, int y, textentry ** ret_ent,
						  int *ret_off, int *ret_len)
{
	textentry *ent;
	int offset;
	unsigned char *str;
	unsigned char *word;
	int len;
	int out_of_bounds;

	ent = gtk_xtext_find_char (xtext, x, y, &offset, &out_of_bounds);
	if (!ent)
		return 0;

	if (out_of_bounds)
		return 0;

	if (offset == ent->str_len)
		return 0;

	if (offset < 1)
		return 0;

	offset--;

	str = ent->str + offset;

	while (!is_del (*str) && str != ent->str)
		str--;
	word = str + 1;

	len = 0;
	str = word;
	while (!is_del (*str) && len != ent->str_len)
	{
		str++;
		len++;
	}

	if (ret_ent)
		*ret_ent = ent;
	if (ret_off)
		*ret_off = word - ent->str;
	if (ret_len)
		*ret_len = str - word;

	return gtk_xtext_strip_color (word, len, xtext->scratch_buffer, NULL, xtext->fonttype, NULL);
}

#ifdef MOTION_MONITOR

static gint
gtk_xtext_leave_notify (GtkWidget * widget, GdkEventCrossing * event)
{
	GtkXText *xtext = GTK_XTEXT (widget);

	if (xtext->cursor_hand)
	{
		xtext->hilight_start = -1;
		xtext->hilight_end = -1;
		xtext->cursor_hand = FALSE;
		gdk_window_set_cursor (widget->window, 0);
		xtext->skip_border_fills = TRUE;
		xtext->do_underline_fills_only = TRUE;
		gtk_xtext_render_ents (xtext, xtext->hilight_ent, NULL, FALSE);
		xtext->skip_border_fills = FALSE;
		xtext->do_underline_fills_only = FALSE;
		xtext->hilight_ent = NULL;
	}
	return FALSE;
}

#endif

static gint
gtk_xtext_motion_notify (GtkWidget * widget, GdkEventMotion * event)
{
	GtkXText *xtext = GTK_XTEXT (widget);
	int tmp, x, y, offset, len;
	unsigned char *word;
	textentry *word_ent, *old_ent;

	gdk_window_get_pointer (widget->window, &x, &y, 0);

	if (xtext->moving_separator)
	{
		if (x < (3 * widget->allocation.width) / 5 && x > 15)
		{
			tmp = xtext->indent;
			xtext->indent = x;
			gtk_xtext_fix_indent (xtext);
			if (tmp != xtext->indent)
			{
				gtk_xtext_recalc_widths (xtext, FALSE);
				if (xtext->scrollbar_down)
					gtk_adjustment_set_value (xtext->adj, xtext->adj->upper -
													  xtext->adj->page_size);
				if (!xtext->io_tag)
					xtext->io_tag = g_timeout_add (REFRESH_TIMEOUT,
																(GSourceFunc)
																gtk_xtext_adjustment_timeout,
																xtext);
			}
		}
		return FALSE;
	}

	if (xtext->button_down)
	{
		gtk_grab_add (widget);
		/*gdk_pointer_grab (widget->window, TRUE,
									GDK_BUTTON_RELEASE_MASK |
									GDK_BUTTON_MOTION_MASK, NULL, NULL, 0);*/
		xtext->select_end_x = x;
		xtext->select_end_y = y;
		gtk_xtext_selection_update (xtext, event, y);
		return FALSE;
	}
#ifdef MOTION_MONITOR

	if (xtext->urlcheck_function == NULL)
		return FALSE;

	word = gtk_xtext_get_word (xtext, x, y, &word_ent, &offset, &len);
	if (word)
	{
		if (xtext->urlcheck_function (GTK_WIDGET (xtext), word) > 0)
		{
			if (!xtext->cursor_hand ||
				 xtext->hilight_ent != word_ent ||
				 xtext->hilight_start != offset ||
				 xtext->hilight_end != offset + len)
			{
				if (!xtext->cursor_hand)
				{
					gdk_window_set_cursor (GTK_WIDGET (xtext)->window,
											  		xtext->hand_cursor);
					xtext->cursor_hand = TRUE;
				}
				old_ent = xtext->hilight_ent;
				xtext->hilight_ent = word_ent;
				xtext->hilight_start = offset;
				xtext->hilight_end = offset + len;
				xtext->skip_border_fills = TRUE;
				xtext->do_underline_fills_only = TRUE;

				if (old_ent != word_ent)
					xtext->jump_out_early = TRUE;

				gtk_xtext_render_ents (xtext, old_ent, word_ent, FALSE);

				xtext->jump_out_early = FALSE;
				xtext->skip_border_fills = FALSE;
				xtext->do_underline_fills_only = FALSE;
			}
			return FALSE;
		}
	}

	gtk_xtext_leave_notify (widget, NULL);

#endif

	return FALSE;
}

static void
gtk_xtext_set_clip_owner (GtkWidget * xtext, GdkEventButton * event)
{
	gtk_selection_owner_set (xtext, gdk_atom_intern ("CLIPBOARD", FALSE),
									 event->time);
	gtk_selection_owner_set (xtext, GDK_SELECTION_PRIMARY, event->time);
}

static gint
gtk_xtext_button_release (GtkWidget * widget, GdkEventButton * event)
{
	GtkXText *xtext = GTK_XTEXT (widget);
	unsigned char *word;

	if (xtext->moving_separator)
	{
		xtext->moving_separator = FALSE;
		if (event->x < (4 * widget->allocation.width) / 5 && event->x > 15)
		{
			xtext->indent = event->x;
		}
		gtk_xtext_fix_indent (xtext);
		gtk_xtext_recalc_widths (xtext, FALSE);
		gtk_xtext_adjustment_set (xtext, TRUE);
		gtk_xtext_render_page (xtext);
		return FALSE;
	}

	if (xtext->word_or_line_select)
	{
		xtext->word_or_line_select = FALSE;
		xtext->button_down = FALSE;
		return FALSE;
	}

	if (event->button == 1)
	{
		xtext->button_down = FALSE;

		gtk_grab_remove (widget);
		/*gdk_pointer_ungrab (0);*/
		if (xtext->last_ent_start)
			gtk_xtext_set_clip_owner (GTK_WIDGET (xtext), event);

		if (xtext->select_start_x == event->x &&
			 xtext->select_start_y == event->y)
		{
			if (gtk_xtext_selection_clear (xtext))
			{
				xtext->skip_border_fills = TRUE;
				gtk_xtext_render_ents (xtext, xtext->last_ent_start,
											  xtext->last_ent_end, TRUE);
				xtext->skip_border_fills = FALSE;

				xtext->last_ent_start = NULL;
				xtext->last_ent_end = NULL;
			}
		} else
		{
			word = gtk_xtext_get_word (xtext, event->x, event->y, 0, 0, 0);
			if (word)
			{
				gtk_signal_emit (GTK_OBJECT (xtext), xtext_signals[WORD_CLICK],
									  word, event);
				return FALSE;
			}
		}
	}

	return FALSE;
}

static gint
gtk_xtext_button_press (GtkWidget * widget, GdkEventButton * event)
{
	GtkXText *xtext = GTK_XTEXT (widget);
	textentry *ent;
	unsigned char *word;
	int line_x, x, y, offset, len;
	gfloat new_value;

	gdk_window_get_pointer (widget->window, &x, &y, 0);

	if (event->button == 3)		  /* right click */
	{
		word = gtk_xtext_get_word (xtext, x, y, 0, 0, 0);
		if (word)
		{
			gtk_signal_emit (GTK_OBJECT (xtext), xtext_signals[WORD_CLICK],
								  word, event);
		} else
			gtk_signal_emit (GTK_OBJECT (xtext), xtext_signals[WORD_CLICK],
								  "", event);
		return FALSE;
	}

	if (event->button == 4)		  /* mouse wheel pageUp */
	{
		new_value = xtext->adj->value - xtext->adj->page_increment;
		if (new_value < xtext->adj->lower)
			new_value = xtext->adj->lower;
		gtk_adjustment_set_value (xtext->adj, new_value);
		return FALSE;
	}

	if (event->button == 5)		  /* mouse wheel pageDn */
	{
		new_value = xtext->adj->value + xtext->adj->page_increment;
		if (new_value > (xtext->adj->upper - xtext->adj->page_size))
			new_value = xtext->adj->upper - xtext->adj->page_size;
		gtk_adjustment_set_value (xtext->adj, new_value);
		return FALSE;
	}

	if (event->button == 2)
	{
		gtk_signal_emit (GTK_OBJECT (xtext), xtext_signals[WORD_CLICK], "", event);
		return FALSE;
	}

	if (event->button != 1)		  /* we only want left button */
		return FALSE;

	if (event->type == GDK_2BUTTON_PRESS)	/* WORD select */
	{
		word = gtk_xtext_get_word (xtext, x, y, &ent, &offset, &len);
		if (word)
		{
			if (len == 0)
				return FALSE;
			gtk_xtext_selection_clear (xtext);
			ent->mark_start = offset;
			ent->mark_end = offset + len;
			/* clear the old mark, if any */
			xtext->skip_border_fills = TRUE;
			gtk_xtext_render_ents (xtext, xtext->last_ent_start, xtext->last_ent_end, TRUE);
			/* render the word select */
			xtext->last_ent_start = ent;
			xtext->last_ent_end = ent;
			gtk_xtext_render_ents (xtext, ent, NULL, FALSE);
			xtext->skip_border_fills = FALSE;
			xtext->word_or_line_select = TRUE;
			gtk_xtext_set_clip_owner (GTK_WIDGET (xtext), event);
		}

		return FALSE;
	}

	if (event->type == GDK_3BUTTON_PRESS)	/* LINE select */
	{
		word = gtk_xtext_get_word (xtext, x, y, &ent, 0, 0);
		if (word)
		{
			gtk_xtext_selection_clear (xtext);
			ent->mark_start = 0;
			ent->mark_end = ent->str_len;
			/* clear the old mark, if any */
			xtext->skip_border_fills = TRUE;
			gtk_xtext_render_ents (xtext, xtext->last_ent_start, xtext->last_ent_end, TRUE);
			/* render the line select */
			xtext->last_ent_start = ent;
			xtext->last_ent_end = ent;
			gtk_xtext_render_ents (xtext, ent, NULL, FALSE);
			xtext->skip_border_fills = FALSE;
			xtext->word_or_line_select = TRUE;
			gtk_xtext_set_clip_owner (GTK_WIDGET (xtext), event);
		}

		return FALSE;
	}

	/* check if it was a separator-bar click */
	if (xtext->separator && xtext->indent)
	{
		line_x = xtext->indent - ((xtext->space_width + 1) / 2);
		if (line_x == x || line_x == x + 1 || line_x == x - 1)
		{
			xtext->moving_separator = TRUE;
			gtk_xtext_render_page (xtext);
			return FALSE;
		}
	}

	xtext->button_down = TRUE;

	xtext->select_start_x = x;
	xtext->select_start_y = y;

	xtext->select_start_adj = xtext->adj->value;

	return FALSE;
}

/* another program has claimed the selection */

static gint
gtk_xtext_selection_kill (GtkWidget * widget, GdkEventSelection * event)
{
#ifndef WIN32
	if (gtk_xtext_selection_clear (GTK_XTEXT (widget)))
	{
		GTK_XTEXT (widget)->last_ent_start = NULL;
		GTK_XTEXT (widget)->last_ent_end = NULL;
		gtk_xtext_render_page (GTK_XTEXT (widget));
	}
#endif
	return TRUE;
}

/* another program is asking for our selection */

static void
gtk_xtext_selection_get (GtkWidget * widget,
								 GtkSelectionData * selection_data_ptr,
								 guint info, guint time)
{
	GtkXText *xtext = GTK_XTEXT (widget);
	textentry *ent;
	char *txt;
	char *pos;
	char *stripped;
	int len;
	int first = TRUE;

	/* first find out how much we need to malloc ... */
	len = 0;
	ent = xtext->last_ent_start;
	while (ent)
	{
		if (ent->mark_start != -1)
		{
			if (ent->mark_end - ent->mark_start > 0)
				len += (ent->mark_end - ent->mark_start) + 1;
			else
				len++;
		}
		if (ent == xtext->last_ent_end)
			break;
		ent = ent->next;
	}

	if (len < 1)
		return;

	/* now allocate mem and copy buffer */
	pos = txt = malloc (len);
	ent = xtext->last_ent_start;
	while (ent)
	{
		if (ent->mark_start != -1)
		{
			if (!first)
			{
				*pos = '\n';
				pos++;
			}
			first = FALSE;
			if (ent->mark_end - ent->mark_start > 0)
			{
				memcpy (pos, ent->str + ent->mark_start,
						  ent->mark_end - ent->mark_start);
				pos += ent->mark_end - ent->mark_start;
			}
		}
		if (ent == xtext->last_ent_end)
			break;
		ent = ent->next;
	}
	*pos = 0;

	if (xtext->color_paste)
		stripped = gtk_xtext_conv_color (txt, strlen (txt), NULL, NULL,
													xtext->fonttype);
	else
		stripped = gtk_xtext_strip_color (txt, strlen (txt), NULL, NULL,
													 xtext->fonttype, NULL);
	free (txt);

	if ((info == TARGET_TEXT) || (info == TARGET_COMPOUND_TEXT))
	{
		guchar *new_text;
		GdkAtom encoding;
		gint format;
		gint new_length;

		gdk_string_to_compound_text (stripped, &encoding, &format, &new_text,
											  &new_length);
		gtk_selection_data_set (selection_data_ptr, encoding, format,
										new_text, new_length);
		gdk_free_compound_text (new_text);
	} else
	{
#ifdef WIN32
		char *loc = g_locale_from_utf8 (stripped, -1, NULL, &len, NULL);
		gtk_selection_data_set (selection_data_ptr, GDK_SELECTION_TYPE_STRING,
										8, loc, len);
		g_free (loc);
#else
		gtk_selection_data_set (selection_data_ptr, GDK_SELECTION_TYPE_STRING,
										8, stripped, strlen (stripped));
#endif
	}

	free (stripped);
}

static void
gtk_xtext_class_init (GtkXTextClass * class)
{
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;
	GtkXTextClass *xtext_class;

	object_class = (GtkObjectClass *) class;
	widget_class = (GtkWidgetClass *) class;
	xtext_class = (GtkXTextClass *) class;

	parent_class = gtk_type_class (gtk_widget_get_type ());

	xtext_signals[WORD_CLICK] =
		gtk_signal_new (/*name*/"word_click",
							 /*GtkSignalRunType*/GTK_RUN_FIRST,
							 /*GtkType*/object_class->type,
							 /*funcoffset*/GTK_SIGNAL_OFFSET (GtkXTextClass, word_click),
							 /*GtkSignalMarshaller*/gtk_marshal_NONE__POINTER_POINTER,
							 /*returnval*/GTK_TYPE_NONE,
							 /*num args*/2, /*args*/GTK_TYPE_POINTER, GTK_TYPE_POINTER);
	gtk_object_class_add_signals (object_class, xtext_signals, LAST_SIGNAL);

	object_class->destroy = gtk_xtext_destroy;

	widget_class->realize = gtk_xtext_realize;
	widget_class->size_request = gtk_xtext_size_request;
	widget_class->size_allocate = gtk_xtext_size_allocate;
	widget_class->button_press_event = gtk_xtext_button_press;
	widget_class->button_release_event = gtk_xtext_button_release;
	widget_class->motion_notify_event = gtk_xtext_motion_notify;
#ifdef MOTION_MONITOR
	widget_class->leave_notify_event = gtk_xtext_leave_notify;
#endif
	widget_class->draw = gtk_xtext_draw;
	widget_class->expose_event = gtk_xtext_expose;

	xtext_class->word_click = NULL;
}

static guint
gtk_xtext_get_type ()
{
	static guint xtext_type = 0;

	if (!xtext_type)
	{
		GtkTypeInfo xtext_info = {
			"GtkXText",
			sizeof (GtkXText),
			sizeof (GtkXTextClass),
			(GtkClassInitFunc) gtk_xtext_class_init,
			(GtkObjectInitFunc) gtk_xtext_init,
			(GtkArgSetFunc) NULL,
			(GtkArgGetFunc) NULL,
		};

		xtext_type = gtk_type_unique (gtk_widget_get_type (), &xtext_info);
	}

	return xtext_type;
}

/*void
gtk_xtext_thaw (GtkXText *xtext)
{
   if (xtext->frozen > 0)
      xtext->frozen--;

   if (xtext->frozen == 0)
      gtk_xtext_render_page (xtext);
}

void
gtk_xtext_freeze (GtkXText *xtext)
{
   xtext->frozen++;
}*/

/* strip MIRC colors and other attribs. */

unsigned char *
gtk_xtext_strip_color (unsigned char *text, int len, unsigned char *outbuf,
							  int *newlen, int fonttype, int *mb_ret)
{
	int nc = 0;
	int i = 0;
	int col = FALSE;
	unsigned char *new_str;
	int mbl;
	int mb = FALSE;

	if (outbuf == NULL)
		new_str = malloc (len + 2);
	else
		new_str = outbuf;

	while (len > 0)
	{
		if ((col && isdigit (*text) && nc < 2) ||
			 (col && *text == ',' && isdigit (*(text+1)) && nc < 3))
		{
			nc++;
			if (*text == ',')
				nc = 0;
		} else
		{
			col = FALSE;
			switch (*text)
			{
			case ATTR_COLOR:
				col = TRUE;
				nc = 0;
				break;
			case ATTR_BEEP:
			case ATTR_RESET:
			case ATTR_REVERSE:
			case ATTR_BOLD:
			case ATTR_UNDERLINE:
				break;
			default:
				if (fonttype != FONT_SET)
					goto single;
				mbl = charlen (text);
				if (mbl == 1)
				{
single:			new_str[i] = *text;
					i++;
				} else if (mbl > 0)
				{
					mb = TRUE;
					memcpy (&new_str[i], text, mbl);
					i += mbl;
					text += mbl - 1; /* -1 -> text++ */
					len -= mbl - 1; /* -1 -> len-- */
				}
			}
		}
		text++;
		len--;
	}

	new_str[i] = 0;

	if (newlen != NULL)
		*newlen = i;

	if (mb_ret != NULL)
		*mb_ret = mb;

	return new_str;
}

/* GeEkMaN: converts mIRC control codes to literal control codes */

static char *
gtk_xtext_conv_color (unsigned char *text, int len, char *outbuf, int *newlen, int fonttype)
{
	int i = 0, j = 0;
	int ilen = len;
	char cchar = 0;
	char *new_str;
	int mbl;

	for (;len >= 0;len--)
	{
		switch (text[len])
		{
		case ATTR_COLOR:
		case ATTR_RESET:
		case ATTR_REVERSE:
		case ATTR_BOLD:
		case ATTR_UNDERLINE:
			j += 2;
		default:
			j++;
		}
	}

	len = ilen;
	ilen = j;

	if (outbuf == NULL)
		new_str = malloc (ilen + 2);
	else
		new_str = outbuf;

	while (len > 0)
	{
		switch (*text)
		{
		case ATTR_COLOR:
			cchar = 'C';
			break;
		case ATTR_RESET:
			cchar = 'O';
			break;
		case ATTR_REVERSE:
			cchar = 'R';
			break;
		case ATTR_BOLD:
			cchar = 'B';
			break;
		case ATTR_UNDERLINE:
			cchar = 'U';
			break;
		case ATTR_BEEP:
			break;
		default:
			if (fonttype != FONT_SET)
				goto single;
			mbl = charlen (text);
			if (mbl == 1)
			{
single:		new_str[i] = *text;
				i++;
			} else if (mbl > 0)
			{
				memcpy (&new_str[i], text, mbl);
				i += mbl;
				text += mbl - 1; /* -1 -> text++ */
				len -= mbl - 1; /* -1 -> len-- */
			}
		}
		if (cchar != 0) {
			new_str[i++] = '%';
			new_str[i++] = cchar;
			cchar = 0;
		}
		text++;
		len--;
	}

	new_str[i] = 0;

	if (newlen != NULL)
		*newlen = i;

	return new_str;
}

/* gives width of a 8bit string - with no mIRC codes in it */

static int
gtk_xtext_text_width_8bit (GtkXText * xtext, unsigned char *str, int len)
{
	int width = 0;

	while (len)
	{
		width += xtext->fontwidth[*str];
		str++;
		len--;
	}

	return width;
}

/* gives width of a string, excluding the mIRC codes */

static int
gtk_xtext_text_width (GtkXText * xtext, unsigned char *text, int len,
							 int *mb_ret)
{
	unsigned char *new_buf;
	int new_len, mb;

	new_buf = gtk_xtext_strip_color (text, len, xtext->scratch_buffer,
												&new_len, xtext->fonttype, &mb);

	if (mb_ret)
		*mb_ret = mb;

	if (xtext->fonttype == FONT_1BYTE || !mb)
		return gtk_xtext_text_width_8bit (xtext, new_buf, new_len);

	return gdk_text_width (xtext->font, new_buf, new_len);
}

/* actually draw text to screen */

static int
gtk_xtext_render_flush (GtkXText * xtext, int x, int y, unsigned char *str,
								int len, GdkGC *gc, int is_mb)
{
	int str_width;
	int dofill;
#ifdef USE_XLIB
	XFontStruct *xfont;
	GC xgc;
	Drawable xdraw_buf;
	Display *xdisplay;
#endif

	if (xtext->dont_render || len < 1)
		return 0;

	if (xtext->fonttype == FONT_1BYTE || !is_mb)
		str_width = gtk_xtext_text_width_8bit (xtext, str, len);
	else
		str_width = gdk_text_width (xtext->font, str, len);

	dofill = TRUE;

	/* backcolor is always handled by XDrawImageString */
	if (!xtext->backcolor)
	{
		if (xtext->skip_fills)
		{
			dofill = FALSE;
		} else if (xtext->do_underline_fills_only)
		{
			if (xtext->underline) /* don't bother drawing the text */
				goto dounder;
			/* redraw the background at the descent (e.g. redraws the
				background where the j's and y's end). This is used for
				unrendering a highlight. */
			gdk_draw_rectangle (GTK_WIDGET (xtext)->window, xtext->bgc, 1,
									  x, y + 1, str_width, 1);
			dofill = FALSE;
		} else if (xtext->pixmap)
		{
	/* draw the background pixmap behind the text - CAUSES FLICKER HERE!! */
			gdk_draw_rectangle (GTK_WIDGET (xtext)->window, xtext->bgc, 1,
									  x, y - xtext->font->ascent, str_width,
							 		  xtext->fontsize);
			dofill = FALSE;	/* non pixmap bg is done by XDrawImageString */
		}
	}

#ifdef USE_XLIB

	xgc = GDK_GC_XGC (gc);
	xdraw_buf = GDK_WINDOW_XWINDOW (xtext->draw_buf);
	xdisplay = GDK_WINDOW_XDISPLAY (GTK_WIDGET (xtext)->window);
	xfont = GDK_FONT_XFONT (xtext->font);

	switch (xtext->fonttype)
	{
	case FONT_1BYTE:
		if (dofill)
			XDrawImageString (xdisplay, xdraw_buf, xgc, x, y, str, len);
		else
			XDrawString (xdisplay, xdraw_buf, xgc, x, y, str, len);
		if (xtext->bold)
			XDrawString (xdisplay, xdraw_buf, xgc, x + 1, y, str, len);
		break;

	case FONT_2BYTE:
		len /= 2;
		if (dofill)
			XDrawImageString16 (xdisplay, xdraw_buf,
									  xgc, x, y, (XChar2b *) str, len);
		else
			XDrawString16 (xdisplay, xdraw_buf,
								xgc, x, y, (XChar2b *) str, len);
		if (xtext->bold)
			XDrawString16 (xdisplay, xdraw_buf,
								xgc, x + 1, y, (XChar2b *) str, len);
		break;

	case FONT_SET:
		if (dofill)
			XmbDrawImageString (xdisplay, xdraw_buf,
									  (XFontSet) xfont, xgc, x, y, str, len);
		else
			XmbDrawString (xdisplay, xdraw_buf,
								(XFontSet) xfont, xgc, x, y, str, len);
		if (xtext->bold)
			XmbDrawString (xdisplay, xdraw_buf,
								(XFontSet) xfont, xgc, x + 1, y, str, len);
	}

#else

	/* don't have Xlib, gdk version --- */
	if (dofill)
	{
		GdkGCValues val;
		gdk_gc_get_values (gc, &val);
		xtext_set_fg (gc, val.background.pixel);
		gdk_draw_rectangle (xtext->draw_buf, gc, 1,
								x, y - xtext->font->ascent, str_width,
							 	xtext->fontsize);
		xtext_set_fg (gc, val.foreground.pixel);
	}
	gdk_draw_text (xtext->draw_buf, xtext->font, gc, x, y, str, len);
	if (xtext->bold)
		gdk_draw_text (xtext->draw_buf, xtext->font, gc, x + 1, y, str, len);

#endif

	if (xtext->underline)
	{
dounder:
		gdk_draw_line (xtext->draw_buf, gc, x, y+1, x+str_width-1, y+1);
	}

	return str_width;
}

static void
gtk_xtext_reset (GtkXText * xtext, int mark, int attribs)
{
	if (attribs)
	{
		xtext->underline = FALSE;
		xtext->bold = FALSE;
	}
	if (!mark)
	{
		xtext->backcolor = FALSE;
		if (xtext->col_fore != 18)
			xtext_set_fg (xtext->fgc, xtext->palette[18]);
		if (xtext->col_back != 19)
			xtext_set_bg (xtext->fgc, xtext->palette[19]);
	}
	xtext->col_fore = 18;
	xtext->col_back = 19;
	xtext->parsing_color = FALSE;
	xtext->parsing_backcolor = FALSE;
	xtext->nc = 0;
}

/* render a single line, which WONT wrap, and parse mIRC colors */

static int
gtk_xtext_render_str (GtkXText * xtext, int y, textentry * ent, unsigned char *str,
							 int len, int win_width, int indent, int line)
{
	GdkGC *gc;
	int i = 0, x = indent, j = 0;
	unsigned char *pstr = str;
	int col_num, tmp;
	int offset;
	int mark = FALSE;
	int hilight = FALSE;
	int ret = 1;

	offset = str - ent->str;

	if (line < 255 && line >= 0)
		xtext->grid_offset[line] = offset;

	gc = xtext->fgc;				  /* our foreground GC */

	if (ent->mark_start != -1 &&
		 ent->mark_start <= i + offset && ent->mark_end > i + offset)
	{
		xtext_set_bg (gc, xtext->palette[16]);
		xtext_set_fg (gc, xtext->palette[17]);
		xtext->backcolor = TRUE;
		mark = TRUE;
	}
#ifdef MOTION_MONITOR
	if (xtext->hilight_ent == ent &&
		 xtext->hilight_start <= i + offset && xtext->hilight_end > i + offset)
	{
		xtext->underline = TRUE;
		hilight = TRUE;
	}
#endif

	if (!xtext->skip_border_fills)
	{
		/* draw background to the left of the text */
		if (str == ent->str && indent && xtext->time_stamp)
		{
			/* don't overwrite the timestamp */
			if (indent > xtext->stamp_width)
			{
				gdk_draw_rectangle (xtext->draw_buf, xtext->bgc, 1, 
								 xtext->stamp_width, y - xtext->font->ascent,
								 indent - xtext->stamp_width, xtext->fontsize);
			}
		} else
		{
			/* fill the indent area with background gc */
			gdk_draw_rectangle (xtext->draw_buf, xtext->bgc, 1, 0,
									y - xtext->font->ascent, indent, xtext->fontsize);
		}
	}

	while (i < len)
	{

#ifdef MOTION_MONITOR
		if (xtext->hilight_ent == ent && xtext->hilight_start == (i + offset))
		{
			x += gtk_xtext_render_flush (xtext, x, y, pstr, j, gc, ent->mb);
			pstr += j;
			j = 0;
			xtext->underline = TRUE;
			hilight = TRUE;
		}
#endif

		if (!mark && ent->mark_start == (i + offset))
		{
			x += gtk_xtext_render_flush (xtext, x, y, pstr, j, gc, ent->mb);
			pstr += j;
			j = 0;
			xtext_set_bg (gc, xtext->palette[16]);
			xtext_set_fg (gc, xtext->palette[17]);
			xtext->backcolor = TRUE;
			mark = TRUE;
		}

		if ((xtext->parsing_color && isdigit (str[i]) && xtext->nc < 2) ||
			 (xtext->parsing_color && str[i] == ',' && isdigit (str[i+1]) && xtext->nc < 3))
		{
			pstr++;
			if (str[i] == ',')
			{
				xtext->parsing_backcolor = TRUE;
				if (xtext->nc)
				{
					xtext->num[xtext->nc] = 0;
					xtext->nc = 0;
					col_num = atoi (xtext->num) % 16;
					xtext->col_fore = col_num;
					if (!mark)
						xtext_set_fg (gc, xtext->palette[col_num]);
				}
			} else
			{
				xtext->num[xtext->nc] = str[i];
				if (xtext->nc < 7)
					xtext->nc++;
			}
		} else
		{
			if (xtext->parsing_color)
			{
				xtext->parsing_color = FALSE;
				if (xtext->nc)
				{
					xtext->num[xtext->nc] = 0;
					xtext->nc = 0;
					col_num = atoi (xtext->num);
					if (col_num == 99)	/* mIRC lameness */
						col_num = 19;
					else
						col_num = col_num % 16;
					if (xtext->parsing_backcolor)
					{
						if (col_num == 19)
							xtext->backcolor = FALSE;
						else
							xtext->backcolor = TRUE;
						if (!mark)
							xtext_set_bg (gc, xtext->palette[col_num]);
						xtext->col_back = col_num;
					} else
					{
						if (!mark)
							xtext_set_fg (gc, xtext->palette[col_num]);
						xtext->col_fore = col_num;
					}
					xtext->parsing_backcolor = FALSE;
				} else
				{
					/* got a \003<non-digit>... i.e. reset colors */
					x += gtk_xtext_render_flush (xtext, x, y, pstr, j, gc, ent->mb);
					pstr += j;
					j = 0;
					gtk_xtext_reset (xtext, mark, FALSE);
				}
			}

			switch (str[i])
			{
			/* FIXME for non-fixed width fonts. \t may not match ' ' width */
			case '\t':
				str[i] = ' ';
				j++;
				break;
			case '\n':
			/*case ATTR_BEEP:*/
				break;
			case ATTR_REVERSE:
				x += gtk_xtext_render_flush (xtext, x, y, pstr, j, gc, ent->mb);
				pstr += j + 1;
				j = 0;
				tmp = xtext->col_fore;
				xtext->col_fore = xtext->col_back;
				xtext->col_back = tmp;
				if (!mark)
				{
					xtext_set_fg (gc, xtext->palette[xtext->col_fore]);
					xtext_set_bg (gc, xtext->palette[xtext->col_back]);
				}
				if (xtext->col_back != 19)
					xtext->backcolor = TRUE;
				else
					xtext->backcolor = FALSE;
				break;
			case ATTR_BOLD:
				x += gtk_xtext_render_flush (xtext, x, y, pstr, j, gc, ent->mb);
				xtext->bold = !xtext->bold;
				pstr += j + 1;
				j = 0;
				break;
			case ATTR_UNDERLINE:
				x += gtk_xtext_render_flush (xtext, x, y, pstr, j, gc, ent->mb);
				xtext->underline = !xtext->underline;
				pstr += j + 1;
				j = 0;
				break;
			case ATTR_RESET:
				x += gtk_xtext_render_flush (xtext, x, y, pstr, j, gc, ent->mb);
				pstr += j + 1;
				j = 0;
				gtk_xtext_reset (xtext, mark, !hilight);
				break;
			case ATTR_COLOR:
				x += gtk_xtext_render_flush (xtext, x, y, pstr, j, gc, ent->mb);
				xtext->parsing_color = TRUE;
				pstr += j + 1;
				j = 0;
				break;
			default:
				j++;
			}
		}
		i++;

#ifdef MOTION_MONITOR
		if (xtext->hilight_ent == ent && xtext->hilight_end == (i + offset))
		{
			x += gtk_xtext_render_flush (xtext, x, y, pstr, j, gc, ent->mb);
			pstr += j;
			j = 0;
			xtext->underline = FALSE;
			if (xtext->jump_out_early)
			{
				/* stop drawing this ent */
				ret = 0;
				break;
			}
			hilight = FALSE;
		}
#endif

		if (mark && ent->mark_end == (i + offset))
		{
			x += gtk_xtext_render_flush (xtext, x, y, pstr, j, gc, ent->mb);
			pstr += j;
			j = 0;
			xtext_set_bg (gc, xtext->palette[xtext->col_back]);
			xtext_set_fg (gc, xtext->palette[xtext->col_fore]);
			if (xtext->col_back != 19)
				xtext->backcolor = TRUE;
			else
				xtext->backcolor = FALSE;
			mark = FALSE;
		}
	}

	if (j)
		x += gtk_xtext_render_flush (xtext, x, y, pstr, j, gc, ent->mb);

	if (mark)
	{
		xtext_set_bg (gc, xtext->palette[xtext->col_back]);
		xtext_set_fg (gc, xtext->palette[xtext->col_fore]);
		if (xtext->col_back != 19)
			xtext->backcolor = TRUE;
		else
			xtext->backcolor = FALSE;
	}

	/* draw separator now so it doesn't appear to flicker */
	gtk_xtext_draw_sep (xtext, y - xtext->font->ascent);
	/* draw background to the right of the text */
	if (!xtext->skip_border_fills)
		gdk_draw_rectangle (xtext->draw_buf, xtext->bgc, 1, 
						 x, y - xtext->font->ascent, (win_width + MARGIN) - x,
						 xtext->fontsize);

	return ret;
}

#ifdef USE_XLIB

/* get the desktop/root window - thanks Eterm */

static Window desktop_window = None;

static Window
get_desktop_window (Window the_window)
{
	Atom prop, type, prop2;
	int format;
	unsigned long length, after;
	unsigned char *data;
	unsigned int nchildren;
	Window w, root, *children, parent;

	prop = XInternAtom (GDK_DISPLAY (), "_XROOTPMAP_ID", True);
	prop2 = XInternAtom (GDK_DISPLAY (), "_XROOTCOLOR_PIXEL", True);

	if (prop == None && prop2 == None)
		return None;

	for (w = the_window; w; w = parent)
	{
		if ((XQueryTree (GDK_DISPLAY (), w, &root, &parent, &children,
				&nchildren)) == False)
			return None;

		if (nchildren)
			XFree (children);

		if (prop != None)
		{
			XGetWindowProperty (GDK_DISPLAY (), w, prop, 0L, 1L, False,
									  AnyPropertyType, &type, &format, &length, &after,
									  &data);
		} else
		{
			XGetWindowProperty (GDK_DISPLAY (), w, prop2, 0L, 1L, False,
									  AnyPropertyType, &type, &format, &length, &after,
									  &data);
		}

		if (data)
			XFree (data);

		if (type != None)
		{
			return (desktop_window = w);
		}
	}

	return (desktop_window = None);
}

/* stolen from zvt, which was stolen from Eterm */

static Pixmap
get_pixmap_prop (Window the_window)
{
	Atom type;
	int format;
	unsigned long length, after;
	unsigned char *data;
	Pixmap pix = None;
	static Atom prop = None;

	if (desktop_window == None)
		desktop_window = get_desktop_window (the_window);
	if (desktop_window == None)
		desktop_window = GDK_ROOT_WINDOW ();

	if (prop == None)
		prop = XInternAtom (GDK_DISPLAY (), "_XROOTPMAP_ID", True);
	if (prop == None)
		return None;

	XGetWindowProperty (GDK_DISPLAY (), desktop_window, prop, 0L, 1L, False,
							  AnyPropertyType, &type, &format, &length, &after,
							  &data);
	if (data)
	{
		if (type == XA_PIXMAP)
			pix = *((Pixmap *) data);

		XFree (data);
	}

	return pix;
}

#ifdef USE_MMX

#include "mmx_cmod.h"

static GdkPixmap *
shade_pixmap_mmx (GtkXText * xtext, Pixmap p, int x, int y, int w, int h)
{
	int dummy, width, height, depth;
	GdkPixmap *shaded_pix;
	Window root;
	Pixmap tmp;
	XImage *ximg;
	XGCValues gcv;
	GC tgc;

	XGetGeometry (GDK_DISPLAY (), p, &root, &dummy, &dummy, &width, &height,
					  &dummy, &depth);

	if (width < x + w || height < y + h || x < 0 || y < 0)
	{
		gcv.subwindow_mode = IncludeInferiors;
		gcv.graphics_exposures = False;
		tgc = XCreateGC (GDK_DISPLAY (), p, GCGraphicsExposures|GCSubwindowMode,
							  &gcv);
		tmp = XCreatePixmap (GDK_DISPLAY (), p, w, h, depth);
		XSetTile (GDK_DISPLAY (), tgc, p);
		XSetFillStyle (GDK_DISPLAY (), tgc, FillTiled);
		XSetTSOrigin (GDK_DISPLAY (), tgc, -x, -y);
		XFillRectangle (GDK_DISPLAY (), tmp, tgc, 0, 0, w, h);
		XFreeGC (GDK_DISPLAY (), tgc);

		ximg = XGetImage (GDK_DISPLAY (), tmp, 0, 0, w, h, -1, ZPixmap);
		XFreePixmap (GDK_DISPLAY (), tmp);
	} else
	{
		ximg = XGetImage (GDK_DISPLAY (), p, x, y, w, h, -1, ZPixmap);
	}

	switch (depth)
	{
	/* 1 and 8 not supported */
	case 15:
		shade_ximage_15_mmx (ximg->data, ximg->bytes_per_line, w, h,
						xtext->tint_red, xtext->tint_green, xtext->tint_blue);
		break;
	case 16:
		shade_ximage_16_mmx (ximg->data, ximg->bytes_per_line, w, h,
						xtext->tint_red, xtext->tint_green, xtext->tint_blue);
		break;
	case 24:
		if (ximg->bits_per_pixel != 32)
			break;
	case 32:
		shade_ximage_32_mmx (ximg->data, ximg->bytes_per_line, w, h,
						xtext->tint_red, xtext->tint_green, xtext->tint_blue);
		break;
	}

	if (xtext->recycle)
		shaded_pix = xtext->pixmap;
	else
		shaded_pix = gdk_pixmap_new (GTK_WIDGET (xtext)->window, w, h, depth);

	XPutImage (GDK_DISPLAY (), GDK_WINDOW_XWINDOW (shaded_pix),
				  GDK_GC_XGC (xtext->fgc), ximg, 0, 0, 0, 0, w, h);

	XDestroyImage (ximg);

	return shaded_pix;
}

#endif	/* !USE_MMX */

#ifdef USE_GDK_PIXBUF

static GdkPixmap *
shade_pixmap_gdk (GtkXText * xtext, Pixmap p, int x, int y, int w, int h)
{
	GdkPixmap *pp, *tmp, *shaded_pixmap;
	GdkPixbuf *pixbuf;
	GdkColormap *cmap;
	GdkGC *tgc;
	unsigned char *buf;
	unsigned char *pbuf;
	int width, height, depth;
	int rowstride;
	int pbwidth;
	int pbheight;
	int i, j;
	int offset;
	int r, g, b, a;

	pp = gdk_pixmap_foreign_new (p);
	cmap = gtk_widget_get_colormap (GTK_WIDGET (xtext));
	gdk_window_get_geometry (pp, NULL, NULL, &width, &height, &depth);

	if (width < x + w || height < y + h || x < 0 || y < 0)
	{
		tgc = gdk_gc_new (pp);
		tmp = gdk_pixmap_new (pp, w, h, depth);
		gdk_gc_set_tile (tgc, pp);
		gdk_gc_set_fill (tgc, GDK_TILED);
		gdk_gc_set_ts_origin (tgc, -x, -y);
		gdk_draw_rectangle (tmp, tgc, TRUE, 0, 0, w, h);
		gdk_gc_destroy (tgc);

		pixbuf = gdk_pixbuf_get_from_drawable (NULL, tmp, cmap,
															0, 0, 0, 0, w, h);
		gdk_pixmap_unref (tmp);
	} else
	{
		pixbuf = gdk_pixbuf_get_from_drawable (NULL, pp, cmap,
															x, y, 0, 0, w, h);
	}
	gdk_xid_table_remove (GDK_WINDOW_XWINDOW (pp));
	g_dataset_destroy (pp);
	g_free (pp);

	if (!pixbuf)
		return NULL;

	buf = gdk_pixbuf_get_pixels (pixbuf);
	rowstride = gdk_pixbuf_get_rowstride (pixbuf);
	pbwidth = gdk_pixbuf_get_width (pixbuf);
	pbheight = gdk_pixbuf_get_height (pixbuf);

	a = 128;	/* alpha */
	r = xtext->tint_red;
	g = xtext->tint_green;
	b = xtext->tint_blue;

	if (gdk_pixbuf_get_has_alpha (pixbuf))
		offset = 4;
	else
		offset = 3;

	for (i=0;i<pbheight;i++)
	{
		pbuf = buf;
		for (j=0;j<pbwidth;j++)
		{
			pbuf[0] = ((pbuf[0] * r) >> 8);
			pbuf[1] = ((pbuf[1] * g) >> 8);
			pbuf[2] = ((pbuf[2] * b) >> 8);
			pbuf+=offset;
		}
		buf+=rowstride;
	}

	/* reuse the same pixmap to save a few cycles */
	if (xtext->recycle)
	{
		shaded_pixmap = xtext->pixmap;
		gdk_pixbuf_render_to_drawable (pixbuf, shaded_pixmap, xtext->fgc, 0, 0,
												 0, 0, w, h, GDK_RGB_DITHER_NORMAL, 0, 0);
	} else
	{
		gdk_pixbuf_render_pixmap_and_mask (pixbuf, &shaded_pixmap, NULL, 0);
	}
	gdk_pixbuf_unref (pixbuf);

	return shaded_pixmap;
}

#endif /* !USE_GDK_PIXBUF */

#if defined(USE_GDK_PIXBUF) || defined(USE_MMX)

static GdkPixmap *
shade_pixmap (GtkXText * xtext, Pixmap p, int x, int y, int w, int h)
{
#ifdef USE_MMX

	if (have_mmx () && xtext->depth != 8)
		return shade_pixmap_mmx (xtext, p, x, y, w, h);

#ifdef USE_GDK_PIXBUF
	return shade_pixmap_gdk (xtext, p, x, y, w, h);
#else
	return NULL;
#endif

#else

	return shade_pixmap_gdk (xtext, p, x, y, w, h);

#endif /* !USE_MMX */
}

#endif /* !USE_TINT */

/* free transparency xtext->pixmap */

static void
gtk_xtext_free_trans (GtkXText * xtext)
{
	if (xtext->pixmap)
	{
		if (xtext->shaded)
		{
			gdk_pixmap_unref (xtext->pixmap);
		} else
		{
			gdk_xid_table_remove (GDK_WINDOW_XWINDOW (xtext->pixmap));
			g_dataset_destroy (xtext->pixmap);
			g_free (xtext->pixmap);
		}
		xtext->pixmap = NULL;
	}
}

/* grab pixmap from root window and set xtext->pixmap */

static void
gtk_xtext_load_trans (GtkXText * xtext)
{
	Pixmap rootpix;
	GtkWidget *widget = GTK_WIDGET (xtext);
	int x, y;

	rootpix = get_pixmap_prop (GDK_WINDOW_XWINDOW (widget->window));
	if (rootpix == None)
	{
		if (xtext->error_function)
			xtext->error_function ("Unable to get root window pixmap!\n\n"
										  "You may need to use Esetroot or Gnome\n"
										  "control-center to set your background.\n");
		xtext->transparent = FALSE;
		return;
	}

	gdk_window_get_origin (widget->window, &x, &y);

#if defined(USE_GDK_PIXBUF) || defined(USE_MMX)
	if (xtext->shaded)
	{
		int width, height;
		gdk_window_get_size (GTK_WIDGET (xtext)->window, &width, &height);
		xtext->pixmap = shade_pixmap (xtext, rootpix, x, y, width, height);
		if (xtext->pixmap == NULL)
		{
			xtext->shaded = 0;
			goto noshade;
		}
		gdk_gc_set_tile (xtext->bgc, xtext->pixmap);
		gdk_gc_set_ts_origin (xtext->bgc, 0, 0);
	} else
	{
#endif
noshade:
		xtext->pixmap = gdk_pixmap_foreign_new (rootpix);
		gdk_gc_set_tile (xtext->bgc, xtext->pixmap);
		gdk_gc_set_ts_origin (xtext->bgc, -x, -y);
#if defined(USE_GDK_PIXBUF) || defined(USE_MMX)
	}
#endif
	gdk_gc_set_fill (xtext->bgc, GDK_TILED);
}

#endif

/* walk through str until this line doesn't fit anymore */

static int
find_next_wrap (GtkXText * xtext, textentry * ent, unsigned char *str,
					 int win_width, int indent)
{
	unsigned char *last_space = str;
	unsigned char *orig_str = str;
	int str_width = indent;
	int col = FALSE;
	int nc = 0;
	int mbl;
	int ret;
	int limit_offset = 0;

	/* single liners */
	if (win_width >= ent->str_width + ent->indent)
		return ent->str_len;

	/* it does happen! */
	if (win_width < 1)
	{
		ret = ent->str_len - (str - ent->str);
		goto done;
	}

	while (1)
	{
		if ((col && isdigit (*str) && nc < 2) ||
			 (col && *str == ',' && isdigit (*(str+1)) && nc < 3))
		{
			nc++;
			if (*str == ',')
				nc = 0;
			limit_offset++;
			str++;
		} else
		{
			col = FALSE;
			switch (*str)
			{
			case ATTR_COLOR:
				col = TRUE;
				nc = 0;
			case ATTR_BEEP:
			case ATTR_RESET:
			case ATTR_REVERSE:
			case ATTR_BOLD:
			case ATTR_UNDERLINE:
				limit_offset++;
				str++;
				break;
			default:
				str_width += get_char_width (xtext, str, &mbl, ent->mb);
				if (str_width > win_width)
				{
					if (xtext->wordwrap)
					{
						if (str - last_space > WORDWRAP_LIMIT + limit_offset)
							ret = str - orig_str; /* fall back to character wrap */
						else
						{
							if (*last_space == ' ')
								last_space++;
							ret = last_space - orig_str;
							if (ret == 0) /* fall back to character wrap */
								ret = str - orig_str;
						}
						goto done;
					}
					ret = str - orig_str;
					goto done;
				}

				/* keep a record of the last space, for wordwrapping */
				if (is_del (*str))
				{
					last_space = str;
					limit_offset = 0;
				}

				/* progress to the next char */
				str += mbl;

			}
		}

		if (str >= ent->str + ent->str_len)
		{
			ret = str - orig_str;
			goto done;
		}
	}

done:

	/* must make progress */
	if (ret < 1)
		ret = 1;

	return ret;
}

/* render a single line, which may wrap to more lines */

static int
gtk_xtext_render_line (GtkXText * xtext, textentry * ent, int line,
							  int lines_max, int subline, int win_width)
{
	unsigned char *str;
	int indent, taken, len, y;

	taken = 0;
	str = ent->str;
	indent = ent->indent;

#ifdef XCHAT
	/* draw the timestamp */
	if (xtext->time_stamp && !xtext->skip_stamp)
	{
		char time_str[64];
		int stamp_size = get_stamp_str (ent->stamp, time_str, sizeof (time_str));
		y = (xtext->fontsize * line) + xtext->font->ascent - xtext->pixel_offset;
		gtk_xtext_render_str (xtext, y, ent, time_str, stamp_size,
									 win_width, 2, line);
	}
#endif

	/* draw each line one by one */
	do
	{
		len = find_next_wrap (xtext, ent, str, win_width, indent);

		y = (xtext->fontsize * line) + xtext->font->ascent - xtext->pixel_offset;
		if (!subline)
		{
			if (!gtk_xtext_render_str (xtext, y, ent, str, len, win_width,
												indent, line))
			{
				/* small optimization */
				return ent->lines_taken - subline;
			}
		} else
		{
			xtext->dont_render = TRUE;
			gtk_xtext_render_str (xtext, y, ent, str, len, win_width,
										 indent, line);
			xtext->dont_render = FALSE;
			subline--;
			line--;
			taken--;
		}

		indent = xtext->indent;
		line++;
		taken++;
		str += len;

		if (line >= lines_max)
			break;

	}
	while (str < ent->str + ent->str_len);

	return taken;
}

void
gtk_xtext_set_palette (GtkXText * xtext, GdkColor palette[])
{
	int i;

	for (i = 0; i < 20; i++)
		xtext->palette[i] = palette[i].pixel;

	if (GTK_WIDGET_REALIZED (xtext))
	{
		xtext_set_fg (xtext->fgc, xtext->palette[18]);
		xtext_set_bg (xtext->fgc, xtext->palette[19]);
		xtext_set_fg (xtext->bgc, xtext->palette[19]);
	}
	xtext->col_fore = 18;
	xtext->col_back = 19;
}

static void
gtk_xtext_fix_indent (GtkXText * xtext)
{
	int j;

	/* make indent a multiple of the space width */
	if (xtext->indent && xtext->space_width)
	{
		j = 0;
		while (j < xtext->indent)
		{
			j += xtext->space_width;
		}
		xtext->indent = j;
	}
}

static void
gtk_xtext_recalc_widths (GtkXText * xtext, int do_str_width)
{
	textentry *ent;

	/* since we have a new font, we have to recalc the text widths */
	ent = xtext->text_first;
	while (ent)
	{
		if (do_str_width)
		{
			ent->str_width = gtk_xtext_text_width (xtext, ent->str, ent->str_len,
																NULL);
		}
		if (ent->left_len != -1)
		{
			ent->indent =
				(xtext->indent -
				 gtk_xtext_text_width (xtext, ent->str,
											  ent->left_len, NULL)) - xtext->space_width;
			if (ent->indent < MARGIN)
				ent->indent = MARGIN;
		}
		ent = ent->next;
	}

	gtk_xtext_calc_lines (xtext, FALSE);
}

void
gtk_xtext_set_font (GtkXText * xtext, GdkFont * font, char *name)
{
	int i;
#ifdef WIN32
	GdkWChar c;
#else
	unsigned char c;
#endif
#ifdef USE_XLIB
	XFontStruct *xfont;
#endif

	if (xtext->font)
		gdk_font_unref (xtext->font);

	if (font)
	{
		xtext->font = font;
		gdk_font_ref (font);
	} else
		font = xtext->font = gdk_font_load (name);

	if (!font)
		font = xtext->font = gdk_font_load ("fixed");

	switch (font->type)
	{
	case GDK_FONT_FONT:
			xtext->fontsize = font->ascent + font->descent;
#ifdef USE_XLIB
			xfont = GDK_FONT_XFONT (font);
			if ((xfont->min_byte1 == 0) && (xfont->max_byte1 == 0))
				xtext->fonttype = FONT_1BYTE;
			else
				xtext->fonttype = FONT_2BYTE;
#else
			/* without X11 we don't support 16bit fonts */
			xtext->fonttype = FONT_1BYTE;
#endif
			break;

	case GDK_FONT_FONTSET:
#ifdef WIN32
			xtext->fontsize = font->ascent + font->descent;
#else
			xtext->fontsize = gdk_text_height (font, " ", 1);
#endif
			xtext->fonttype = FONT_SET;
			break;
	}

	/* measure the width of every char */
	for (i = 0; i < 256; i++)
	{
		c = i;
#ifdef WIN32
		xtext->fontwidth[i] = gdk_text_width_wc (font, &c, 1);
#else
		xtext->fontwidth[i] = gdk_text_width (font, &c, 1);
#endif
	}
	xtext->space_width = xtext->fontwidth[' '];

#ifdef XCHAT
	{
		char time_str[64];
		int stamp_size = get_stamp_str (time(0), time_str, sizeof (time_str));
		xtext->stamp_width =
			gtk_xtext_text_width (xtext, time_str, stamp_size, NULL) + MARGIN;
	}
#endif

	gtk_xtext_fix_indent (xtext);

	if (GTK_WIDGET_REALIZED (xtext))
	{
		if (xtext->fonttype != FONT_SET)
			gdk_gc_set_font (xtext->fgc, xtext->font);

		gtk_xtext_recalc_widths (xtext, TRUE);
	}
}

void
gtk_xtext_set_background (GtkXText * xtext, GdkPixmap * pixmap, int trans,
								  int shaded)
{
	GdkGCValues val;

#if !defined(USE_GDK_PIXBUF) && !defined(USE_MMX)
	shaded = FALSE;
#endif

#ifndef USE_XLIB
	shaded = FALSE;
	trans = FALSE;
#endif

	if (xtext->pixmap)
	{
#ifdef USE_XLIB
		if (xtext->transparent)
			gtk_xtext_free_trans (xtext);
		else
#endif
			gdk_pixmap_unref (xtext->pixmap);
		xtext->pixmap = NULL;
	}

	xtext->transparent = trans;

#ifdef USE_XLIB
	if (trans)
	{
		xtext->shaded = shaded;
		if (GTK_WIDGET_REALIZED (xtext))
			gtk_xtext_load_trans (xtext);
		return;
	}
#endif

	xtext->pixmap = pixmap;

	if (pixmap != 0)
	{
		gdk_pixmap_ref (pixmap);
		if (GTK_WIDGET_REALIZED (xtext))
		{
			gdk_gc_set_tile (xtext->bgc, pixmap);
			gdk_gc_set_ts_origin (xtext->bgc, 0, 0);
			gdk_gc_set_fill (xtext->bgc, GDK_TILED);
		}
	} else
	{
		if (GTK_WIDGET_REALIZED (xtext))
		{
			gdk_gc_destroy (xtext->bgc);
			val.subwindow_mode = GDK_INCLUDE_INFERIORS;
			val.graphics_exposures = 0;
			xtext->bgc = gdk_gc_new_with_values (GTK_WIDGET (xtext)->window,
									&val, GDK_GC_EXPOSURES | GDK_GC_SUBWINDOW);
			xtext_set_fg (xtext->bgc, xtext->palette[19]);
		}
	}
}

#ifndef XCHAT

gchar *
gtk_xtext_get_chars (GtkXText * xtext)
{
	int lenght = 0;
	gchar *chars;
	textentry *tentry = xtext->text_first;
	while (tentry != NULL)
	{
		lenght += tentry->str_len + 1;
		tentry = tentry->next;
	}
	if (lenght == 0)
		return NULL;
	chars = g_malloc (lenght + 1);
	*chars = 0;

	tentry = xtext->text_first;
	while (tentry != NULL)
	{
		strcat (chars, tentry->str);
		strcat (chars, "\n");
		tentry = tentry->next;
	}

	return chars;
}

#endif

/* count how many lines 'ent' will take (with wraps) */

static int
gtk_xtext_lines_taken (GtkXText * xtext, textentry * ent)
{
	unsigned char *str;
	int indent, taken, len;
	int win_width;

	gdk_window_get_size (GTK_WIDGET (xtext)->window, &win_width, NULL);
	win_width -= MARGIN;

	if (ent->str_width + ent->indent < win_width)
		return 1;

	indent = ent->indent;
	str = ent->str;
	taken = 0;

	do
	{
		len = find_next_wrap (xtext, ent, str, win_width, indent);
		indent = xtext->indent;
		taken++;
		str += len;
	}
	while (str < ent->str + ent->str_len);

	return taken;
}

/* Calculate number of actual lines (with wraps), to set adj->lower. *
 * This should only be called when the window resizes.               */

static void
gtk_xtext_calc_lines (GtkXText * xtext, int fire_signal)
{
	textentry *ent;
	int width;
	int height;
	int lines;

	gdk_window_get_size (GTK_WIDGET (xtext)->window, &width, &height);
	width -= MARGIN;

	if (width < 30 || height < xtext->fontsize || width < xtext->indent + 30)
		return;

	lines = 0;
	ent = xtext->text_first;
	while (ent)
	{
		ent->lines_taken = gtk_xtext_lines_taken (xtext, ent);
		lines += ent->lines_taken;
		ent = ent->next;
	}

	xtext->pagetop_ent = NULL;
	xtext->num_lines = lines;
	gtk_xtext_adjustment_set (xtext, fire_signal);
}

/* find the n-th line in the linked list, this includes wrap calculations */

static textentry *
gtk_xtext_nth (GtkXText * xtext, textentry * ent, int line, int *subline)
{
	int lines = 0;

	if (ent == NULL)
	{
		ent = xtext->text_first;
		line += xtext->adj->value;
	} else
	{
		/* -- optimization -- */
		/* try to make a short-cut using the pagetop ent */
		if (ent != xtext->pagetop_ent && xtext->pagetop_ent)
		{
			if (line == xtext->pagetop_line)
			{
				*subline = xtext->pagetop_subline;
				return xtext->pagetop_ent;
			}
			if (line > xtext->pagetop_line && ent == xtext->text_first)
			{
				/* lets start from the pagetop instead of the absolute beginning */
				ent = xtext->pagetop_ent;
				lines = xtext->pagetop_line - xtext->pagetop_subline;
			}
		}
		/* -- end of optimization -- */
		lines -= *subline;
	}

	while (ent)
	{
		lines += ent->lines_taken;
		if (lines > line)
		{
			*subline = ent->lines_taken - (lines - line);
			return ent;
		}
		ent = ent->next;
	}
	return 0;
}

static void
gtk_xtext_draw_sep (GtkXText * xtext, int y)
{
	int x, height;
	GdkGC *light, *dark;

	if (y == -1)
	{
		y = 0;
		height = GTK_WIDGET (xtext)->allocation.height;
	} else
	{
		height = xtext->fontsize;
	}

	/* draw the separator line */
	if (xtext->separator && xtext->indent)
	{
		light = xtext->light_gc;
		dark = xtext->dark_gc;

		x = xtext->indent - ((xtext->space_width + 1) / 2);
		if (x < 1)
			return;

		if (xtext->thinline)
		{
			if (xtext->moving_separator)
				gdk_draw_line (xtext->draw_buf, light, x, y, x, y + height);
			else
				gdk_draw_line (xtext->draw_buf, dark, x, y, x, y + height);
		} else
		{
			if (xtext->moving_separator)
			{
				gdk_draw_line (xtext->draw_buf, light, x - 1, y, x - 1, y + height);
				gdk_draw_line (xtext->draw_buf, dark, x, y, x, y + height);
			} else
			{
				gdk_draw_line (xtext->draw_buf, dark, x - 1, y, x - 1, y + height);
				gdk_draw_line (xtext->draw_buf, light, x, y, x, y + height);
			}
		}
	}
}

/* render 2 ents (or an inclusive range) */

static void
gtk_xtext_render_ents (GtkXText * xtext, textentry * enta, textentry * entb,
							  int inclusive)
{
	textentry *ent, *orig_ent, *tmp_ent;
	int line;
	int lines_max;
	int width;
	int height;
	int subline;
	int drawing = FALSE;

	if (xtext->indent < MARGIN)
		xtext->indent = MARGIN;	  /* 2 pixels is our left margin */

	gdk_window_get_size (GTK_WIDGET (xtext)->window, &width, &height);
	width -= MARGIN;

	if (width < 32 || height < xtext->fontsize || width < xtext->indent + 30)
		return;

#ifdef SMOOTH_SCROLL
	lines_max = (height - xtext->font->descent + xtext->pixel_offset) / xtext->fontsize + 1;
#else
	lines_max = (height - xtext->font->descent) / xtext->fontsize;
#endif
	line = 0;
	orig_ent = xtext->pagetop_ent;
	subline = xtext->pagetop_subline;

	/* check if enta is before the start of this page */
	if (inclusive)
	{
		tmp_ent = orig_ent;
		while (tmp_ent)
		{
			if (tmp_ent == enta)
				break;
			if (tmp_ent == entb)
			{
				drawing = TRUE;
				break;
			}
			tmp_ent = tmp_ent->next;
		}
	}

	ent = orig_ent;
	while (ent)
	{
		if (inclusive && ent == enta)
			drawing = TRUE;

		if (drawing || ent == entb || ent == enta)
		{
			gtk_xtext_reset (xtext, FALSE, TRUE);
			line += gtk_xtext_render_line (xtext, ent, line, lines_max,
													 subline, width);
			subline = 0;
		} else
		{
			if (ent == orig_ent)
			{
				line -= subline;
				subline = 0;
			}
			line += ent->lines_taken;
		}

		if (inclusive && ent == entb)
			break;

		if (line >= lines_max)
			break;

		ent = ent->next;
	}
}

/* render a whole page/window, starting from 'startline' */

static void
gtk_xtext_render_page (GtkXText * xtext)
{
	textentry *ent;
	int line;
	int lines_max;
	int width;
	int height;
	int subline;
	int startline = xtext->adj->value;

	if (xtext->indent < MARGIN)
		xtext->indent = MARGIN;	  /* 2 pixels is our left margin */

	gdk_window_get_size (GTK_WIDGET (xtext)->window, &width, &height);
	width -= MARGIN;

	if (width < 32 || height < xtext->fontsize || width < xtext->indent + 30)
		return;

#ifdef SMOOTH_SCROLL
	xtext->pixel_offset = ((float)((float)xtext->adj->value - (float)startline) * xtext->fontsize);
	lines_max = (height - xtext->font->descent + xtext->pixel_offset) / xtext->fontsize + 1;
#else
	xtext->pixel_offset = 0;
	lines_max = (height - xtext->font->descent) / xtext->fontsize;
#endif

	subline = line = 0;
	ent = xtext->text_first;

	if (startline > 0)
		ent = gtk_xtext_nth (xtext, ent, startline, &subline);

	xtext->pagetop_ent = ent;
	xtext->pagetop_subline = subline;
	xtext->pagetop_line = startline;

	while (ent)
	{
		gtk_xtext_reset (xtext, FALSE, TRUE);
		line += gtk_xtext_render_line (xtext, ent, line, lines_max,
												 subline, width);
		subline = 0;

		if (line >= lines_max)
			break;

		ent = ent->next;
	}

	line = (xtext->fontsize * line) - xtext->pixel_offset;
	/* fill any space below the last line with our background GC */
	gdk_draw_rectangle (xtext->draw_buf, xtext->bgc, 1,
						 0, line, width + MARGIN, height - line);

	/* draw the separator line */
	gtk_xtext_draw_sep (xtext, -1);
}

void
gtk_xtext_refresh (GtkXText * xtext, int do_trans)
{
	if (GTK_WIDGET_REALIZED (GTK_WIDGET (xtext)))
	{
#ifdef USE_XLIB
		if (xtext->transparent && do_trans)
		{
			gtk_xtext_free_trans (xtext);
			gtk_xtext_load_trans (xtext);
		}
#endif
		gtk_xtext_render_page (xtext);
	}
}

/* remove the topline from the list */

static void
gtk_xtext_remove_top (GtkXText * xtext)
{
	textentry *ent;

	ent = xtext->text_first;
	if (!ent)
		return;
	xtext->num_lines -= ent->lines_taken;
	xtext->pagetop_line -= ent->lines_taken;
	xtext->text_first = ent->next;

	if (ent == xtext->pagetop_ent)
		xtext->pagetop_ent = NULL;

	if (ent == xtext->last_ent_start)
		xtext->last_ent_start = ent->next;

	if (ent == xtext->last_ent_end)
	{
		xtext->last_ent_start = NULL;
		xtext->last_ent_end = NULL;
	}

	free (ent);
}

void
gtk_xtext_clear (GtkXText * xtext)
{
	textentry *next;

	xtext->last_ent_start = NULL;
	xtext->last_ent_end = NULL;

	while (xtext->text_first)
	{
		next = xtext->text_first->next;
		free (xtext->text_first);
		xtext->text_first = next;
	}
	xtext->text_last = NULL;

	gtk_xtext_calc_lines (xtext, TRUE);
	gtk_xtext_refresh (xtext, 0);

	if (xtext->auto_indent)
		xtext->indent = 1;
}

void *
gtk_xtext_search (GtkXText * xtext, unsigned char *text, void *start)
{
	textentry *ent, *fent;
	unsigned char *str;
	int line;

	gtk_xtext_selection_clear (xtext);
	xtext->last_ent_start = NULL;
	xtext->last_ent_end = NULL;

	if (start)
		ent = ((textentry *) start)->next;
	else
		ent = xtext->text_first;
	while (ent)
	{
		if ((str = nocasestrstr (ent->str, text)))
		{
			ent->mark_start = str - ent->str;
			ent->mark_end = ent->mark_start + strlen (text);
			break;
		}
		ent = ent->next;
	}

	fent = ent;
	ent = xtext->text_first;
	line = 0;
	while (ent)
	{
		line += ent->lines_taken;
		ent = ent->next;
		if (ent == fent)
			break;
	}
	while (line > xtext->adj->upper - xtext->adj->page_size)
		line--;

	if (fent != 0)
	{
		xtext->adj->value = line;
		xtext->scrollbar_down = FALSE;
		gtk_adjustment_changed (xtext->adj);
	}
	gtk_xtext_render_page (xtext);

	return fent;
}

static int
gtk_xtext_render_page_timeout (GtkXText * xtext)
{
	GtkAdjustment *adj = xtext->adj;
	gfloat val;

	if (xtext->scrollbar_down)
	{
		gtk_xtext_adjustment_set (xtext, FALSE);
		gtk_adjustment_set_value (adj, adj->upper - adj->page_size);
	} else
	{
		val = adj->value;
		gtk_xtext_adjustment_set (xtext, TRUE);
		gtk_adjustment_set_value (adj, val);
	}

	if (adj->value >= adj->upper - adj->page_size || adj->value < 1)
		gtk_xtext_render_page (xtext);

	xtext->add_io_tag = 0;

	return 0;
}

/* append a textentry to our linked list */

static void
gtk_xtext_append_entry (GtkXText * xtext, textentry * ent)
{
	unsigned int mb;

	ent->stamp = time (0);
	ent->str_width = gtk_xtext_text_width (xtext, ent->str, ent->str_len, &mb);
	ent->mb = FALSE;
	if (mb)
		ent->mb = TRUE;
	ent->mark_start = -1;
	ent->mark_end = -1;
	ent->next = NULL;

	if (ent->indent < MARGIN)
		ent->indent = MARGIN;	  /* 2 pixels is the left margin */

	/* append to our linked list */
	if (xtext->text_last)
		xtext->text_last->next = ent;
	else
		xtext->text_first = ent;
	xtext->text_last = ent;

	ent->lines_taken = gtk_xtext_lines_taken (xtext, ent);
	xtext->num_lines += ent->lines_taken;

	if (xtext->max_lines > 2 && xtext->max_lines < xtext->num_lines)
	{
		gtk_xtext_remove_top (xtext);
	}

/*   if (xtext->frozen == 0 && !xtext->add_io_tag)*/
	if (!xtext->add_io_tag)
	{
		xtext->add_io_tag = g_timeout_add (REFRESH_TIMEOUT * 2,
														(GSourceFunc)
														gtk_xtext_render_page_timeout,
														xtext);
	}
}

/* the main two public functions */

void
gtk_xtext_append_indent (GtkXText * xtext,
								 unsigned char *left_text, int left_len,
								 unsigned char *right_text, int right_len)
{
	textentry *ent;
	unsigned char *str;
	int space;
	int tempindent;
	int left_width;

	if (left_len == -1)
		left_len = strlen (left_text);

	if (right_len == -1)
		right_len = strlen (right_text);

	if (right_len >= sizeof (xtext->scratch_buffer))
		right_len = sizeof (xtext->scratch_buffer) - 1;

	if (right_text[right_len-1] == '\n')
		right_len--;

#ifdef WIN32
	left_text = g_locale_to_utf8 (left_text, left_len, NULL, &left_len, NULL);
	right_text = g_locale_to_utf8 (right_text, right_len, NULL, &right_len, NULL);
#endif

	ent = malloc (left_len + right_len + 2 + sizeof (textentry));
	str = (unsigned char *) ent + sizeof (textentry);

	memcpy (str, left_text, left_len);
	str[left_len] = ' ';
	memcpy (str + left_len + 1, right_text, right_len);
	str[left_len + 1 + right_len] = 0;

	left_width = gtk_xtext_text_width (xtext, left_text, left_len, NULL);

	ent->left_len = left_len;
	ent->str = str;
	ent->str_len = left_len + 1 + right_len;
	ent->indent = (xtext->indent - left_width) - xtext->space_width;

	if (xtext->time_stamp)
		space = xtext->stamp_width;
	else
		space = 0;

	/* do we need to auto adjust the separator position? */
	if (xtext->auto_indent && ent->indent < MARGIN + space)
	{
		tempindent = MARGIN + space + xtext->space_width + left_width;

		if (tempindent > xtext->indent)
			xtext->indent = tempindent;

		if (xtext->indent > xtext->max_auto_indent)
			xtext->indent = xtext->max_auto_indent;

		gtk_xtext_fix_indent (xtext);
		gtk_xtext_recalc_widths (xtext, FALSE);

		ent->indent = (xtext->indent - left_width) - xtext->space_width;
	}

#ifdef WIN32
	g_free (left_text);
	g_free (right_text);
#endif

	gtk_xtext_append_entry (xtext, ent);
}

void
gtk_xtext_append (GtkXText * xtext, unsigned char *text, int len)
{
	textentry *ent;

	if (len == -1)
		len = strlen (text);

	if (text[len-1] == '\n')
		len--;

	if (len >= sizeof (xtext->scratch_buffer))
		len = sizeof (xtext->scratch_buffer) - 1;

#ifdef WIN32
	text = g_locale_to_utf8 (text, len, NULL, &len, NULL);
#endif

	ent = malloc (len + 1 + sizeof (textentry));
	ent->str = (unsigned char *) ent + sizeof (textentry);
	ent->str_len = len;
	if (len)
		memcpy (ent->str, text, len);
	ent->str[len] = 0;
	ent->indent = 0;
	ent->left_len = -1;

#ifdef WIN32
	g_free (text);
#endif

	gtk_xtext_append_entry (xtext, ent);
}
