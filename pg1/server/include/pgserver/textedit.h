/* $Id$
 *
 * Multi-line text widget. The widget is divided into PicoGUI specific
 * code in widget/textedit.c, and abstract text widget code in
 * widget/text_behavior.c.
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
 * Copyright (C) 2002 Blue Mug, Inc.
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
 * Initial version by Chuck Groom (cgroom@bluemug.com) and John Blair,
 * Blue Mug, Inc, 2002.
 */

#include <picogui/types.h>
#include <pgserver/divtree.h>
#include <pgserver/render.h>
#include <pgserver/handle.h>
#include <pgserver/llist.h>
#include <sys/types.h>

#ifndef __TEXT_H__
#define __TEXT_H__

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif


#ifdef CONFIG_TEXTEDIT_WCHART

# include <wchar.h>
# include <wctype.h>
# define TEXTEDIT_CHAR                    wchar_t
# define TEXTEDIT_UCHAR                   wint_t
# define TEXTEDIT_ISALPHA(a)              iswalpha((a))
# define TEXTEDIT_TOUPPER(a)              towupper((a))
# define TEXTEDIT_STRNCPY(a, b, c)        wcsncpy((a), (b), (c))
# define TEXTEDIT_MEMCPY(a, b, c)         wmemcpy((a), (b), (c))

#else /* ! CONFIG_TEXTEDIT_WCHART */

# define TEXTEDIT_CHAR                    char
# define TEXTEDIT_UCHAR                   u8
# define TEXTEDIT_ISALPHA(a)              isalpha((a))
# define TEXTEDIT_TOUPPER(a)              toupper((a))
# define TEXTEDIT_STRNCPY(a, b, c)        strncpy((a), (b), (c))
# define TEXTEDIT_MEMCPY(a, b, c)         memcpy((a), (b), (c))

#endif /* CONFIG_TEXTEDIT_WCHART */

#define FIXED_BUFFER_LEN       4096
#define BUFFER_GROW            256 

/* Widget flags */
#define TEXT_WIDGET_MFC_VALID     (1 << 0)
#define TEXT_WIDGET_ATOMS_VALID   (1 << 1)
#define TEXT_WIDGET_FOCUS         (1 << 2)
#define TEXT_WIDGET_READONLY      (1 << 3)
#define TEXT_WIDGET_FLASH_ON      (1 << 4)
#define TEXT_WIDGET_HAS_SCROLLBAR (1 << 5)

/* Block flags */
#define BLOCK_FLAG_DIRTY          (1 << 0)
#define BLOCK_FLAG_MFC            (1 << 1)

/* Paragraph flags */
#define PARAGRAPH_FLAG_DRAW       (1 << 0)
#define PARAGRAPH_FLAG_DRAW_AFTER (1 << 1)
#define PARAGRAPH_FLAG_DRAW_ALL   PARAGRAPH_FLAG_DRAW | PARAGRAPH_FLAG_DRAW_AFTER | (1 << 2)
#define PARAGRAPH_FLAG_H_INVALID  (1 << 3)

/* Atom flags and mask */
#define ATOM_TYPE_MASK           ((char) (1 << 0) | (1 << 1) | (1 << 2))
#define ATOM_FLAG_LEFT           (1 << 3)
#define ATOM_FLAG_RIGHT          (1 << 4)
#define ATOM_FLAG_DRAW           (1 << 5)
#define ATOM_FLAG_DRAW_LAST_CHAR (1 << 6)
#define ATOM_FLAG_SELECTED       (1 << 7)


typedef enum {
    CURSOR_UP = PGKEY_UP,
    CURSOR_DOWN = PGKEY_DOWN,
    CURSOR_LEFT = PGKEY_LEFT,
    CURSOR_RIGHT= PGKEY_RIGHT,
    CURSOR_PAGE_DOWN = PGKEY_PAGEDOWN,
    CURSOR_PAGE_UP = PGKEY_PAGEUP,
    CURSOR_HOME = PGKEY_HOME,
    CURSOR_END = PGKEY_END
} cursor_direction;



/**
 * The atom type is stored in the low bits of the atom bitfield.
 * Future types are listed for completeness, but only ATOM_TEXT is
 * currently implemented. */
typedef enum {
    ATOM_TEXT = 0,
    ATOM_FONT_START,
    ATOM_FONT_END,
    ATOM_SELECTION_START,
    ATOM_SELECTION_END,
    ATOM_TYPE_LAST
} atom_type;


typedef enum {
    SELECTION_NONE,
    SELECTION_AFTER_CURSOR,
    SELECTION_BEFORE_CURSOR
} selection_type;


#ifndef MAX
#define MAX(a, b)  (a > b ? a : b)
#endif /* max */
#ifndef MIN
#define MIN(a, b)  (a < b ? a : b)
#endif /* min */

#define GET_FLAG(flag_u8, field)   (flag_u8 & ((u8) field))
#define SET_FLAG(flag_u8, field)   (flag_u8 |= field)
#define UNSET_FLAG(flag_u8, field) (flag_u8 -= GET_FLAG(flag_u8, field) ? \
                                    field : 0 )

/* _text_widget renamed to texteditdata to match the convention used in other widgets */
typedef struct texteditdata text_widget;
typedef struct _block block;
typedef struct _paragraph paragraph;
typedef struct _atom atom;
typedef struct _data_block data_block;


struct texteditdata {
    struct widget  * self;
    struct gropctxt  ctx;
    hwrbitmap        bit;
    handle           bit_h;
    hwrcolor         bg, fg, highlight;
    u8               border_h;
    u8               border_v; 

    /* Data being passed to the client */
    struct pgstring *client_data;
    handle client_data_h;
    
    /* Scrollbar */
    u16     thumb_size;
    s16     thumb_top;        /* Top of thumb */
    s16     thumb_drag_start; /* Start of a drag, relative to top of thumb */
    u8      scroll_lock; 

    /* Cursor */
    struct  gropnode * cursor_grop;
    u32     cursor_v_y;      /* Virtual y-coordinate */
    u8      cursor_state;    /* Cursor blink state. > 0 is on */
    s16     cursor_x_stash;  /* Store the cursor's preferred x
                                coordinate when moving up/down. Neg if not
                                valid. */

    /* We currently use only a single font */
    struct font_descriptor *fd;
    
    /* For drawing the widget, we keep track of the updated rect */
    u8      update_clean;
    s16     update_x1, update_y1, update_x2, update_y2;

    /**
     * Logical backend 
     */
    LList * blocks;        /* Head of linked list of blocks */
    u16     width, height; /* Size of viewable widget area */
    u32     v_height;      /* Virtual widget height (px, if rendered
                              into one pane) */
    u32     v_y_top;       /* Y-coordinate of top of visible area in this 
                              pane */

    /* Current block */
    LList * current;        /* Currently edited block */

    /* First visible block */
    LList * fvb;           /* First visible block */
    u32     fvb_v_y;       /* Virtual y-coordinate of first visible
                              block */
    u8      flags;

    /* Is there a current selection */
    selection_type selection;
};


/**
 * Each block stores a fixed-size text array and a linked list of atoms.
 * Blocks are stored in a doubly-linked list.
 */
struct _block {
    size_t          len;       /* Length of text in block */
    TEXTEDIT_CHAR * data; 
    u16             data_size;
    u16             b_gap;
    
    LList * paragraphs;        /* Paragraphs in block */
    LList * cursor_paragraph;  /* Paragraph containing cursor. NULL if none */ 
    LList * cursor_atom;       /* Atom before cursor. NULL if none */

    u8      flags; 
};


struct _paragraph {
    LList * atoms;           /* Linked list of atoms */
    LList * dirty;           /* First dirty atom in list. If not NULL, all
                              * atoms after dirty atom need to be redrawn. */
    size_t  len;             /* Length (chars) */
    size_t  height;          /* Height (pixels) */
    u8      flags;
};


struct _atom {
    u16     len;        /* Length of text (chars) */
    u16     width;      /* Width when rendered */
    u16     height;     /* Height when rendered */
    u8      flags;      
};



/**
 * Translate generic requests to specific PicoGUI actions (textedit.c)
 */
void textedit_draw_str    ( struct widget * self,
                            TEXTEDIT_UCHAR * txt,
                            size_t len,
                            s16 x, 
                            s16 y,
                            u8 highlight );
void textedit_str_size    ( struct widget * self,
                            TEXTEDIT_UCHAR * txt,
                            size_t len,
                            s16 * w, 
                            s16 * h );
void textedit_clear_rect  ( struct widget * self, 
                            s16 x, s16 y,
                            s16 w, s16 h );
void textedit_move_cursor ( struct widget * self,
                            s16 x, s16 y, s16 h );
void textedit_char_size   ( struct widget * self,
                            TEXTEDIT_CHAR ch,
                            s16 * w, 
                            s16 * h );
g_error textedit_set_font ( struct widget * self,
                            handle font );
void textedit_scrollevent ( struct widget * self );


/**
 * Back-end services (textedit_logical.c)
 */
g_error text_backend_init            ( text_widget * widget );
void    text_backend_destroy         ( text_widget * widget );
g_error text_backend_build           ( text_widget * widget,
                                       s16 w,
                                       s16 h );
void    text_backend_set_v_top       ( text_widget * widget,
                                       u32 v_top );
g_error text_backend_set_text        ( text_widget * widget,
                                       struct pgstring * text );
g_error text_backend_set_selection   ( text_widget * widget,
                                       struct pgstring * text );
g_error text_backend_save            ( text_widget * widget );
g_error text_backend_store_selection ( text_widget * widget );
g_error text_backend_insert_char     ( text_widget * widget,
                                       TEXTEDIT_UCHAR ch ); 
g_error text_backend_delete_char     ( text_widget * widget );
g_error text_backend_cursor_move_dir ( text_widget * widget,
                                       cursor_direction dir );
g_error text_backend_cursor_move_xy  ( text_widget * widget,
                                       u16 x, 
                                       u16 y );
g_error text_backend_selection_unset ( text_widget * widget );
g_error text_backend_selection_dir   ( text_widget * widget,
                                       cursor_direction dir );
g_error text_backend_selection_xy    ( text_widget * widget,
                                       u16 x, 
                                       u16 y);


#ifdef TEXTEDIT_DEBUG
void    print_data                   ( text_widget * widget );
void    print_tree                   ( text_widget * widget );
void    print_block                  ( text_widget * widget,
                                       LList * ll_b );
void    print_string                 ( u8 * txt,
                                       size_t len );
#endif /* TEXTEDIT_DEBUG */

#endif /* __TEXT_H__ */
