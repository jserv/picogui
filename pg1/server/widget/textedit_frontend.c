/* $Id$
 *
 * textedit.c - Multi-line text widget. By Chuck Groom,
 * cgroom@bluemug.com, Blue Mug, Inc, July 2002. Intended to be
 * flexible enough to handle both editing a 2Mb text document and also
 * displaying a few lines of read-only text.
 *
 * This is an early version. It is buggy. pgserver may crash. I'm
 * working on it, and apologize but take no responsibility. ;)
 *
 * Currently, the client may create a text widget with/without a
 * scrollbar and read-only/not read-only, and set/get plaintext
 * (ASCII). The user can type, move the cursor around with the mouse
 * or arrows, and scroll around.
 *
 * TBD; Short term:
 *   - Beat up on widget to identify bugs (yes, there are several)
 *   - The scrollbar implementation is shameful
 *   - Optimize wrapping
 *   - On idle, compress blocks
 *   - Add client text manipulation: 
 *     - Insert string
 *     - Delete string
 *     - Scroll to line
 *     - Get # lines
 *     - Scroll to virtual y
 *     - Get virtual y height
 *     - Set cursor width, blink 
 *   - Add client events for
 *     - Insert
 *     - Delete
 *     - Cursor moves
 *     - Scroll
 *     - Cursor hits top 
 *     - Cursor hits bottom
 *   - Tab is a fixed location, not distance
 * TBD; Long term
 *   - Multiple font support
 *   - Widget draws on top of background, not a solid rect
 * TBD; Very long term
 *   - Unicode
 *   - On-demand (read-only). This is useful for reading very large text files
 *     without storing everything on the server.
 *
 * Overview: 
 * 
 * The code is divided into two sections: the part which is
 * pgui-specific (textedit_frontend.c) and the part which manages a generic
 * text editor (textedit_logical.c). There is also a generic linked list 
 * (textedit_llist.c).
 *
 * The text editor widget has a bitmap, a scrollbar, and a cursor. It
 * draws directly to the bitmap; no grops. When the pgui drawing
 * abstractions are called, we keep track of the minimum rect that
 * needs to be updated in the bitmap, then periodically blit over the
 * update rect.
 *
 * Text is stored in the following manner:
 *   - The widget has a list of blocks
 *   -  A block has a text buffer and a list of paragraphs
 *   -  A paragraph has a list of atoms
 *         o  Paragraphs cannot span blocks
 *   -  An atom describes the content in a line or partial line.
 *         o  Atoms cannot span lines 
 * 
 * The block's text buffer is buffer-gapped. When a paragraph grows
 * too large to fit in a block, we shed the last paragraph to a new
 * block; if the block only contains a single paragraph, grow the
 * size of the block's buffer.
 * 
 * Atoms have flags. The most useful are the text atom's left/right
 * flags, which are really useful for quickly wrapping and drawing.
 * 
 * About PicoGUI:
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
 *   Textedit widget started by John Blair, rewritten and completed by 
 *   Chuck Groom, cgroom@bluemug.com. Blue Mug, Inc, July 2002.
 */

#include <assert.h>
#include <stdio.h>
#include <pgserver/common.h>
#include <pgserver/widget.h>
#include <pgserver/hotspot.h>
#include <pgserver/handle.h>
#include <pgserver/video.h>
#include <pgserver/render.h>
#include <picogui/pgkeys.h>
#include <picogui/types.h>

#include <pgserver/textedit.h>
#define WIDGET_SUBCLASS 0
#define DATA WIDGET_DATA(texteditdata)
#define CTX  (&DATA->ctx)

/* Cursor properties (TBD: Theme these) */
#define CURSORWIDTH     2
#define FLASHTIME_ON    700
#define FLASHTIME_OFF   300
#define CURSORCOLOR_ON  0x000000
#define CURSORCOLOR_OFF 0xCCCCCC
#define TEXT_FG         0x000000
#define TEXT_BG         0xFFFFFF
#define TEXT_HIGHLIGHT  0xc2dbe8


void        textedit_remove         ( struct widget *self );
g_error     textedit_set            ( struct widget *self,
                                      int property, 
                                      glob data );
glob        textedit_get            ( struct widget *self,
                                      int property );
void        textedit_build          ( struct gropctxt *c,
                                      unsigned short state,
                                      struct widget *self );
void        textedit_build_scroll   ( struct gropctxt *c,
                                      unsigned short state,
                                      struct widget *self );
void        textedit_draw_update    ( struct widget * self );
static TEXTEDIT_CHAR upper          ( TEXTEDIT_CHAR ch );
static void add_update_region       ( struct widget * self,
                                      s16 x, s16 y, 
                                      s16 w, s16 h );
static u8   point_in_rect           ( s16 x, s16 y,
                                      s16 r_x1, s16 r_y1,
                                      s16 r_x2, s16 r_y2);
static void textedit_draw_rect     ( struct widget *self, 
                                     s16 x, s16 y, 
                                     s16 w, s16 h,
                                     hwrcolor color );

/* Set up divnodes */
g_error textedit_install(struct widget *self) {
    g_error e;

    WIDGET_ALLOC_DATA(texteditdata);

    /* main split */
    e = newdiv(&self->in, self);  
    errorcheck;
    
    self->in->flags |= PG_S_ALL;

    /* Visible node */
    e = newdiv(&self->in->div, self);  
    errorcheck;
    
    self->in->div->build = &textedit_build; 
    self->in->div->state = PGTH_O_TEXTEDIT;
    self->in->div->flags = DIVNODE_SPLIT_EXPAND | DIVNODE_SIZE_AUTOSPLIT | 
        DIVNODE_SIZE_RECURSIVE | DIVNODE_HOTSPOT; 
    self->trigger_mask = PG_TRIGGER_ACTIVATE |
        PG_TRIGGER_DEACTIVATE | PG_TRIGGER_DRAG |
        PG_TRIGGER_DOWN | PG_TRIGGER_RELEASE | 
        PG_TRIGGER_KEYUP | PG_TRIGGER_KEYDOWN | 
        PG_TRIGGER_TIMER |  PG_TRIGGER_NONTOOLBAR;
 
    self->out = &self->in->next;
    gropctxt_init(CTX,self->in->div);

    self->in->div->flags |= PG_S_RIGHT;
    self->in->div->flags &= ~DIVNODE_SIZE_AUTOSPLIT;

    DATA->fg =        VID(color_pgtohwr) (TEXT_FG);
    DATA->bg =        VID(color_pgtohwr) (TEXT_BG);
    DATA->highlight = VID(color_pgtohwr) (TEXT_HIGHLIGHT);           
    DATA->bit = NULL;
    DATA->thumb_size = 0;
    DATA->thumb_top = 0;
    DATA->thumb_drag_start = 0;
    DATA->scroll_lock = 0;

    e = text_backend_init(DATA);
    errorcheck;

    self->in->div->flags |= PG_S_RIGHT;
    self->in->div->flags &= ~DIVNODE_SIZE_AUTOSPLIT;

    e = newdiv(&self->in->div->div,self);
    errorcheck;
    self->in->div->div->build = &textedit_build_scroll;
    self->in->div->div->state = PGTH_O_SCROLL;
    self->out = &self->in->div->next;

    DATA->self = self;
    return success;
}

void textedit_build_scroll ( struct gropctxt *c,
                             unsigned short state,
                             struct widget *self ) {
    self->in->div->div->r.w = theme_lookup(PGTH_O_SCROLL_V,PGTH_P_WIDTH);
    self->in->div->div->r.x = self->in->div->preferred.w + self->in->div->r.x;
    
    exec_fillstyle(c,state,PGTH_P_BGFILL);
        
    c->defaultgropflags = PG_GROPF_TRANSLATE;
    
    if (DATA->thumb_size) {
        c->r.w = theme_lookup(PGTH_O_SCROLL_V,PGTH_P_WIDTH);
        c->r.h = DATA->thumb_size;
        c->r.y = DATA->thumb_top;
        exec_fillstyle(c,state,PGTH_P_OVERLAY);
    } 
}

void textedit_build ( struct gropctxt *c,
                      unsigned short state,
                      struct widget *self ) {
    s16 w, h, tw;
    g_error e;
    
    w = self->in->div->r.w - theme_lookup(PGTH_O_SCROLL_V,PGTH_P_WIDTH);
    h = self->in->div->r.h;

    self->in->div->preferred.h = h;
    self->in->div->preferred.w = w;

    if (!DATA->fd){
        /* FIXME: Theme lookup foreground, background colors, border */
        e = textedit_set_font (self, theme_lookup (state, PGTH_P_FONT));
//        errorcheck;
    }
    assert (DATA->fd);

    /*
     * The general rule is that once you create a handle you should never
     * delete the object it refers to, only delete the handle
     */
    handle_free(self->owner,DATA->bit_h);

    e = VID(bitmap_new) (&(DATA->bit), w, h, vid->bpp);
//    errorcheck;

    /* the handle should be owned by the application not by pgserver itself */
    e = mkhandle(&DATA->bit_h, PG_TYPE_BITMAP, self->owner, DATA->bit);
//    errorcheck;

    /* Size and add the bitmap itself */
    e = addgropsz(c, PG_GROP_BITMAP, 0, 0, w, h);
//    errorcheck;

    c->current->param[0] = DATA->bit_h;

    /* Create cursor */
    e = addgropsz(c,PG_GROP_RECT, 0, 0, 0, 0);
//    errorcheck;
    
    c->current->flags |= PG_GROPF_COLORED;
    DATA->cursor_grop = c->current;
    DATA->cursor_state = 1;
    DATA->cursor_grop->param[0] = VID(color_pgtohwr)(CURSORCOLOR_ON);

    /* Set cursor height to that of typical char */
    textedit_str_size(self, NULL, 0, &tw, &DATA->cursor_grop->r.h); 
    DATA->cursor_grop->r.x = DATA->border_h;
    DATA->cursor_grop->r.y = DATA->border_v;

    e = text_backend_build( DATA, w, h);
//    errorcheck;

//    return success;
}


void textedit_remove(struct widget *self) {    
    /*
     * The general rule is that once you create a handle you should never
     * delete the object it refers to, only delete the handle
     */
    handle_free(self->owner,DATA->bit_h);
  
    text_backend_destroy(DATA);
    g_free(DATA);    
    r_divnode_free(self->in);
}


void textedit_resize(struct widget *self) { 
    // FIXME
    //    self->in->div->preferred.w = self->in->div->r.w;
    //    self->in->div->preferred.h = self->in->div->r.h;
}


g_error textedit_set ( struct widget *self,
                       int property, 
                       glob data ) {
    struct pgstring *text;

    switch (property) {
    case PG_WP_TEXT: 
        if (iserror(rdhandle((void **) &text, PG_TYPE_PGSTRING, -1, 
                             (handle) data)))
            text = NULL;
        
        if (text) {
            text_backend_set_text(DATA, text);
        }
        break;
    case PG_WP_FONT:
        if (iserror(rdhandle((void **)&DATA->fd,
                             PG_TYPE_FONTDESC,-1,data)) || !DATA->fd) 
            return mkerror(PG_ERRT_HANDLE,44); 
        text_backend_build(DATA, DATA->width, DATA->height);
        break;
    case PG_WP_SELECTION:
        if (iserror(rdhandle((void **) &text, PG_TYPE_PGSTRING, -1, 
                             (handle) data)))
            text = NULL;
        if (text) 
            text_backend_set_selection(DATA, text);
        break;
    case PG_WP_READONLY:
        if (data) { 
            SET_FLAG(DATA->flags, TEXT_WIDGET_READONLY);
            UNSET_FLAG(DATA->flags, TEXT_WIDGET_FLASH_ON);
        } else {
            UNSET_FLAG(DATA->flags, TEXT_WIDGET_READONLY);
            SET_FLAG(DATA->flags, TEXT_WIDGET_FLASH_ON);
            DATA->cursor_state = 1;
            install_timer(self, FLASHTIME_ON);
        }
        break;
    }
    return success;
}


glob textedit_get ( struct widget *self,
                    int property ) {
     g_error e;
     switch (property) {
     case PG_WP_TEXT:
         text_backend_save(DATA);
	 return (glob) DATA->client_data_h;
     case PG_WP_SELECTION:
         text_backend_store_selection(DATA);
         return (glob) DATA->client_data_h;
     case PG_WP_READONLY:
         return (glob) GET_FLAG(DATA->flags, TEXT_WIDGET_READONLY);
     default:
         return 0;
     }
}


void textedit_trigger ( struct widget *self,
                        s32 type,
                        union trigparam *param) {
    int key;
    struct cursor * c;
    u32 v_height, v_top;
    struct font_metrics m;

    switch (type) {
    case PG_TRIGGER_DEACTIVATE:
        UNSET_FLAG(DATA->flags, TEXT_WIDGET_FOCUS);
        UNSET_FLAG(DATA->flags, TEXT_WIDGET_FLASH_ON);
        break;
    case PG_TRIGGER_ACTIVATE:
        SET_FLAG(DATA->flags, TEXT_WIDGET_FOCUS);
        SET_FLAG(DATA->flags, TEXT_WIDGET_FLASH_ON);
        post_event(PG_WE_FOCUS,self,1,0,NULL);
        /* Fall into PG_TRIGGER_TIMER to set up cursor flash */
    case PG_TRIGGER_TIMER:
        if (!DATA->cursor_state)
            DATA->cursor_state++;
        else
            DATA->cursor_state--;
        DATA->cursor_grop->param[0] = VID(color_pgtohwr)(DATA->cursor_state ? 
                                                         CURSORCOLOR_ON :
                                                         CURSORCOLOR_OFF );
        add_update_region(self,
                          DATA->cursor_grop->r.x,
                          DATA->cursor_grop->r.y,
                          DATA->cursor_grop->r.w,
                          DATA->cursor_grop->r.h);
        if (GET_FLAG(DATA->flags, TEXT_WIDGET_FLASH_ON)) {
            /* Reset timer */
            DATA->cursor_grop->r.w = CURSORWIDTH;
            install_timer(self, (DATA->cursor_state ? FLASHTIME_ON : 
                                 FLASHTIME_OFF ));
        } else { 
            DATA->cursor_grop->r.w = 0;
         }
        break;
    case PG_TRIGGER_DOWN:
        if (!GET_FLAG(DATA->flags, TEXT_WIDGET_FOCUS))
             request_focus(self);
        if (!GET_FLAG(DATA->flags, TEXT_WIDGET_READONLY)) {
            if (param->mouse.x - self->in->div->r.x < DATA->width) {
                /* Move cursor. */
                DATA->thumb_drag_start = -1;
                text_backend_cursor_move_xy ( DATA, 
                                              param->mouse.x - self->in->div->r.x,
                                              param->mouse.y - self->in->div->r.y);
                grop_render(self->in->div, NULL);
            } else {
                text_backend_selection_unset(DATA);
                v_height = MAX(DATA->v_height, 
                               DATA->cursor_v_y + DATA->cursor_grop->r.h);
                
                /* In scrollbar region */
                if ((param->mouse.y - self->in->div->r.y < DATA->thumb_top) ||
                    (param->mouse.y - self->in->div->r.y >
                     DATA->thumb_top + DATA->thumb_size)) {
                    /* Page up or down */
                    if (param->mouse.y - self->in->div->r.y < DATA->thumb_top)
                        DATA->thumb_top -= DATA->thumb_size;
                    else 
                        DATA->thumb_top += DATA->thumb_size;
                    DATA->thumb_top = MAX(0, MIN(DATA->thumb_top, 
                                                 DATA->height - 
                                                 DATA->thumb_size));
                    v_top = (v_height * DATA->thumb_top) / DATA->height;
                    /* Round to full screen lines */
                    DATA->fd->lib->getmetrics(DATA->fd,&m);
                    if (v_top % m.lineheight) {
                        v_top = (v_top  - v_top % m.lineheight) +
                            m.lineheight;
                    }
                    if (v_top != DATA->v_y_top) {
                        DATA->scroll_lock = 1;                
                        text_backend_set_v_top(DATA, v_top);
                        DATA->scroll_lock = 0;
                    }
                    div_rebuild(self->in->div->div);
                    update(NULL,1);
                } else {
                    DATA->thumb_drag_start = 
                        (param->mouse.y - self->in->div->r.y) - 
                        DATA->thumb_top;
                }
            }
        } 
        break;
    case PG_TRIGGER_DRAG:
        if (param->mouse.btn) {
            if (DATA->thumb_drag_start < 0) {
                text_backend_selection_xy( DATA,
                                           param->mouse.x - self->in->div->r.x,
                                           param->mouse.y - self->in->div->r.y );
            } else {
                v_height = MAX(DATA->v_height, 
                               DATA->cursor_v_y + DATA->cursor_grop->r.h);
                DATA->thumb_top = param->mouse.y - 
                    self->in->div->r.y -
                    DATA->thumb_drag_start;
                DATA->thumb_top = MAX(0, MIN(DATA->thumb_top, 
                                             DATA->height - DATA->thumb_size));
                DATA->thumb_drag_start = param->mouse.y -
                    self->in->div->r.y -
                    DATA->thumb_top;
                v_top = (v_height * DATA->thumb_top) / DATA->height;
                /* Round to full screen lines */
                DATA->fd->lib->getmetrics(DATA->fd,&m);
                if (v_top % m.lineheight) {
                    v_top = (v_top  - v_top % m.lineheight) +
                                              m.lineheight;
                }
                if (v_top != DATA->v_y_top) {
                    DATA->scroll_lock = 1;  
                    text_backend_set_v_top(DATA, v_top);
                    DATA->scroll_lock = 0;  
                }
                div_rebuild(self->in->div->div);
                update(NULL,1);
            }
        }
        break;
    case PG_TRIGGER_KEYUP:
        if (GET_FLAG(DATA->flags, TEXT_WIDGET_READONLY) ||
            !GET_FLAG(DATA->flags, TEXT_WIDGET_FOCUS))
            return;
        param->kbd.consume++;
        return;
    case PG_TRIGGER_KEYDOWN:
        if (GET_FLAG(DATA->flags, TEXT_WIDGET_READONLY) ||
            !GET_FLAG(DATA->flags, TEXT_WIDGET_FOCUS))
            return;
        param->kbd.consume++;
        key = param->kbd.key;

        switch(key) {
#ifdef DEBUG_TEXTEDIT 
        case PGKEY_F1:
            print_data(DATA);
            break;
        case PGKEY_F2:
            print_tree(DATA);
            break;
        case PGKEY_F3:
            print_block(DATA, DATA->current);
            break;
#endif /* DEBUG_TEXTEDIT */
        case PGKEY_RETURN:
            text_backend_insert_char(DATA, '\n');
            break;
        case PGKEY_BACKSPACE:
        case PGKEY_DELETE:
            /* Screw "backspace" vs. "delete". This is my widget, and
             * I say they're going to be the same thing. So, there. */
            text_backend_delete_char(DATA);
            break;
        case PGKEY_TAB:
            if (param->kbd.mods & PGMOD_SHIFT) {
                /* Post event to app */
                post_event(PG_WE_ACTIVATE,self,0,0,NULL);
                param->kbd.consume--;
                return;
            } else {
                text_backend_insert_char(DATA, '\t');
            }
            break;
        case PGKEY_UP:
        case PGKEY_DOWN:
        case PGKEY_RIGHT:
        case PGKEY_LEFT: 
        case PGKEY_HOME:
        case PGKEY_END:
        case PGKEY_PAGEUP:
        case PGKEY_PAGEDOWN: 
            /* CURSOR_UP etc. match the PGKEY_UP etc. values */
            if (param->kbd.mods & PGMOD_SHIFT)
                text_backend_selection_dir(DATA, key);
            else
                text_backend_cursor_move_dir(DATA, key);
            break;
        default:
            if ((key >= PGKEY_SPACE) && (key <= PGKEY_WORLD_95)) {
                if (param->kbd.mods & (PGMOD_SHIFT | PGMOD_CAPS)) {
                    key = upper(key);
                }
		/* if ctrl key is pressed*/
		if (param->kbd.mods & PGMOD_CTRL) {
		  text_backend_cut_copy_paste(DATA, key);
		}
                else {
		  text_backend_insert_char(DATA, key);
		}
            } else {
                /* Post key to app */
                post_event(PG_WE_ACTIVATE,self,0,0,NULL);
                param->kbd.consume--;
                return;
            }
        }
    }
    textedit_draw_update(self);
    //update(NULL,1);
}

void textedit_scrollevent( struct widget *self ) {
    u16 thumb_size, thumb_top;
    u32 v_height;

    if (DATA->scroll_lock)
        return;
    DATA->scroll_lock = 1;
    
    v_height = MAX(DATA->v_height, DATA->cursor_v_y + DATA->cursor_grop->r.h);
        
    if (DATA->height < v_height) {
        thumb_size = (DATA->height * self->in->div->div->r.h) / v_height;
        thumb_top = (DATA->v_y_top * self->in->div->div->r.h) / v_height;
        if (thumb_top + thumb_size > DATA->height)
            thumb_top = DATA->height - thumb_size;
    } else {
        thumb_size = 0;
        thumb_top = 0;
    }
    if ((thumb_size != DATA->thumb_size) || (thumb_top != DATA->thumb_top)) {
        DATA->thumb_size = thumb_size;
        DATA->thumb_top = thumb_top;
        div_rebuild(self->in->div->div);
        update(NULL,1);
    }
    DATA->scroll_lock = 0;
}



/******************************************************* 
 * Back-end services 
 */


void textedit_draw_str ( struct widget * self,
                         TEXTEDIT_UCHAR * txt,
                         size_t len,
                         s16 x, 
                         s16 y,
                         u8 highlight) {
    s16 w, h;
    struct pgpair t;
    size_t k;
    TEXTEDIT_UCHAR ch;

    if (len == 0)
        return;
    
    t.x = x;
    t.y = y;
    
    textedit_str_size ( self, txt, len, &w, &h);
    textedit_draw_rect ( self, x, y, w, h, 
			 highlight ? DATA->highlight : DATA->bg );

    for (k = 0; k < len; k++) {
        ch = (TEXTEDIT_CHAR) *(txt + k);
        if (ch == '\t') {
            textedit_str_size (self, &ch, 1, &w, &h);
            t.x += w;
        } else if (ch != '\n') {
            /* Clipping should work, but doesn't. So we have to be totally
             * lame and not display chars that would be cut off 
	     *
	     * Note to Chuck from Micah:
	     *   Clipping should work... what's the problem specifically?
             *
             * Re: server seg faults on drawing chars which are partially 
             * off-screen
             */
            if (t.y + h >= DATA->height)
                continue;

	    DATA->fd->lib->draw_char(DATA->fd,DATA->bit,&t,
				     DATA->fg,ch,NULL,PG_LGOP_NONE,0);
        }
    }
}


/* Get the physical size of a single line of text */
void textedit_str_size ( struct widget *self,
                         TEXTEDIT_UCHAR * txt,
                         size_t len,
                         s16 * w,
                         s16 * h ) {
    int ch;
    struct pgpair p;
    struct font_metrics m;

    DATA->fd->lib->getmetrics(DATA->fd,&m);  
    *h = m.lineheight;

    for (*w = 0; len; len--) {
        ch = *txt;
        txt++;
        if (ch == '\t') {
	    p.x = 0;
	    DATA->fd->lib->measure_char(DATA->fd,&p,' ',0);
            *w += p.x * 4;
        } else if (ch != '\n') {
	    p.x = 0;
	    DATA->fd->lib->measure_char(DATA->fd,&p,ch,0);
	    *w += p.x;
        } 
    }
}


void textedit_char_size  ( struct widget * self,
                           TEXTEDIT_CHAR ch,
                           s16 * w, 
                           s16 * h ) {
    textedit_str_size (self, (TEXTEDIT_UCHAR *) &ch, 1, w, h);
}


g_error textedit_set_font ( struct widget *self,
                            handle font ) {
    return rdhandle((void **)&(DATA->fd), PG_TYPE_FONTDESC, -1, font);
}


void textedit_draw_update ( struct widget *self ) {
    g_error e;
    struct cursor * c;
    if (!DATA->update_clean) {
        /* Translate to screen coordinates */
        DATA->update_x1 += self->in->div->r.x;
        DATA->update_x2 += self->in->div->r.x;
        DATA->update_y1 += self->in->div->r.y;
        DATA->update_y2 += self->in->div->r.y;

        grop_render(self->in->div, NULL);  
        VID(update) (self->dt->display,
		     DATA->update_x1,
                     DATA->update_y1,
                     DATA->update_x2 - DATA->update_x1,
                     DATA->update_y2 - DATA->update_y1);
        
        /* Make sure we show cursors affected by update */
        for (c = cursor_get_default(); c; c = c->next) {
            if ((c->sprite) && 
                (point_in_rect(c->sprite->x, c->sprite->y,
                               DATA->update_x1, DATA->update_y1,
                               DATA->update_x2, DATA->update_y2) ||
                 point_in_rect(c->sprite->x + c->sprite->w, 
                               c->sprite->y + c->sprite->h,
                               DATA->update_x1, DATA->update_y1,
                               DATA->update_x2, DATA->update_y2))) {
                VID(sprite_update) (c->sprite);
            } 
        }
        DATA->update_clean= 1;
    }
}


/* Move the cursor on screen. Add the old cursor rect to the list of
   regions to be drawn on the next update. If the cursor will be above
   the screen, put it safely off the bottom of the screen. */
void textedit_move_cursor ( struct widget * self,
                            s16 x, s16 y, s16 h ) {
    add_update_region(self, 
                      DATA->cursor_grop->r.x, 
                      DATA->cursor_grop->r.y,
                      DATA->cursor_grop->r.w,
                      DATA->cursor_grop->r.h);
    if (y + h < 0) {
        y = DATA->height;
    } else {
        add_update_region(self, x, y, DATA->cursor_grop->r.w, h);
    }
    DATA->cursor_grop->r.x = x;    
    DATA->cursor_grop->r.y = y;
    DATA->cursor_grop->r.h = h;
}


void textedit_clear_rect ( struct widget *self, 
                           s16 x, s16 y, 
                           s16 w, s16 h ) {  
    textedit_draw_rect(self, x, y, w, h, DATA->bg);
}


static void textedit_draw_rect (struct widget *self, 
                                s16 x, s16 y, 
                                s16 w, s16 h,
                                hwrcolor color) {
    assert (DATA->bit);
    if (x >= DATA->width)
        return;
    if (x + w >= DATA->width)
        w = DATA->width - x;
    if (y >= DATA->height)
        return;
    if (y + h >= DATA->height)
        h = DATA->height - y;
    
    VID(rect) (DATA->bit, 
               x, y, 
               w, h,
               color,
               PG_LGOP_NONE); 
    add_update_region(self, x, y, w, h);
}



static TEXTEDIT_CHAR upper(TEXTEDIT_CHAR ch) {
    if (TEXTEDIT_ISALPHA(ch))
        return TEXTEDIT_TOUPPER(ch);
    switch (ch) {
    case '`':
        return '~';
    case '1':
        return '!';
    case '2':
        return '@';
    case '3':
        return '#';
    case '4':
        return '$';
    case '5':
        return '%';
    case '6':
        return '^';
    case '7':
        return '&';
    case '8':
        return '*';
    case '9':
        return '(';
    case '0':
        return ')';
    case '-':
        return '_';
    case '=':
        return '+';
    case '[':
        return '{';
    case ']':
        return '}';
    case '|':
        return '\\';
    case ';':
        return ':';
    case '\'':
        return '"';
    case ',':
        return '<';
    case '.':
        return '>';
    default: 
        return '?';
    }
}

static void add_update_region ( struct widget * self,
                                s16 x, s16 y, 
                                s16 w, s16 h ) {
    s16 x2, y2;
    x2 = x + w;
    y2 = y + h;
    if (DATA->update_clean) {
        DATA->update_x1 = x;
        DATA->update_y1 = y;
        DATA->update_x2 = x2;
        DATA->update_y2 = y2;
    } else {
        DATA->update_x1 = MIN(DATA->update_x1, x);
        DATA->update_y1 = MIN(DATA->update_y1, y);
        DATA->update_x2 = MAX(DATA->update_x2, x2); 
        DATA->update_y2 = MAX(DATA->update_y2, y2);
    } 
    DATA->update_clean = 0;
}


static u8 point_in_rect ( s16 x, s16 y,
                          s16 r_x1, s16 r_y1,
                          s16 r_x2, s16 r_y2) {
    return ((x >= r_x1) && (x <= r_x2) && (y >= r_y1) && (y <= r_y2));
}

#ifdef DEBUG_TEXTEDIT 
void print_block ( text_widget * widget,
                   LList * ll_b ) {
    block * b;
    paragraph * p;
    atom * a;
    LList * ll_p, * ll_a;
    b = (block *) llist_data(ll_b);
    if (widget->current == ll_b)
        printf ("*");
    if (widget->fvb == ll_b)
        printf ("@");
    
    printf("Block %x len %d\n", b, b->len);
    for (ll_p = b->paragraphs; ll_p; ll_p = llist_next(ll_p)) {
        p = (paragraph *) llist_data(ll_p);
        if (b->cursor_paragraph == ll_p)
            printf ("*");
        printf("\tParagraph %x len %d height %d flags %x\n", 
               p, p->len, p->height, p->flags);
        
        for (ll_a = p->atoms; ll_a; ll_a = llist_next(ll_a)) {
            a = (atom *) llist_data(ll_a);
            if (b->cursor_atom == ll_a)
                printf ("*");
            
            printf("\t\tAtom %x len %d width %d height %d", 
                   a, a->len, a->width, a->height);
            if (a->flags & ATOM_FLAG_LEFT)
                printf(" L");
            if (a->flags & ATOM_FLAG_RIGHT)
                printf(" R");
            if (a->flags & ATOM_FLAG_DRAW)
                printf(" D");
            if (a->flags & ATOM_FLAG_SELECTED)
                printf(" S");
            printf("\n");
        }
    }
}

void print_tree ( text_widget * widget ) {
    LList * ll_b;
    for (ll_b = widget->blocks; ll_b; ll_b = llist_next(ll_b)) {
        print_block(widget, ll_b);
    }
}

void print_data ( text_widget * widget ) {
    u16 count = 0;
    u16 col = 0;
    block * b;
    LList * ll_b;

    for (ll_b = widget->blocks; ll_b; ll_b = llist_next(ll_b)) {
        b = (block *) llist_data(ll_b);

        printf("Block %x len %d size %d gap %d contains: \n[", 
               b, b->len, b->data_size, b->b_gap);
        for (count = 0, col = 1; count <  b->data_size; count++, col++) {
            if (col == 80)
                printf("\n");
            col = col % 80;
            
            if ((count >= b->b_gap) && 
                (count < b->b_gap + b->data_size - b->len)) {
                printf("*");
            } else if (b->data[count] == '\n') {
                printf("^");
            } else if (b->data[count] == '\t') {
                printf("_");
            } else {
                printf("%c", b->data[count]);
            }
        }
        printf("]\n\n");
    }
}


void print_string ( u8 * txt,
                    size_t len ) {
    size_t k;
    printf("\"");
    for (k = 0; k < len; k++) {
        if (txt[k] == '\n')
            printf("*");
        else if (txt[k] == '\t')
            printf("_");
        else
            printf("%c", txt[k]);
    }
    printf("\"\n");
}
#endif /* DEBUG_TEXTEDIT */
