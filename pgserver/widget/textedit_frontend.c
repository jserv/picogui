/* $Id: textedit_frontend.c,v 1.1 2002/10/05 11:21:05 micahjd Exp $
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
 * Open issues:
 *   - Should we introduce concept of text selection? This may imply a 
 *     clipboard, which to my knowledge is not part of pgui.
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
 *     - Get text selection
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
 *   - On-demand (read-only)
 *
 * Overview: 
 * 
 * The code is divided into two sections: the part which is
 * pgui-specific (textedit.c) and the part which manages a generic
 * text editor (textedit_logical.c).
 *
 * The text editor widget has a bitmap, a scrollbar, and a cursor. It
 * draws directly to the bitmap; no grops. When the pgui drawing
 * abstractions are called, we keep track of the minimum rect that
 * needs to be updated in the bitmap, then periodically blit over the
 * update rect.
 *
 * Text is stored in the following manner:
 *   The widget has a list of blocks
 *   A block has a text buffer and a list of paragraphs
 *   A paragraph has a list of atoms
 *   An atom describes the content in a line or partial line.
 *  
 * Paragraphs cannot span blocks. Atoms cannot span lines. The block's
 * text buffer is buffer-gapped. When a paragraph grows too large to fit
 * in a block, we shed the last paragraph to a new block or, if the block
 * only contains a single paragraph, grow the size of the block's buffer. 
 * 
 * Atoms have flags. The most useful are the text atom's left/right
 * flags, which are really useful for quickly wrapping and drawing.
 * 
 * We make extensive use of a generic linked list API, gcore/llist.c.
 *
 * About PicoGUI:
 * 
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2002 Micah Dowty <micahjd@users.sourceforge.net>
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
#define DATA WIDGET_DATA(0,texteditdata)
#define CTX  (&DATA->ctx)

/* Cursor properties (TBD: Theme) */
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

static char upper                   ( char ch );
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

    WIDGET_ALLOC_DATA(0,texteditdata);

    /* main split */
    e = newdiv(&self->in, self);  
    errorcheck;    
    self->in->flags |= PG_S_ALL;

    /* Visible node */
    e = newdiv(&self->in->div, self);  
    errorcheck;
    
    self->in->div->build = &textedit_build; 
    self->in->div->state = PGTH_O_TEXTEDIT;
    self->in->div->flags = DIVNODE_HOTSPOT | DIVNODE_SPLIT_TOP;
    self->trigger_mask = PG_TRIGGER_ACTIVATE |
        PG_TRIGGER_DEACTIVATE | PG_TRIGGER_DRAG |
        PG_TRIGGER_DOWN | PG_TRIGGER_RELEASE | 
        PG_TRIGGER_KEYUP | PG_TRIGGER_KEYDOWN | 
        PG_TRIGGER_TIMER |  PG_TRIGGER_NONTOOLBAR;
 
    self->out = &self->in->next;
    gropctxt_init(CTX,self->in->div);

    self->in->div->flags |= PG_S_RIGHT;
    self->in->div->flags &= ~DIVNODE_SIZE_AUTOSPLIT;

    SET_FLAG(DATA->flags, TEXT_WIDGET_HAS_SCROLLBAR);

    DATA->fg =        VID(color_pgtohwr) (TEXT_FG);
    DATA->bg =        VID(color_pgtohwr) (TEXT_BG);
    DATA->highlight = VID(color_pgtohwr) (TEXT_HIGHLIGHT);
           
    DATA->bit = NULL;
    text_backend_init(DATA);
    DATA->self = self;
    return success;
}

void textedit_build ( struct gropctxt *c,
                      unsigned short state,
                      struct widget *self ) {
    s16 w, h, tw;
    g_error e;

    w = self->in->div->w;
    h = self->in->div->h;

    /* Set scrollbar properties */
    self->in->div->ph = h;

    if (!DATA->fd){
        /* FIXME: Theme lookup foreground, background colors, border */
        textedit_set_font (self, theme_lookup (state, PGTH_P_FONT));
        /* FIXME: theme */
        DATA->fd->interline_space = 2;
    }
    assert (DATA->fd);

    /*
     * The general rule is that once you create a handle you should never
     * delete the object it refers to, only delete the handle
     */
    handle_free(self->owner,DATA->bit_h);

    e = VID(bitmap_new) (&(DATA->bit), w, h, vid->bpp);
    /* the handle should be owned by the application not by pgserver itself */
    e = mkhandle(&DATA->bit_h, PG_TYPE_BITMAP, self->owner, DATA->bit);

    /* Size and add the bitmap itself */
    e = addgropsz(c, PG_GROP_BITMAP, 0, 0, w, h);
    c->current->param[0] = DATA->bit_h;

    /* Create cursor */
    addgropsz(c,PG_GROP_RECT, 0, 0, 0, 0);
    c->current->flags |= PG_GROPF_COLORED;
    DATA->cursor_grop = c->current;
    DATA->cursor_state = 1;
    DATA->cursor_grop->param[0] = VID(color_pgtohwr)(CURSORCOLOR_ON);

    /* Set cursor height to that of typical char */
    textedit_str_size(self, NULL, 0, &tw, &DATA->cursor_grop->r.h); 
    DATA->cursor_grop->r.x = DATA->border_h;
    DATA->cursor_grop->r.y = DATA->border_v;

    text_backend_build( DATA, w, h);
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
    return;
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
        /* Fixme: theme */
        DATA->fd->interline_space = 2;
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
    case PG_WP_SCROLL_Y:
        if (self->in->div->h) {
            struct divnode * p_node;
            struct widget * parent;
            int top;
            p_node = divnode_findparent(self->dt->head, self->in);
            parent = p_node->owner;
            top = ((u32) data) * DATA->v_height /
                DATA->height;
            text_backend_set_v_top(DATA, top); 
        }
        break;
    }
    return success;
}


glob textedit_get ( struct widget *self,
                    int property ) {
     handle h;
     g_error e;
     switch (property) {
     case PG_WP_TEXT:
         text_backend_save(DATA);
         e = mkhandle(&h, PG_TYPE_PGSTRING, self->owner, DATA->data);
         errorcheck;
         return (glob) h;
     case PG_WP_SELECTION:
         text_backend_store_selection(DATA);
         e = mkhandle(&h, PG_TYPE_PGSTRING, self->owner, DATA->data);
         errorcheck;
         return (glob) h;
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
            /* Move cursor. */
            text_backend_cursor_move_xy ( DATA, 
                                          param->mouse.x - self->in->div->x,
                                          param->mouse.y - self->in->div->y);
            grop_render(self->in->div, NULL);
        } 
        break;
    case PG_TRIGGER_DRAG:
        if (param->mouse.btn) {
            text_backend_selection_xy( DATA,
                                       param->mouse.x - self->in->div->x,
                                       param->mouse.y - self->in->div->y );
        }
        return;
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
        case PGKEY_F10:
            /* Screen shot. Code taken from scrshot.c */
            {
                FILE *f;
                int x,y;
                f = fopen("temp.ppm","wb");  
                fprintf(f,"P6\n%d %d\n255\n",vid->xres,vid->yres);
                for (y=0;y<vid->yres;y++) {
                    for (x=0;x<vid->xres;x++) {
                        pgcolor c;
                        c = VID(color_hwrtopg)(VID(getpixel)(vid->display,x,y));
                        fputc(getred(c),f);
                        fputc(getgreen(c),f);
                        fputc(getblue(c),f);
                    }
                }
                fclose(f);
            }
            break;
#endif /* DEBUG_TEXTEDIT */
        case PGKEY_RETURN:
            text_backend_insert_char(DATA, '\n');
            break;
        case PGKEY_BACKSPACE:
        case PGKEY_DELETE:
            /* Screw "backspace" vs. "delete". This is my widget, and
               I say they're going to be the same thing. So, there. */
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
            if ((key >= PGKEY_SPACE) && (key <= PGKEY_TILDE)) {
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

/* FIXME
 * This is bad, wicked, naughty, and in all ways reprehensible. 
 */
struct scrolldata {
  int on,over;
  int res;       
  int grab_offset;
  int release_delta;
  int value,old_value;
  u32 wait_tick;
  int thumbscale;
};


void textedit_scrollevent( struct widget *self ) {
    int value, thumb, height;
    struct divnode * p_node;
    struct widget * parent;
    struct scrolldata * p_data;
    
    p_node = divnode_findparent(self->dt->head, self->in);
    parent = p_node->owner;
    p_data = (struct scrolldata *) (parent->data);
    value = (p_data->res * DATA->v_y_top) / DATA->v_height;
    height = MAX(DATA->height, DATA->v_height);

    if ((parent->type == PG_WIDGET_SCROLL) &&
        ((self->in->div->ph != height) ||
         p_data->value != value)) {
        self->in->div->ph = height;
        if (parent->in) {
            p_data->value = value;
            if (p_data->res)
                parent->in->div->ty = p_data->value * p_data->thumbscale / p_data->res;
            parent->in->div->flags |= DIVNODE_NEED_REDRAW;
            parent->dt->flags |= DIVTREE_NEED_REDRAW;
        }
        self->dt->flags |= DIVTREE_NEED_RESIZE;
        post_event(PG_WE_ACTIVATE,parent,0,0,NULL);
    }
}



/******************************************************* 
 * Back-end services 
 */


void textedit_draw_str ( struct widget * self,
                         u8 * txt,
                         size_t len,
                         s16 x, 
                         s16 y,
                         u8 highlight) {
    s16 tx, ty, w, h;
    size_t k;
    u8 ch;

    if (len == 0)
        return;
    
    tx = x;
    ty = y - DATA->fd->interline_space/2;
    
    textedit_str_size ( self, txt, len, &w, &h);
    textedit_draw_rect ( self, x, y, w, h, 
                          highlight ? DATA->highlight : DATA->bg );

    for (k = 0; k < len; k++) {
        ch = (char) *(txt + k);
        if (ch == '\t') {
            textedit_str_size (self, &ch, 1, &w, &h);
            tx += w;
        } else if (ch != '\n') {
            /* Clipping should work, but doesn't. So we have to be totally
               lame and not display chars that would be cut off */
            if (ty + h >= DATA->height)
                continue;
            outchar (DATA->bit, 
                     DATA->fd,
                     &tx,
                     &ty,
                     DATA->fg,
                     ch,
                     NULL, 
                     PG_LGOP_NONE, 0);
        }
    }
}


/* Get the physical size of a single line of text */
void textedit_str_size ( struct widget *self,
                         u8 * txt,
                         size_t len,
                         s16 * w, 
                         s16 * h ) {
    int ch;
    s16 tab_w;
    *h = DATA->fd->font->h + DATA->fd->interline_space;
    for (*w = 0; len; len--) {
        ch = *txt;
        txt++;
        if (ch == '\t') {
            tab_w = 0;
            outchar_fake(DATA->fd, &tab_w, ' ');
            *w += tab_w*4;
        } else if (ch != '\n') {
            outchar_fake(DATA->fd, w, ch);
        } 
    }
}


void textedit_char_size  ( struct widget * self,
                           char ch,
                           s16 * w, 
                           s16 * h ) {
    textedit_str_size (self, &ch, 1, w, h);
}


void textedit_set_font ( struct widget *self,
                         handle font ) {
    rdhandle((void **)&(DATA->fd), PG_TYPE_FONTDESC, -1, font);
}


void textedit_draw_update ( struct widget *self ) {
    struct cursor * c;
    if (!DATA->update_clean) {
        /* Translate to screen coordinates */
        DATA->update_x1 += self->in->div->x;
        DATA->update_x2 += self->in->div->x;
        DATA->update_y1 += self->in->div->y;
        DATA->update_y2 += self->in->div->y;

        grop_render(self->in->div, NULL);  
        VID(update) (DATA->update_x1,
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



static char upper(char ch) {
    if (isalpha(ch))
        return toupper(ch);
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
