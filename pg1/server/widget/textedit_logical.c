/* $Id$
 *
 * textedit_logical.c - Backend for multi-line text widget. This
 * defines the behavior of a generic wrapping text widget, and is not
 * tied specifically to PicoGUI. 
 *
 * This is an early version which is far from complete.
 * 
 * An overview is given in textedit_frontend.c. 
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
 *   June 2002
 *   Chuck Groom (cgroom@bluemug.com) - Original Version
 *   
 *   July 2002
 *   Philippe Ney - patch to fix backspace on last char bug
 *
 *   October 2002
 *   Philippe Ney - global text clipboard to all instance of the widget.
 *                  Embryo of the future pgserver global clipboard support,
 *                - support for ctrl-[xcv] keyboard event.
 *                - unicode support through wchar_t data format for
 *                  manipulations and utf-8 for communication
 *   
 *   October 28, 2002
 *   Chuck Groom  - Cleaned up memory allocation, error handling
 *                - Scrolling support
 */

#include <pgserver/common.h>     /* for g_malloc, g_free */
#include <pgserver/textedit.h>
#include <pgserver/widget.h>
#include <assert.h>

#ifdef CONFIG_TEXTEDIT_WCHART
# include <iconv.h>
#endif

#define BLOCK(l)     ((block *)      llist_data(l))
#define PARAGRAPH(l) ((paragraph *)  llist_data(l))
#define ATOM(l)      ((atom *)       llist_data(l))

/**
 * Clipboard 
 *
 * NOTE: This is a temporary kludge until the picogui server has a 
 * standardized clipboard.
 */
/* the text clipboard itself. Global to all instances of the widget */
static struct pgstring * textedit_clipboard = NULL;
/* number of textedit widgets that could use the clipboard */
static int               clipboard_client = 0;


/** 
 * Widget methods
 */
static void    widget_render           ( text_widget * widget );
static void    widget_scroll_to_cursor ( text_widget * widget );
static u32     widget_cursor_v_y       ( text_widget * widget );
static void    widget_set_v_top        ( text_widget * widget,
                                         u32 v_top );
static void    widget_cursor_stay_on   ( text_widget * widget );
static g_error widget_move_cursor_dir  ( text_widget * widget,
                                         cursor_direction dir,
                                         u8 unset_selection,
                                         u8 unite );
static g_error widget_move_cursor_xy  ( text_widget * widget,
                                        u16 x, 
                                        u16 y,
                                        u8 unset_selection,
                                        u8 unite );
static g_error widget_move_cursor      ( text_widget * widget,
                                         LList * ll_new_block,
                                         size_t offset,
                                         u8 unite);
static void    widget_selection_to_cursor ( text_widget * widget,
                                            LList * b_ll,
                                            LList * p_ll,
                                            LList * a_ll,
                                            u16 len,
                                            u8 sel_up);
static void    widget_unset_selection  ( text_widget * widget );
static g_error widget_clear_selection  ( text_widget * widget );
static g_error widget_insert_chars ( text_widget * widget,
                                     TEXTEDIT_UCHAR * str,
                                     u32 len );
static g_error widget_delete_chars ( text_widget * widget,
                                     LList * ll_b,
                                     LList * ll_p,
                                     LList * ll_a,
                                     u32 offset,
                                     u32 len );

/**
 * Block methods 
 */
static g_error block_create          ( block ** b );
static void    block_destroy         ( block * b );
static void    block_set_bgap        ( block * b, 
                                       u16 offset );
static TEXTEDIT_CHAR * block_str_at  ( block * b,
                                       u16 l_offset );
static void    block_string_size     ( text_widget * widget,
                                       block * b,
                                       u16 offset,
                                       u16 len,
                                       s16 * w,
                                       s16 * h );
static g_error block_data_resize     ( text_widget * widget,
                                       block * b,
                                       u16 new_size );
static g_error block_shed_last_para  ( text_widget * widget,
                                       LList * ll_b );
#define        block_char_at(b,o)    (*block_str_at(b,o))
#define        block_char_size(wi,b,o,w,h) (textedit_char_size(wi->self, \
                                            *block_str_at(b, o), w, h))

/**
 * Paragraph methods 
 */
static g_error     paragraph_create   ( paragraph **p );
static void        paragraph_destroy  ( paragraph * p);


/**
 * Atom methods 
 */
static g_error     atom_create        ( atom ** a,
                                        atom_type type, 
                                        u8 flags );
static atom_type   atom_get_type      ( atom * a );
static void        atom_destroy       ( atom * a );

/**
 * If the atom a_ll is not the end of a line, try joining it with the
 * next text atom
 */
static void        join_atom_next     ( text_widget * widget,
                                        LList * a_ll );
/**
 * Break apart the atom a_ll such that the first chunk is of len 
 */
static g_error     split_atom         ( text_widget * widget,
                                        block * b,
                                        LList * a_ll,
                                        size_t offset,
                                        size_t len );
/**
 * Get whatever atom follows a_ll, even though it may be in the
 * next para or block
 */
static LList *     next_atom          ( LList * b_ll,
                                        LList * p_ll,
                                        LList * a_ll,
                                        LList ** next_b_ll,
                                        LList ** next_p_ll);
/**
 * Get whatever atom comes before a_ll, even though it may be in the
 * prev para or block
 */
static LList *     prev_atom          ( LList * b_ll,
                                        LList * p_ll,
                                        LList * a_ll,
                                        LList ** prev_b_ll,
                                        LList ** prev_p_ll);

/**
 * Wrapping fun. wrap() checks to see whether we need to wrap; rewrap()
 * does the work of wrapping.
 */
static g_error wrap                   ( text_widget * widget,
                                        block * b,
                                        paragraph * p, 
                                        LList * a_ll );
static g_error rewrap                 ( text_widget * widget,
                                        block * b,
                                        paragraph * p, 
                                        LList * a_ll, 
                                        u16 offset,
                                        u16 p_offset );
#define        break_char(ch)  ((ch == '-') || (ch == ' ') || (ch == '\t'))


#ifdef CONFIG_TEXTEDIT_WCHART
/**
 * conversion functions UTF-8 <-> UCS-4
 */
int utf8ToWchart (char * inBuf, int inNbChars, wchar_t * outBuf)
{
    iconv_t ihandle;
    size_t iSize, oSize;
    char * oBufp = (char *) outBuf;

    /* define useful size */
    iSize = strlen (inBuf);
    oSize = inNbChars * 4;    /* each chars use 32 bits */

    /* create conversion handle */
    ihandle = iconv_open ("WCHAR_T", "UTF-8");
    if (ihandle == (iconv_t) -1) {
      /* Something went wrong.  */
      if (errno == EINVAL) {
        printf (__FILE__ " : conversion not available (UTF-8 to wchar_t)\n");
      }
      else {
        printf (__FILE__ " : error iconv_open : %s\n", strerror (errno));
      }
      /* Terminate the output string.  */
      *outBuf = L'\0';
      exit (1);
    }

    /* convert */
    iconv (ihandle, & inBuf, & iSize, & oBufp, & oSize);
    outBuf [inNbChars] = L'\0';

    /* release conversion handle */
    iconv_close (ihandle);

    return 0;
}


/**
 * convert a wchar_t encoding string to an utf-8 encoding
 * the availlable bytes remaining in the output buffer is stored in
 * the outSize parameter
 */
int wchartToUtf8 (wchar_t * inBuf, char * outBuf, int * outSize)
{
    iconv_t ihandle;
    size_t iSize;
    char * iBufp = (char *) inBuf;
    char * keepp = outBuf;      /* keep a ref to the beginning of the buffer */

    /* define useful size */
    iSize = wcslen (inBuf) * 4;

    /* create conversion handle */
    ihandle = iconv_open ("UTF-8", "WCHAR_T");
    if (ihandle == (iconv_t) -1) {
      /* Something went wrong.  */
      if (errno == EINVAL) {
        printf (__FILE__ " : conversion not available (wchar_t to utf-8)\n");
      }
      else {
        printf (__FILE__ " : error iconv_open : %s\n", strerror (errno));
      }
      /* Terminate the output string.  */
      *outBuf = '\0';
      exit (1);
    }

    /* convert */
    iconv (ihandle, & iBufp, & iSize, & outBuf, outSize);
    /* terminate string.
     * the outBuf pointer is set to the first free room by iconv
     */
    *outBuf = '\0';
    /* re-position the pointer */
    outBuf = keepp;

    /* release conversion handle */
    iconv_close (ihandle);

    return 0;
}
#endif /* CONFIG_TEXTEDIT_WCHART */


g_error text_backend_init ( text_widget * widget ) {
    g_error e;
    LList * l;
    paragraph * p;
    block * b;
    
    assert(widget);

    /* clipboard client registration */
    clipboard_client++;

    /* Destroy old blocks, if any */
    for (l = widget->blocks; l; l = llist_next(l)) 
        block_destroy(BLOCK(l));
    llist_free(widget->blocks);

    e = block_create(&b);
    errorcheck;

    e = paragraph_create(&p);
    errorcheck;

    e = llist_append(&b->paragraphs, NULL, p);
    errorcheck;

    b->cursor_paragraph = b->paragraphs;
    b->cursor_atom = PARAGRAPH(b->paragraphs)->atoms;

    e = llist_append(&widget->blocks, widget->blocks, b);
    errorcheck;

    widget->current = widget->blocks;
    widget->fvb = widget->blocks;
    widget->fvb_v_y = 0;
    widget->border_h = 2;
    widget->border_v = 2;
    widget->selection = SELECTION_NONE;
    
    widget->client_data = NULL;
    widget->client_data_h = 0;
    return success;
}


void text_backend_destroy (  text_widget * widget ) {
    LList * l;
    assert(widget);
    for (l = widget->blocks; l; l = llist_next(l))
        block_destroy(BLOCK(l));
    llist_free(widget->blocks);

    if (widget->client_data_h)
      handle_free(widget->self->owner, widget->client_data_h);

    /* clipboard client deregistration */
    clipboard_client--;
    /* destroy clipboard */
    if (!clipboard_client && textedit_clipboard) {
        pgstring_delete (textedit_clipboard);
        textedit_clipboard = NULL;
    }
}


/**
 * (Re)build the text backend. This sets the width and height, rewraps,
 * and renders. Simply returns if the widget isn't ready to be drawn. */
g_error text_backend_build ( text_widget * widget,
                             s16 w,
                             s16 h ) {
    g_error e;
    LList * ll_b, * ll_p;
    u16 p_offset;
    struct font_metrics m;
    
    if (! ((widget->fd) && (widget->bit)) ) 
        return;

    /* Refuse to do anything if width or height is less than size of
       font */
    widget->fd->lib->getmetrics(widget->fd,&m);
    if ((w < m.charcell.w) || (h < m.charcell.h))
        return;

    widget->width = w;
    widget->height = h;
    widget->v_height = 0;
    widget->v_y_top = 0;
    widget->cursor_x_stash = -1;
    widget->cursor_v_y = 0;

    textedit_clear_rect(widget->self, 0, 0, 
                        widget->width, widget->height);
    
    /* Wrap all */
    for (ll_b = widget->blocks; ll_b; ll_b = llist_next(ll_b)) {
        for (ll_p = BLOCK(ll_b)->paragraphs, p_offset = 0; ll_p; 
             ll_p = llist_next(ll_p)) {
            e = rewrap(widget, BLOCK(ll_b), PARAGRAPH(ll_p), 
                       PARAGRAPH(ll_p)->atoms, p_offset, p_offset);
            errorcheck;
            p_offset += PARAGRAPH(ll_p)->len;
        }
    }
    widget_cursor_v_y(widget);
    widget_render(widget);
    return success;
}


g_error text_backend_set_text ( text_widget * widget,
                                struct pgstring * text ) {
    g_error e;
    LList * ll_b, * ll_p, * ll_a;
    size_t para_len;
    block * b;
    paragraph * p;
    atom * a;
#ifdef CONFIG_TEXTEDIT_WCHART
    u32 len;
    wchar_t txtBuf [text->num_chars + 1];
    wchar_t * txt = txtBuf;
    int cnt = 0;
    size_t n;
    char * pBuf = (char *) text->buffer;

    utf8ToWchart (pBuf, text->num_chars, txt);

#else
    u8 * txt = text->buffer;
    u16 len = text->num_bytes;
#endif

    /* Destroy everything in widget */
    for (ll_b = widget->blocks; ll_b; ll_b = llist_next(ll_b))
        block_destroy(BLOCK(ll_b));
    llist_free(widget->blocks);
    widget->blocks = NULL;

    e = block_create(&b);
    errorcheck;
    
    b->b_gap = 0;
    e = llist_append(&widget->blocks, widget->blocks, b);
    errorcheck;
    
#ifdef CONFIG_TEXTEDIT_WCHART
    len = text->num_chars;
    while (len) {
        for (para_len = 0;
             (para_len < len) && (txt[para_len] != L'\n'); 
             para_len++) {
	  ;            /* noop, just parse the paragraph */
	}
#else
    while (len) {
        for (para_len = 0;
             (para_len < len) && (txt[para_len] != L'\n'); 
             para_len++) {
	  ;            /* noop, just parse the paragraph */
	}
#endif

	/* test carriage return only if we don't reach the end of the string */
	if (para_len < len) {
#ifdef CONFIG_TEXTEDIT_WCHART
	  if (txt[para_len] == L'\n') {
#else
	  if (txt[para_len] == '\n') {
#endif
            para_len++;
	  }
	}

        while (para_len + 1 > b->data_size - b->len) {
            /* Paragraph won't fit into b */
            if (b->len) {
                e = block_create(&b);
                errorcheck;
                
                e = llist_append(&widget->blocks, widget->blocks, b);
                errorcheck;
            } else { 
                /* Single long para */
                e = block_data_resize (widget, b, b->len + para_len + BUFFER_GROW);
                errorcheck;
            }
        }
        TEXTEDIT_STRNCPY(b->data + b->b_gap, txt, para_len);
        e = paragraph_create(&p);
        errorcheck;

        e = llist_append(&b->paragraphs, b->paragraphs, p);
	errorcheck;

        p->len = para_len;
        len -= para_len;
        b->len += para_len;
        b->b_gap += para_len;
        ATOM(p->atoms)->len = para_len;
        txt += para_len;
    }

    /* Set cursor */
    widget->fvb = widget->current = widget->blocks;
    widget->v_y_top = 0;
    widget->cursor_v_y  = 0;

    b = BLOCK(widget->current);
    b->cursor_paragraph = b->paragraphs;
    block_set_bgap(b, 0);
    p = PARAGRAPH(b->cursor_paragraph);
    UNSET_FLAG(ATOM(p->atoms)->flags, ATOM_FLAG_LEFT);
    e = atom_create(&a, ATOM_TEXT, ATOM_FLAG_LEFT);
    errorcheck;

    e = llist_prepend(&p->atoms, p->atoms, a);
    errorcheck;
    b->cursor_atom = p->atoms;

    e = text_backend_build(widget, widget->width, widget->height);
    errorcheck;
    return success;
}


g_error text_backend_set_selection ( text_widget * widget,
                                     struct pgstring * text ) {
    g_error e;
#ifdef CONFIG_TEXTEDIT_WCHART
    wchar_t * wBuffer;

    if (!text->buffer || text->num_chars == 0)
      return success;

    SET_FLAG(PARAGRAPH(BLOCK(widget->current)->cursor_paragraph)->flags,
             PARAGRAPH_FLAG_DRAW_ALL);
    SET_FLAG(ATOM(BLOCK(widget->current)->cursor_atom)->flags,
             ATOM_FLAG_DRAW);

    /* allocate wchar_t buffer as 4 bytes/char */
    wBuffer = malloc ((text->num_chars + 1) * 4);

    /* convert text->buffer to a wchar_t string */
    utf8ToWchart ((char *) text->buffer, text->num_chars, wBuffer);

    /* insert chars */
    e = widget_insert_chars (widget, (wint_t *) wBuffer, wcslen (wBuffer));
    errorcheck;

    /* free memory */
    free (wBuffer);

#else /* CONFIG_TEXTEDIT_WCHART */

    SET_FLAG(PARAGRAPH(BLOCK(widget->current)->cursor_paragraph)->flags,
             PARAGRAPH_FLAG_DRAW_ALL);
    SET_FLAG(ATOM(BLOCK(widget->current)->cursor_atom)->flags,
             ATOM_FLAG_DRAW);

    e = widget_insert_chars(widget, text->buffer, text->num_bytes);
    errorcheck;
#endif /* CONFIG_TEXTEDIT_WCHART */
    return success;
}


g_error text_backend_insert_char ( text_widget * widget,
                                TEXTEDIT_UCHAR ch ) {
    return widget_insert_chars(widget, &ch, 1);
}
 

g_error text_backend_delete_char  ( text_widget * widget ) {
    g_error e; 
    block * b;
    paragraph * p;
    atom * a;
    LList *ll_b, * ll_p, * ll_a;
    u32 offset;

    /* Invalidate cursor's preferred x position */
    widget->cursor_x_stash = -1;

    if (widget->selection == SELECTION_NONE) {
        for (offset = 0, ll_p = BLOCK(widget->current)->paragraphs;
             ll_p != BLOCK(widget->current)->cursor_paragraph;
             ll_p = llist_next(ll_p))
            offset += PARAGRAPH(ll_p)->len;
        for (ll_a = BLOCK(widget->current)->cursor_atom;
             ll_a;
             ll_a = llist_prev(ll_a)) 
            offset += ATOM(ll_a)->len;
        
        e = widget_delete_chars(widget, 
                                widget->current,
                                BLOCK(widget->current)->cursor_paragraph,
                                BLOCK(widget->current)->cursor_atom,
                                offset, 
                                1);
        errorcheck;
    } else {
        e = widget_clear_selection(widget);
        errorcheck;
    }
    
    widget_cursor_v_y (widget);
    widget_scroll_to_cursor(widget);
    widget_render(widget); 
    widget_cursor_stay_on (widget);
    return success;
}


g_error text_backend_cursor_move_dir ( text_widget * widget,
                                     cursor_direction dir ) {
    return widget_move_cursor_dir (widget, dir, TRUE, TRUE);
}


g_error text_backend_cursor_move_xy ( text_widget * widget,
                                      u16 x, 
                                      u16 y ) {
    return widget_move_cursor_xy (widget, x, y, TRUE, TRUE);
}


g_error text_backend_cut_copy_paste ( text_widget * widget,
                                      char ch ) {
    g_error e;

    switch (ch) {
    case PGKEY_x:
        if (widget->selection != SELECTION_NONE) {
            /* copy the selection */
            e = text_backend_cut_copy_paste (widget, PGKEY_c);
            errorcheck;
            /* delete the selection */
            e = text_backend_delete_char (widget);
            errorcheck;
        }
        break;
        
    case PGKEY_c:
        /* store selection in the clipboard */
        e = text_backend_store_selection (widget);
        errorcheck;
        break;
        
    case PGKEY_v:
        if (textedit_clipboard) {
            /* set the selection from the clipboard */
            e = text_backend_set_selection (widget, textedit_clipboard);
            errorcheck;
        }
        break;
        
    default:
        break;
    }
    return success;
}
 

/**
 * Set the virtual top of the visible region within the virtual pane.
 * This is called when the scrollbar is moved. Note two things: first,
 * the actual virtual top may be shifted a bit to line-align the top;
 * second, if the cursor isn't in the virtual pane, we move it outside
 * the visible region.
 */
void text_backend_set_v_top ( text_widget * widget,
                              u32 v_top ) {
    g_error e;
    v_top = MIN(v_top, widget->v_height - (widget->height - 2*widget->border_v));
    widget_set_v_top (widget, v_top);
    if ((widget->cursor_v_y < widget->v_y_top) || 
        (widget->cursor_v_y >= widget->v_y_top + (widget->height - 2*widget->border_v))) {
        /* Hide cursor if off-screen */
        textedit_move_cursor (widget->self, (s16) widget->cursor_grop->r.x,
                              widget->height,
                              widget->cursor_grop->r.h);
    }
    widget_render (widget);
}


/**
 * Copy broken-up bits of data in block buffers to a unified single
 * buffer, widget->data. This is used for passing widget text to the
 * client. The function name is possibly misleading, and may be changed.
 */
g_error text_backend_save ( text_widget * widget ) {
    u32 len, k;
    LList * ll_b;
    g_error e;
    u16 gap_len;
    block * b;
    TEXTEDIT_CHAR * iBuf;  /* buffer to re-arrange the text */

    if (widget->client_data_h)
      handle_free(widget->self->owner, widget->client_data_h);
    widget->client_data = NULL;
    widget->client_data_h = 0;

    /* determine the length of the text */
    for (len = 0, ll_b = widget->current; ll_b; ll_b = llist_next(ll_b))
      len += BLOCK(ll_b)->len;

    /* allocate buffer
     * it will be use as input buffer for iconv ifdef CONFIG_TEXTEDIT_WCHART
     */
    iBuf = malloc (sizeof (TEXTEDIT_CHAR) * (len + 1));

    /* fulfill the buffer */
    for (k = 0, ll_b = widget->current; ll_b; ll_b = llist_next(ll_b)) {
      b = BLOCK(ll_b);
      gap_len = b->data_size - b->len;
      TEXTEDIT_MEMCPY(iBuf + k, b->data, b->b_gap);
      k += b->b_gap;
      TEXTEDIT_STRNCPY(iBuf + k, b->data + b->b_gap + gap_len,  
		       b->len - b->b_gap);
      k += b->len - b->b_gap;
    }
    /* null terminate the buffer */
    iBuf [len] = '\0';


#ifdef CONFIG_TEXTEDIT_WCHART /* we need to convert the buffer to utf-8 */
    {
      char * oBuf;
      int oSize = len * 6;
      int keepSize = oSize;

      /* allocate the max possible buffer for iconv output encoded utf-8 */
      oBuf = malloc (oSize);

      /* convert */
      wchartToUtf8 (iBuf, oBuf, & oSize);

      /* oSize is the number of free rooms in the output buffer. Then the real
       * length of the string is the following:
       */
      keepSize -= oSize;

      pgstring_new (& widget->client_data, PGSTR_ENCODE_UTF8, keepSize, oBuf);
      errorcheck;

      /* free memory */
      free (oBuf);
    }

#else  /* ! CONFIG_TEXTEDIT_WCHART */

    pgstring_new (& widget->client_data, PGSTR_ENCODE_UTF8, len, iBuf);
    errorcheck;

#endif /* CONFIG_TEXTEDIT_WCHART */

    /* make the handle to pass to client applications */
    e = mkhandle (& widget->client_data_h, PG_TYPE_PGSTRING,
		  widget->self->owner, widget->client_data);
    errorcheck;

    /* free memory */
    free (iBuf);

    return success;
}


g_error text_backend_store_selection ( text_widget * widget) {
    g_error e;
    u32 len, offset, k, b_gap;
    LList * ll_b, * ll_p, * ll_a, * temp_ll;
    block * b;

    if (textedit_clipboard)
        pgstring_delete(textedit_clipboard);
    textedit_clipboard = NULL;

    if (widget->selection == SELECTION_NONE) {
        e = pgstring_new(&textedit_clipboard, PGSTR_ENCODE_UTF8, 0, NULL);
        errorcheck;
        return success;
    }

    ll_b = widget->current;
    ll_p = BLOCK(widget->current)->cursor_paragraph;
    ll_a = BLOCK(widget->current)->cursor_atom;
    len = 0;
    if (widget->selection == SELECTION_BEFORE_CURSOR) {
        for ( ; 
              ll_a && GET_FLAG(ATOM(ll_a)->flags, ATOM_FLAG_SELECTED); 
              ll_a = prev_atom (ll_b, ll_p, ll_a, &ll_b, &ll_p))
            len += ATOM(ll_a)->len;
        if (ll_a) {
            ll_a = next_atom (ll_b, ll_p, ll_a, &ll_b, &ll_p);
        } else {
            ll_b = widget->blocks;
            ll_p = BLOCK(ll_b)->paragraphs;
            ll_a = PARAGRAPH(ll_p)->atoms;
        }
    } else {
        /* Selection after cursor */
        for ( ll_a = next_atom (ll_b, ll_p, ll_a, &ll_b, &ll_p); 
              ll_a && GET_FLAG(ATOM(ll_a)->flags, ATOM_FLAG_SELECTED); 
              ll_a = next_atom (ll_b, ll_p, ll_a, &ll_b, &ll_p))
            len += ATOM(ll_a)->len;
        ll_b = widget->current;
        ll_p = BLOCK(widget->current)->cursor_paragraph;
        ll_a = BLOCK(widget->current)->cursor_atom;
        ll_a = next_atom (ll_b, ll_p, ll_a, &ll_b, &ll_p);
    }
//    pgstring_new(&textedit_clipboard, PGSTR_ENCODE_UTF8, len, NULL);

    /* ll_a is the first selected atom */
    temp_ll = ll_a;
    ll_p = BLOCK(ll_b)->paragraphs;
    ll_a = PARAGRAPH(ll_p)->atoms;
    for (offset = 0; 
         ll_a != temp_ll; 
         ll_a = next_atom (ll_b, ll_p, ll_a, &ll_b, &ll_p)) {
        offset += ATOM(ll_a)->len;
    }
    b = BLOCK(ll_b);
    b_gap = b->b_gap;
    block_set_bgap(b, b->len);
    k = MIN(len, b->len - offset);

#ifdef CONFIG_TEXTEDIT_WCHART
    {
      char * oBuf;
      wchar_t * iBuf;
      int oSize = k * 6;
      int keepSize = oSize;

      /* allocate the max possible buffer */
      oBuf = malloc (oSize);
 
      /* copy the characters to re-encode to a null terminated string */
      iBuf = malloc (sizeof (wchar_t) * (k + 1));
      wcsncpy (iBuf, b->data + offset, k);
      iBuf [k] = L'\0';

      /* convert */
      wchartToUtf8 (iBuf, oBuf, & oSize);

      /* oSize is the number of free rooms in the output buffer. Then the real
       * length of the string is the following:
       */
      keepSize -= oSize;

      e = pgstring_new (&textedit_clipboard, PGSTR_ENCODE_UTF8, keepSize, oBuf);
      errorcheck;
//	memcpy (textedit_clipboard->buffer, oBuf, k);
        
      /* free memory */
      free (oBuf);
      free (iBuf);
    }
#else
    e = pgstring_new (&textedit_clipboard, PGSTR_ENCODE_UTF8, k, b->data + offset);
    errorcheck;
//	  memcpy(textedit_clipboard->buffer, b->data + offset, k);
#endif

//    len -= k;
//    block_set_bgap(b, b_gap);
//    while (len) {
//	    ll_b = llist_next(ll_b);
//	    b = BLOCK(ll_b);
//	    b_gap = b->b_gap;
//	    block_set_bgap(b, b->len);
//
//
//#ifdef CONFIG_TEXTEDIT_WCHART
//	  {
//	    char * cBuf;
//	    int oSize = wcslen (b->data) * 6;
//
//	    /* allocate the max possible buffer */
//	    cBuf = malloc (oSize);
// 
//	    /* convert */
//	    wchartToUtf8 (b->data, cBuf, & oSize);
//
//	    memcpy(textedit_clipboard->buffer + k, cBuf, MIN(len, b->len));
//
//	    /* free memory */
//	    free (cBuf);
//	  }
//#else
//	    memcpy(textedit_clipboard->buffer + k, b->data, MIN(len, b->len));
//#endif
//
//	    k += MIN(len, b->len);
//	    len -= MIN(len, b->len);
//	    block_set_bgap(b, b_gap);
//    }
    return success;
}

g_error text_backend_selection_unset ( text_widget * widget ) {
    widget_unset_selection(widget);
    return success;     
}

g_error text_backend_selection_dir ( text_widget * widget,
                                  cursor_direction dir ) {
    g_error e;
    LList * a_ll, * b_ll, * p_ll;
    u16 len;
    u8 sel_up;
    
    switch (dir) {
    case CURSOR_LEFT:
    case CURSOR_UP:
    case CURSOR_PAGE_UP:
    case CURSOR_HOME:
        sel_up = TRUE;
        break;
    default:
        sel_up = FALSE;
    }
    
    b_ll = widget->current;
    p_ll = BLOCK(b_ll)->cursor_paragraph;
    a_ll = BLOCK(b_ll)->cursor_atom;
    len = ATOM(a_ll)->len;

    e = widget_move_cursor_dir(widget, dir, FALSE, FALSE);
    errorcheck;
    widget_selection_to_cursor ( widget,
                                 b_ll,
                                 p_ll,
                                 a_ll, 
                                 len, 
                                 sel_up);
    return success;
}


/* Move the cursor. Find out if this was before or after the
   prev. cursor locaion. */
g_error text_backend_selection_xy ( text_widget * widget,
                                 u16 x, 
                                 u16 y) {
    g_error e;
    LList * b_ll, * p_ll, * a_ll; /* The old cursor position */
    u16 len; /* Old cursor atom len */
    LList * n_b_ll, * n_p_ll, * n_a_ll; /* Scan forward from new cursor */
    LList * o_b_ll, * o_p_ll, * o_a_ll; /* Scan forward from old cursor */
    u8 sel_up;
    
    o_b_ll = b_ll = widget->current;
    o_p_ll = p_ll = BLOCK(b_ll)->cursor_paragraph;
    o_a_ll = a_ll = BLOCK(b_ll)->cursor_atom;
    len = ATOM(a_ll)->len;
    
    e = widget_move_cursor_xy(widget, x, y, FALSE, FALSE);
    errorcheck;

    n_b_ll = widget->current;
    n_p_ll = BLOCK(b_ll)->cursor_paragraph;
    n_a_ll = BLOCK(b_ll)->cursor_atom;
    
    /* Scan forward from both new and old cursor positions. If the
     * old cursor scan finds the new cursor, the selection moved 
     * right/down; if the new cursor scan finds the old cursor, the 
     * selection moves up/left. */
    while ( o_a_ll && n_a_ll && 
            (o_a_ll != BLOCK(b_ll)->cursor_atom) &&
            (n_a_ll != a_ll) ) {
        o_a_ll = next_atom (o_b_ll, o_p_ll, o_a_ll, &o_b_ll, &o_p_ll);
        n_a_ll = next_atom (n_b_ll, n_p_ll, n_a_ll, &n_b_ll, &n_p_ll);
    }
    if (!o_a_ll || (n_a_ll == a_ll)) {
        sel_up = TRUE;
    } else {
        sel_up = FALSE;
    }
    widget_selection_to_cursor ( widget,
                                 b_ll,
                                 p_ll,
                                 a_ll, 
                                 len, 
                                 sel_up);
    return success;
}


/* Grow or shrink the selection from the old cursor position (b_ll,
 * p_ll, a_ll) to the current cursor position. Sel_up means the new cursor
 * is to the left or above teh old cursor.
 *
 * There are four possibilities:
 *  1. Cursor does not change position (start/end of file)
 *  2. There is no current selection: move the cursor and mark the area
 *     selected.
 *  3. Grow selection; split atoms.
 *  4. Shrink selection (possible to no selection); unite atoms
 */
static void widget_selection_to_cursor ( text_widget * widget,
                                         LList * b_ll,
                                         LList * p_ll,
                                         LList * a_ll,
                                         u16 len,
                                         u8 sel_up) {
    LList * next_ll, * prev_ll;
    u8 grow;
        
    if ((BLOCK(widget->current)->cursor_atom == a_ll) && 
        (ATOM(BLOCK(widget->current)->cursor_atom)->len == len)) {
        /* The cursor didn't move, which means that we're at the start 
           or end. */
        return;
    }

    next_ll = next_atom(widget->current,
                        BLOCK(widget->current)->cursor_paragraph,
                        BLOCK(widget->current)->cursor_atom,
                        NULL, NULL);
    prev_ll = prev_atom(widget->current,
                        BLOCK(widget->current)->cursor_paragraph,
                        BLOCK(widget->current)->cursor_atom,
                        NULL, NULL);


    if (widget->selection == SELECTION_NONE) {
        if (sel_up) {
            /* The cursor will be at the head of the selection.
             *
             * Set every atom between the current cursor and starting
             * cursor position to SELECTED */
            widget->selection = SELECTION_AFTER_CURSOR;
            if (a_ll == BLOCK(widget->current)->cursor_atom)
                a_ll = next_ll;
            for (;  a_ll && a_ll != BLOCK(widget->current)->cursor_atom;
                 a_ll = prev_atom(b_ll, p_ll, a_ll, &b_ll, &p_ll)) {
                SET_FLAG(ATOM(a_ll)->flags, ATOM_FLAG_SELECTED);
                SET_FLAG(ATOM(a_ll)->flags, ATOM_FLAG_DRAW);
                SET_FLAG(PARAGRAPH(p_ll)->flags, PARAGRAPH_FLAG_DRAW);
            }
        } else {
            /* Cursor right, down, page down, end... */
            /* The cursor will be at the tail of the selection */
            widget->selection = SELECTION_BEFORE_CURSOR;
            for( a_ll = next_atom(b_ll, p_ll, a_ll, &b_ll, &p_ll);
                 a_ll;
                 a_ll = next_atom(b_ll, p_ll, a_ll, &b_ll, &p_ll)) {
                SET_FLAG(ATOM(a_ll)->flags, ATOM_FLAG_SELECTED);
                SET_FLAG(ATOM(a_ll)->flags, ATOM_FLAG_DRAW);
                SET_FLAG(PARAGRAPH(p_ll)->flags, PARAGRAPH_FLAG_DRAW);
                if (a_ll == BLOCK(widget->current)->cursor_atom)
                    break;
            }
        }
    } else if (widget->selection == SELECTION_AFTER_CURSOR) {
        if (sel_up) {
            /* Grow selection from cursor, going forward until we find
               the old selection */
            b_ll = widget->current;
            p_ll = BLOCK(b_ll)->cursor_paragraph;
            for (a_ll = next_ll;
                 a_ll && !GET_FLAG(ATOM(a_ll)->flags, ATOM_FLAG_SELECTED);
                 a_ll = next_atom(b_ll, p_ll, a_ll, &b_ll, &p_ll)) {
                SET_FLAG(ATOM(a_ll)->flags, ATOM_FLAG_SELECTED);
                SET_FLAG(ATOM(a_ll)->flags, ATOM_FLAG_DRAW);
                SET_FLAG(PARAGRAPH(p_ll)->flags, PARAGRAPH_FLAG_DRAW);
            }
            join_atom_next(widget, next_ll);
        } else {
            /* Selection right/down */
            /* Shrink selection */
             b_ll = widget->current;
             p_ll = BLOCK(b_ll)->cursor_paragraph;

            if (next_ll && 
                !GET_FLAG(ATOM(next_ll)->flags, ATOM_FLAG_SELECTED)) {
                /* We over-shot the start */
                for (a_ll = BLOCK(b_ll)->cursor_atom;
                     a_ll && !GET_FLAG(ATOM(a_ll)->flags, ATOM_FLAG_SELECTED);
                     a_ll = prev_atom(b_ll, p_ll, a_ll, &b_ll, &p_ll)) {
                    SET_FLAG(ATOM(a_ll)->flags, ATOM_FLAG_SELECTED);
                    SET_FLAG(ATOM(a_ll)->flags, ATOM_FLAG_DRAW);
                    SET_FLAG(PARAGRAPH(p_ll)->flags, PARAGRAPH_FLAG_DRAW);
                }
                for (; a_ll && GET_FLAG(ATOM(a_ll)->flags, ATOM_FLAG_SELECTED);
                     a_ll = prev_atom(b_ll, p_ll, a_ll, &b_ll, &p_ll)) {
                    UNSET_FLAG(ATOM(a_ll)->flags, ATOM_FLAG_SELECTED);
                    SET_FLAG(ATOM(a_ll)->flags, ATOM_FLAG_DRAW);
                    SET_FLAG(PARAGRAPH(p_ll)->flags, PARAGRAPH_FLAG_DRAW);
                }
                if (a_ll)
                    join_atom_next(widget, a_ll);
                if (!GET_FLAG(ATOM(BLOCK(widget->current)->cursor_atom)->flags,
                              ATOM_FLAG_SELECTED)) 
                    widget->selection = SELECTION_NONE;
                else 
                    widget->selection = SELECTION_BEFORE_CURSOR;
            } else {
                for (a_ll = BLOCK(widget->current)->cursor_atom;
                     a_ll && GET_FLAG(ATOM(a_ll)->flags, ATOM_FLAG_SELECTED);
                     a_ll = prev_atom(b_ll, p_ll, a_ll, &b_ll, &p_ll)) {
                    UNSET_FLAG(ATOM(a_ll)->flags, ATOM_FLAG_SELECTED);
                    SET_FLAG(ATOM(a_ll)->flags, ATOM_FLAG_DRAW);
                    SET_FLAG(PARAGRAPH(p_ll)->flags, PARAGRAPH_FLAG_DRAW);
                }
                if (a_ll) {
                    join_atom_next(widget, a_ll);
                    SET_FLAG(ATOM(a_ll)->flags, ATOM_FLAG_DRAW);
                    SET_FLAG(PARAGRAPH(p_ll)->flags, PARAGRAPH_FLAG_DRAW);
                }
                a_ll = next_atom(widget->current, 
                                 BLOCK(widget->current)->cursor_paragraph,
                                 BLOCK(widget->current)->cursor_atom,
                                 NULL,
                                 NULL);
                if (!a_ll || !GET_FLAG(ATOM(a_ll)->flags, ATOM_FLAG_SELECTED)) 
                    widget->selection = SELECTION_NONE;
            }
        }
    } else if (widget->selection == SELECTION_BEFORE_CURSOR) {
        if (sel_up) {
            /* Shrink selection. Work forward from cursor. */
            b_ll = widget->current;
            p_ll = BLOCK(b_ll)->cursor_paragraph;
            if (!GET_FLAG(ATOM(BLOCK(b_ll)->cursor_atom)->flags, 
                          ATOM_FLAG_SELECTED)) {
                /* We over-shot the selection start */
                for (a_ll = next_atom(b_ll, p_ll, BLOCK(b_ll)->cursor_atom, &b_ll, &p_ll);
                     a_ll && !GET_FLAG(ATOM(a_ll)->flags, ATOM_FLAG_SELECTED);
                     a_ll = next_atom(b_ll, p_ll, a_ll, &b_ll, &p_ll)) {
                    SET_FLAG(ATOM(a_ll)->flags, ATOM_FLAG_SELECTED);
                    SET_FLAG(ATOM(a_ll)->flags, ATOM_FLAG_DRAW);
                    SET_FLAG(PARAGRAPH(p_ll)->flags, PARAGRAPH_FLAG_DRAW);
                }
                for (; a_ll && GET_FLAG(ATOM(a_ll)->flags, ATOM_FLAG_SELECTED);
                     a_ll = next_atom(b_ll, p_ll, a_ll, &b_ll, &p_ll)) {
                    UNSET_FLAG(ATOM(a_ll)->flags, ATOM_FLAG_SELECTED);
                    SET_FLAG(ATOM(a_ll)->flags, ATOM_FLAG_DRAW);
                    SET_FLAG(PARAGRAPH(p_ll)->flags, PARAGRAPH_FLAG_DRAW);
                }
                if (a_ll && llist_prev(a_ll))
                    join_atom_next(widget, llist_prev(a_ll));
                b_ll = widget->current;
                p_ll = BLOCK(b_ll)->cursor_paragraph;
                a_ll = next_atom(b_ll, p_ll, BLOCK(b_ll)->cursor_atom, NULL, NULL);
                if (a_ll && !GET_FLAG(ATOM(a_ll)->flags, ATOM_FLAG_SELECTED))
                    widget->selection = SELECTION_NONE;
                else 
                    widget->selection = SELECTION_AFTER_CURSOR;
            } else {
                for (a_ll = next_atom(b_ll, p_ll, BLOCK(b_ll)->cursor_atom, &b_ll, &p_ll);
                     a_ll && GET_FLAG(ATOM(a_ll)->flags, ATOM_FLAG_SELECTED);
                     a_ll = next_atom(b_ll, p_ll, a_ll, &b_ll, &p_ll)) {
                    UNSET_FLAG(ATOM(a_ll)->flags, ATOM_FLAG_SELECTED);
                    SET_FLAG(ATOM(a_ll)->flags, ATOM_FLAG_DRAW);
                    SET_FLAG(PARAGRAPH(p_ll)->flags, PARAGRAPH_FLAG_DRAW);
                }
                if (a_ll && llist_prev(a_ll)) 
                    join_atom_next(widget, llist_prev(a_ll));
                if (!GET_FLAG(ATOM(BLOCK(widget->current)->cursor_atom)->flags, 
                              ATOM_FLAG_SELECTED))
                    widget->selection = SELECTION_NONE;
            }
        } else {
            /* Right/down */
            /* Grow selection from cursor, working backwards until we
               find the old selection */
            b_ll = widget->current;
            p_ll = BLOCK(b_ll)->cursor_paragraph;
            for (a_ll = BLOCK(b_ll)->cursor_atom;
                 a_ll && !GET_FLAG(ATOM(a_ll)->flags, ATOM_FLAG_SELECTED);
                 a_ll = prev_atom(b_ll, p_ll, a_ll, &b_ll, &p_ll)) {
                SET_FLAG(ATOM(a_ll)->flags, ATOM_FLAG_SELECTED);
                SET_FLAG(ATOM(a_ll)->flags, ATOM_FLAG_DRAW);
                SET_FLAG(PARAGRAPH(p_ll)->flags, PARAGRAPH_FLAG_DRAW);
            }
            if (prev_ll)
                join_atom_next(widget, prev_ll);
        }
    }
    widget_render(widget);
}



/**
 * Widget methods
 */
static void widget_render ( text_widget * widget ) {
    block * b;
    paragraph * p;
    atom * a;
    LList * ll_p, * ll_b, * ll_a;
    u8  draw_all, draw_atoms, draw_line_atoms;
    u16 offset = 0;
    s16 x, y, w, h;

    y = (widget->fvb_v_y - widget->v_y_top) + widget->border_v;
    ll_b = widget->fvb;
    draw_all = FALSE;
    draw_line_atoms = FALSE;

    while (y < widget->height - 2*widget->border_v) {
        if (!ll_b)
            break;
        b = BLOCK(ll_b);
        offset = 0;

        for (ll_p = b->paragraphs; 
             ll_p && (y < widget->height - 2*widget->border_v); 
             ll_p = llist_next(ll_p)) {
            p = PARAGRAPH(ll_p);
            if (!(draw_all || GET_FLAG(p->flags, PARAGRAPH_FLAG_DRAW))) {
                /* We don't need to draw this paragraph */
                offset += p->len;
                y += p->height;
            } else {
                if (GET_FLAG(p->flags, PARAGRAPH_FLAG_DRAW_ALL) == 
                    ((u8) PARAGRAPH_FLAG_DRAW_ALL))
                    draw_all = TRUE;
                draw_atoms = draw_all;

                for (ll_a = p->atoms; 
                     ll_a && (y < widget->height - 2*widget->border_v); 
                     ll_a = llist_next(ll_a)) {
                    a = ATOM(ll_a);
                    if (GET_FLAG(a->flags, ATOM_FLAG_LEFT)) 
                        x = widget->border_h;
                    
                    if ((draw_atoms || draw_line_atoms || GET_FLAG(a->flags, ATOM_FLAG_DRAW | ATOM_FLAG_DRAW_LAST_CHAR)) && (y >= 0)) {
                        if (GET_FLAG(a->flags, ATOM_FLAG_DRAW_LAST_CHAR) && !(draw_atoms || draw_line_atoms)) {
                            if (a->len > 0) {
                                textedit_char_size(widget->self, block_char_at(b, offset + a->len - 1), &w, &h);
                                textedit_draw_str(widget->self, 
                                                  (TEXTEDIT_UCHAR *) block_str_at(b, offset + a->len - 1),
                                                  1, 
                                                  x + a->width - w, y,
                                                  GET_FLAG(a->flags, ATOM_FLAG_SELECTED));
                            }
                        } else {
                            textedit_draw_str(widget->self, 
                                              (TEXTEDIT_UCHAR *) block_str_at(b, offset),
                                              a->len, 
                                              x, y,
                                              GET_FLAG(a->flags, ATOM_FLAG_SELECTED));
                        }

                        if (GET_FLAG(a->flags, ATOM_FLAG_RIGHT)) {
                            textedit_clear_rect(widget->self, 
                                                x + a->width, 
                                                y, 
                                                widget->width - widget->border_h - (x + a->width),
                                                a->height);
                            draw_line_atoms = FALSE;
                        } else {
                            draw_line_atoms = TRUE;
                        }
                    } 
                    if ((ll_p == b->cursor_paragraph) &&
                        (ll_a == b->cursor_atom)) {
                        textedit_move_cursor (widget->self, 
                                              x + a->width, y, 
                                              a->height ? a->height :
                                              widget->cursor_grop->r.h);
                    }
                    offset += a->len;
                    if (GET_FLAG(a->flags, ATOM_FLAG_RIGHT)) {
                        y += a->height;
                    }
                    x += a->width;
                    UNSET_FLAG(a->flags, ATOM_FLAG_DRAW);
                    UNSET_FLAG(a->flags, ATOM_FLAG_DRAW_LAST_CHAR);
                }
                if (GET_FLAG(p->flags, PARAGRAPH_FLAG_DRAW_AFTER)) {
                    /* We need to shift text down. Draw everything after
                       this paragraph. */
                    draw_all = TRUE;
                }
            }
            /* Clear all paragraph flags */
            p->flags = 0;
        }
        /* Go to next block */
        ll_b = llist_next(ll_b);
    }
    
    /* Clear any undrawn region if paragraphs have changed height */
    if (draw_all && (y < widget->height - 2*widget->border_v)) {
        textedit_clear_rect(widget->self,
                            widget->border_h, y, 
                            widget->width - 2*widget->border_h, 
                            widget->height - 2*widget->border_v - y);
    }
 
    textedit_scrollevent (widget->self);
}


/**
 * Calculate and set the cursor virtual y coordiate
 */
static u32 widget_cursor_v_y ( text_widget * widget ) {
    block * b;
    paragraph * p;
    LList * ll_p, * ll_b, * ll_a;
    atom * a;
    u32 cursor_v_y = 0;

    for (ll_b = widget->blocks; ll_b; ll_b = llist_next(ll_b)) {
        b = BLOCK(ll_b);
        for (ll_p = b->paragraphs; ll_p; ll_p = llist_next(ll_p)) {
            p = PARAGRAPH(ll_p);
            if (ll_p == b->cursor_paragraph) {
                for (ll_a = p->atoms; ll_a; ll_a = llist_next(ll_a)) {
                    a = ATOM(ll_a);
                    if (ll_a == b->cursor_atom) {
                        widget->cursor_v_y = cursor_v_y;
                        return cursor_v_y;
                    }
                    if (GET_FLAG(a->flags, ATOM_FLAG_RIGHT))
                        cursor_v_y += a->height;
                }
            } else {
                cursor_v_y += p->height;
            }
        }
    }
    assert(0); // We shouldn't get here
    return cursor_v_y;
}


/**
 * Make sure the cursor is within the visible region. It should be
 * called after wrap (so paragraph sizes are properly calculated) but
 * before render.
 */
static void widget_scroll_to_cursor ( text_widget * widget ) {
    u16 widget_height;
    u16 line_extend;
    
    widget_height = widget->height - widget->border_h * 2;
    line_extend = widget_height % widget->cursor_grop->r.h;
    widget_height -= line_extend;
    
    if (widget->v_height < widget_height) {
        widget_set_v_top (widget, 0);
    } else if (widget->cursor_v_y < widget->v_y_top ) {
        widget_set_v_top (widget, widget->cursor_v_y);
    } else if (widget->cursor_v_y + widget->cursor_grop->r.h + line_extend >= 
               widget->v_y_top + widget_height) {
        widget_set_v_top (widget, 
                          (widget->cursor_v_y + 
                           line_extend +
                           widget->cursor_grop->r.h) -
                          widget_height);
    }
}


/**
 * Scroll the widget pane to a given virtual y coordinate. Note that
 * we will tweak the v_top value so it does not cut a word off mid-line.
 * This does not re-render; it is typically called before widget_render.
 */
static void widget_set_v_top ( text_widget * widget,
                               u32 v_top ) {
    LList * ll_b, * ll_p, * ll_a;
    u32 h;

    /* Determine first visible block */
    for (widget->fvb_v_y = 0, widget->fvb = widget->blocks; ;
         widget->fvb = llist_next(widget->fvb)) {
        for (h = 0, ll_p = BLOCK(widget->fvb)->paragraphs; 
             ll_p && (v_top > widget->fvb_v_y + h + PARAGRAPH(ll_p)->height); 
             ll_p = llist_next(ll_p)) {
            h += PARAGRAPH(ll_p)->height;
        }
        if (ll_p)
            break;
        widget->fvb_v_y += h;
    }

    /* Line-align top */
    for (ll_a = PARAGRAPH(ll_p)->atoms; 
         ll_a && (widget->fvb_v_y + h < v_top); 
         ll_a = llist_next(ll_a)) {
        if (GET_FLAG(ATOM(ll_a)->flags, ATOM_FLAG_RIGHT)) {
            h += ATOM(ll_a)->height;
            if (widget->fvb_v_y + h > v_top) {
                v_top = widget->fvb_v_y + h - ATOM(ll_a)->height;
            }
        }
    }

    widget->v_y_top = v_top;
    SET_FLAG(PARAGRAPH(BLOCK(widget->fvb)->paragraphs)->flags, 
             PARAGRAPH_FLAG_DRAW_ALL);
}


/**
 * Keep the cursor 'on' for an additional blink cycle. This is done so
 * the cursor doesn't blink as we're in the midst of typing or moving
 * the cursor around.
 */
static void  widget_cursor_stay_on ( text_widget * widget ) {
    if (widget->cursor_state < 2)
        widget->cursor_state = 2;
}


/**
 * Move the cursor up/down/page up/page down/end/home. On vertical movement,
 * attempt to honor the cursor column.
 * 
 * The clear_selection flag determines whether we should unset any selections.
 * This is usually TRUE.
 *
 * The unite flag determines whether we should unite cursor-split atoms
 * before moving the cursor. This is usually TRUE unless we're doing
 * fancy things with selections.
 */
static g_error widget_move_cursor_dir ( text_widget * widget,
                                        cursor_direction dir,
                                        u8 unset_selection,
                                        u8 unite) {
    g_error e;
    LList * ll_b, * ll_p, * ll_a;
    block * b;
    atom * a;
    s16 w, h;
    u16 width, cursor, cursor_end;
    u16 num_lines, line;

    if (unset_selection && (widget->selection != SELECTION_NONE)) 
        widget_unset_selection(widget);
    
    ll_b = widget->current;
    b = BLOCK(ll_b);  
    ll_p = b->cursor_paragraph;
    ll_a = b->cursor_atom;
    cursor = b->b_gap;

    switch (dir) {
    case CURSOR_LEFT:
        widget->cursor_x_stash = -1;
        if (cursor) {
            cursor--;
        } else {
            while (!cursor) {
                ll_b = llist_prev(ll_b);
                if (!ll_b)
                    return; // Start of file
                b = BLOCK(ll_b);
                cursor = b->len;
                if (cursor && (block_char_at(b, cursor-1) == '\n'))
                    cursor--;
            }
        }
        break;
    case CURSOR_RIGHT:
        widget->cursor_x_stash = -1;
        cursor++;
        while ((cursor > b->len) || 
               (cursor && 
                (cursor == b->len) && 
                llist_next(ll_b) &&
                (block_char_at(b, cursor-1) == '\n'))) {
            cursor = 0;
            ll_b = llist_next(ll_b);
            if (!ll_b)
                return; // End of file
            b = BLOCK(ll_b);
        }
        break;
    case CURSOR_UP:
    case CURSOR_PAGE_UP:
        if (dir == CURSOR_UP)
            num_lines = 1;
        else
            num_lines = (widget->height - 2*widget->border_v) /
                ATOM(ll_a)->height - 1;
        for (line = 0; ll_a; ) {
            if (GET_FLAG(ATOM(ll_a)->flags, ATOM_FLAG_LEFT)) {
                if (++line > num_lines)
                    break;
            }
            cursor -= ATOM(ll_a)->len;
            if (llist_prev(ll_a)) {
                ll_a = llist_prev(ll_a);
            } else if (llist_prev(ll_p)) {
                ll_p = llist_prev(ll_p);
                ll_a = llist_last(PARAGRAPH(ll_p)->atoms);
            } else if (llist_prev(ll_b)) {
                ll_b = llist_prev(ll_b);
                b = BLOCK(ll_b);
                cursor = b->len;
                ll_p = llist_last(b->paragraphs);
                ll_a = llist_last(PARAGRAPH(ll_p)->atoms);
            } else {
                /* Start of file */
                cursor = ATOM(ll_a)->len; 
                break; 
            }
        }
        break;
    case CURSOR_DOWN:
    case CURSOR_PAGE_DOWN:
        if (dir == CURSOR_DOWN)
            num_lines = 1;
        else
            num_lines = (widget->height - 2*widget->border_v) /
                ATOM(ll_a)->height - 1;
        for (line = 0; ll_a; ) {
            if (GET_FLAG(ATOM(ll_a)->flags, ATOM_FLAG_RIGHT)) {
                if (++line > num_lines)
                    break;
            }
            if (llist_next(ll_a)) {
                ll_a = llist_next(ll_a);
            } else if (llist_next(ll_p)) {
                ll_p = llist_next(ll_p);
                ll_a = PARAGRAPH(ll_p)->atoms;
            } else if (llist_next(ll_b)) {
                ll_b = llist_next(ll_b);
                b = BLOCK(ll_b);
                ll_p = b->paragraphs;
                ll_a = PARAGRAPH(ll_p)->atoms;
                cursor = 0;
            } else {
                break; /* End of file */
            }
            cursor += ATOM(ll_a)->len;
        }
        while (!GET_FLAG(ATOM(ll_a)->flags, ATOM_FLAG_LEFT)) {
            cursor -= ATOM(ll_a)->len;
            ll_a = llist_prev(ll_a);
        }
        break;
    case CURSOR_HOME:
        while (llist_prev(ll_b))
            ll_b = llist_prev(ll_b);
        b = BLOCK(ll_b);
        cursor = 0;
        break;
    case CURSOR_END:
        while (llist_next(ll_b))
            ll_b = llist_next(ll_b);
        b = BLOCK(ll_b);
        cursor = b->len;
        break;
    }
    
    /* On up/down, we've set ll_a to the start of the line. Move
       cursor forward to match target X coordinate */
    if ((dir == CURSOR_UP) ||
        (dir == CURSOR_DOWN) ||
        (dir == CURSOR_PAGE_UP) ||
        (dir == CURSOR_PAGE_DOWN)) {
        if (widget->cursor_x_stash < 0) 
            widget->cursor_x_stash = widget->cursor_grop->r.x - 
                widget->border_h;

        /* a_ll is the atom at the start of the target line */
        cursor -= ATOM(ll_a)->len;
        for (cursor_end = cursor; ll_a; ll_a = llist_next(ll_a)) {
            cursor_end += ATOM(ll_a)->len;
            if (GET_FLAG(ATOM(ll_a)->flags, ATOM_FLAG_RIGHT))
                break;
        }
        for (width = 0; 
             ((width < widget->cursor_x_stash) && 
              (cursor < cursor_end) && 
              (block_char_at(b, cursor) != '\n')); 
             cursor++) {
            block_char_size ( widget, b, cursor, &w, &h);
            width += w;
        }
    }

    e = widget_move_cursor ( widget, 
                             ll_b,
                             cursor,
                             unite); 
    errorcheck;
    widget_cursor_stay_on ( widget );
    return success;
}


/**
 * Move the cursor to an (x, y) coordinate (0,0 is topleft corner of widget).
 */
static g_error widget_move_cursor_xy  ( text_widget * widget,
                                        u16 x, 
                                        u16 y,
                                        u8 unset_selection,
                                        u8 unite) {
    g_error e;
    LList * ll_b, * ll_p, * ll_a;
    block * b;
    atom * a;
    u16 cursor, width, cursor_end;
    s16 w, h;
    s32 v_y;
    
    if (unset_selection && (widget->selection != SELECTION_NONE))
        widget_unset_selection(widget);

    ll_b = widget->fvb;
    b = BLOCK(ll_b);
    ll_p = b->paragraphs;
    ll_a = PARAGRAPH(ll_p)->atoms;
    cursor = 0;

    y -= widget->border_v;
    x -= widget->border_h;
    
    for (v_y = widget->fvb_v_y;  v_y <= widget->v_y_top + y; ) {
        if (!ll_a) {
            ll_p = llist_next(ll_p);
            if (!ll_p) {
                ll_b = llist_next (ll_b);
                if (!ll_b)
                    break;
                b = BLOCK(ll_b);
                cursor = 0;
                ll_p = b->paragraphs;
            }
            ll_a = PARAGRAPH(ll_p)->atoms;
        }
        a = ATOM(ll_a);
        cursor += a->len;
        if (GET_FLAG(a->flags, ATOM_FLAG_RIGHT)) {
            v_y += a->height;
            if (v_y >= widget->v_y_top + y)
                break;
        }
        ll_a = llist_next(ll_a);
    }
    if (!ll_b) {
        ll_b = llist_last(widget->blocks);
        ll_p = llist_last(BLOCK(ll_b)->paragraphs);
        ll_a = NULL;
        cursor = BLOCK(ll_b)->len;
    } 
    if (!ll_a) {
        ll_a = llist_last(PARAGRAPH(ll_p)->atoms);
    }
    b = BLOCK(ll_b);

    /* Set cursor to start of line */
    for ( ; !GET_FLAG(ATOM(ll_a)->flags, ATOM_FLAG_LEFT); 
          ll_a = llist_prev(ll_a)) 
        cursor -= ATOM(ll_a)->len;
    cursor -= ATOM(ll_a)->len;

    /* Determine how far over the cursor may be */
    for (cursor_end = cursor; !GET_FLAG(ATOM(ll_a)->flags, ATOM_FLAG_RIGHT); 
        ll_a = llist_next(ll_a)) 
        cursor_end += ATOM(ll_a)->len;
    cursor_end += ATOM(ll_a)->len;
    
    for (width = 0; (width < x) && (cursor < cursor_end); cursor++) {
        block_char_size( widget, b, cursor, &w, &h);
        width += w;
    }
    
    /* Special case: cursor at end of line which ends in break or newline */
    if ((cursor == cursor_end) && 
        (ATOM(ll_a)->len > 1) && 
        ((break_char(block_char_at(b, cursor - 1))) ||
         (block_char_at(b, cursor - 1) == '\n')))
        cursor--;
        
    e = widget_move_cursor ( widget, 
                             ll_b,
                             cursor, 
                             unite );
    errorcheck;
    return success;
}


/**
 * Move the cursor to the offset in the given block.  
 */
static g_error widget_move_cursor ( text_widget * widget,
                                    LList * ll_new_block,
                                    size_t offset,
                                    u8 unite) {
    g_error e;
    block * b, * old_block;
    atom * a, * n_a;
    paragraph * p;
    size_t count;
    LList * ll_b, * ll_p, * ll_a;
    u16 x, p_len;
    u32 v_top;

    
    b = BLOCK(widget->current);
    /* Unite lines split by old cursor */
    if (unite)
        join_atom_next(widget, b->cursor_atom);

    /* Clear old cursor position */
    b->cursor_paragraph = NULL;
    b->cursor_atom = NULL;

    b = BLOCK(ll_new_block);
    block_set_bgap(b, offset);
    
    /* Calculate virtual height up to block b */
    for (widget->cursor_v_y = 0, ll_b = widget->blocks; 
         ll_b != ll_new_block; 
         ll_b = llist_next(ll_b)) {
        for (ll_p = BLOCK(ll_b)->paragraphs; ll_p; ll_p = llist_next(ll_p)) {
            widget->cursor_v_y += PARAGRAPH(ll_p)->height;
        }
    }

    /* Find paragraph containing cursor */
    for (count = 0, ll_p = b->paragraphs; 
         ll_p && (count + PARAGRAPH(ll_p)->len <= offset); 
         ll_p = llist_next(ll_p)) {
        widget->cursor_v_y += PARAGRAPH(ll_p)->height;
        count += PARAGRAPH(ll_p)->len;
    }
    if (!ll_p) {
        ll_p = llist_last(b->paragraphs);
        count -= PARAGRAPH(ll_p)->len;
        widget->cursor_v_y -= PARAGRAPH(ll_p)->height;
    }
    p = PARAGRAPH(ll_p);
    b->cursor_paragraph = ll_p;

    /* Set the block cursor, splitting atoms as necessary. */
    for (ll_a = p->atoms; 
         ll_a && (count <= offset); 
         ll_a = llist_next(ll_a)) {
        a = ATOM(ll_a);
        
        if (GET_FLAG(a->flags, ATOM_FLAG_LEFT))
            x = 0;
        
        if (offset < count + a->len) { 
            e = split_atom(widget, b, ll_a, count, offset - count);
            errorcheck;
        }
        count += a->len;
        p_len += a->len;
        x += a->width; 
        
        if ((offset > count) && (GET_FLAG(a->flags, ATOM_FLAG_RIGHT)))
            widget->cursor_v_y += a->height;
        
        if (offset == count)
            break;
    }
    b->cursor_atom = ll_a;  
    assert(b->cursor_atom);

    // FIXME
    /* If the cursor operates on the end of a line and that line
       ends with break char or newline, display cursor at start of
       next line */
    if (offset &&
        GET_FLAG(ATOM(ll_a)->flags, ATOM_FLAG_RIGHT) && 
        llist_next(ll_a)) {
        x = 0;
        widget->cursor_v_y += ATOM(llist_next(ll_a))->height;
    }
    
    widget->current = ll_new_block;

    v_top = widget->v_y_top; 
    widget_scroll_to_cursor(widget);
    if (v_top != widget->v_y_top) {
        /* If we scrolled, re-render. This draws the cursor */
        widget_render(widget);
    } else {
        /* Move cursor on screen */
        textedit_move_cursor (widget->self, 
                              widget->border_h + x, 
                              widget->border_v + widget->cursor_v_y - 
                              widget->v_y_top,
                              widget->cursor_grop->r.h);
    }
    return success;
}


static void widget_unset_selection ( text_widget * widget ) {
    LList * ll_b, * ll_p, * ll_a;
    block * b;
    
    ll_b = widget->current;
    b = BLOCK(ll_b);  
    ll_p = b->cursor_paragraph;
    ll_a = b->cursor_atom;
    switch (widget->selection) {
    case SELECTION_AFTER_CURSOR:
        for (ll_a = next_atom(ll_b, ll_p, ll_a, &ll_b, &ll_p); 
             ll_a && GET_FLAG(ATOM(ll_a)->flags, ATOM_FLAG_SELECTED);
             ll_a = next_atom(ll_b, ll_p, ll_a, &ll_b, &ll_p)) {
            UNSET_FLAG(ATOM(ll_a)->flags, ATOM_FLAG_SELECTED);
            SET_FLAG(ATOM(ll_a)->flags, ATOM_FLAG_DRAW);
            SET_FLAG(PARAGRAPH(ll_p)->flags, PARAGRAPH_FLAG_DRAW);
        }
        if (ll_a && llist_prev(ll_a))
            join_atom_next(widget, llist_prev(ll_a));
        widget_render(widget);
        break;
    case SELECTION_BEFORE_CURSOR:
        for (; ll_a && GET_FLAG(ATOM(ll_a)->flags, ATOM_FLAG_SELECTED);
             ll_a = prev_atom(ll_b, ll_p, ll_a, &ll_b, &ll_p)) {
            UNSET_FLAG(ATOM(ll_a)->flags, ATOM_FLAG_SELECTED);
            SET_FLAG(ATOM(ll_a)->flags, ATOM_FLAG_DRAW);
            SET_FLAG(PARAGRAPH(ll_p)->flags, PARAGRAPH_FLAG_DRAW);
        }
        if (ll_a)
            join_atom_next(widget, ll_a);
        widget_render(widget);
        break;
    }
    widget->selection = SELECTION_NONE;
}


static g_error widget_clear_selection  ( text_widget * widget ) {
    g_error e;
    LList *ll_b, * ll_p, * ll_a, *ll;
    u32 offset, len;

    /* If there is a selected region, delete it */
    ll_b = widget->current;
    ll_p = BLOCK(ll_b)->cursor_paragraph;
    ll_a = BLOCK(ll_b)->cursor_atom;

    if (widget->selection != SELECTION_NONE) {
        len = 0;
        if (widget->selection == SELECTION_AFTER_CURSOR) {
            for (ll_a = next_atom(ll_b, ll_p, ll_a, &ll_b, &ll_p); 
                 ll_a && GET_FLAG(ATOM(ll_a)->flags, ATOM_FLAG_SELECTED);
                 ll_a = next_atom(ll_b, ll_p, ll_a, &ll_b, &ll_p)) {
                len += ATOM(ll_a)->len;
            }
            if (!ll_a) {
                ll_b = llist_last(widget->blocks);
                ll_p = llist_last(BLOCK(ll_b)->paragraphs);
                ll_a = llist_last(PARAGRAPH(ll_p)->atoms);
            } else {
                ll_a = prev_atom(ll_b, ll_p, ll_a, &ll_b, &ll_p);
            }
        } else if (widget->selection == SELECTION_BEFORE_CURSOR) {
            for (; ll_a && GET_FLAG(ATOM(ll_a)->flags, ATOM_FLAG_SELECTED);
                 ll_a = prev_atom(ll_b, ll_p, ll_a, &ll_b, &ll_p)) {
                len += ATOM(ll_a)->len;
            }
            ll_b = widget->current;
            ll_p = BLOCK(ll_b)->cursor_paragraph;
            ll_a = BLOCK(ll_b)->cursor_atom;
        }
        for (offset = 0, ll = BLOCK(ll_b)->paragraphs; 
             ll != ll_p;
             ll = llist_next(ll))
            offset += PARAGRAPH(ll)->len;
        for (ll = ll_a;
             ll;
             ll = llist_prev(ll))
            offset += ATOM(ll)->len;
        
        e = widget_delete_chars(widget, ll_b, ll_p, ll_a, offset, len);
        errorcheck;
        widget->selection = SELECTION_NONE;
    }
    return success;
}


static g_error widget_insert_chars ( text_widget * widget,
                                     TEXTEDIT_UCHAR * str,
                                     u32 len ) {
    g_error e;
    block * b;
    paragraph * p;
    atom * a;
    LList * ll_p, * ll_a, *ll_a_temp;
    u32 chunk_len;
    u8 new_para;
    u8 widget_realized = TRUE;
    struct font_metrics m;
    widget->fd->lib->getmetrics(widget->fd,&m);

    /* If there is a selected region, delete it */
    e = widget_clear_selection(widget);
    errorcheck;

    /* Invalidate cursor's preferred x position */
    widget->cursor_x_stash = -1;

    if (! ((widget->fd) && (widget->bit)) ) 
        widget_realized = FALSE;
    else if ((widget->width < m.charcell.w) || 
        (widget->height < m.charcell.h))
        widget_realized = FALSE;

    while (len) {
        b = BLOCK(widget->current);
        p = PARAGRAPH(b->cursor_paragraph);
        a = ATOM(b->cursor_atom);
        SET_FLAG(p->flags, PARAGRAPH_FLAG_DRAW);

        for (chunk_len = 1, new_para = FALSE;
             (chunk_len < len) && (str[chunk_len-1] != '\n'); 
             chunk_len++)
            break;
        if (str[chunk_len-1] == '\n')
            new_para = TRUE;

        /* Make sure there is enough room in block to insert chunk */
        if (b->len + chunk_len >= b->data_size) {
            e = block_shed_last_para (widget, widget->current);
            errorcheck;
            if (b->len == b->data_size) {
                /* If shedding didn't reduce block size, grow block */
                e = block_data_resize ( widget, b, 
                                        b->len + chunk_len + BUFFER_GROW);
                errorcheck;
            } 
            b = BLOCK(widget->current);
            p = PARAGRAPH(b->cursor_paragraph);
            a = ATOM(b->cursor_atom); 
        }
        TEXTEDIT_MEMCPY(b->data + b->b_gap, (TEXTEDIT_CHAR *) str, chunk_len);

        b->b_gap += chunk_len;
        a->len += chunk_len;
        p->len += chunk_len;
        b->len += chunk_len;
        SET_FLAG(a->flags, ATOM_FLAG_DRAW_LAST_CHAR);

        if (new_para) {
            /* Create a new paragraph */
            SET_FLAG ( a->flags, ATOM_FLAG_DRAW | ATOM_FLAG_RIGHT );
            ll_a = llist_next(b->cursor_atom);
            e = paragraph_create(&p);
            errorcheck;

            if (ll_a) {
                /* Split ll_a list at ll_a, insert after 0-len atom which
                 * starts paragraph p */
                e = llist_split( ll_a,
                                 &PARAGRAPH(b->cursor_paragraph)->atoms,
                                 &ll_a_temp);
                errorcheck;
                llist_concat(&p->atoms, p->atoms, ll_a_temp);
                /* Get paragraph len */
                for (ll_a = p->atoms; ll_a; ll_a = llist_next(ll_a))
                    p->len += ATOM(ll_a)->len;
                PARAGRAPH(b->cursor_paragraph)->len -= p->len;
                UNSET_FLAG(ATOM(p->atoms)->flags, ATOM_FLAG_RIGHT);
            }

            e = llist_insert_after (&b->paragraphs, b->cursor_paragraph, p);
            errorcheck;
            
            b->cursor_paragraph = llist_next(b->cursor_paragraph);
            b->cursor_atom = p->atoms;

            SET_FLAG( ATOM(b->cursor_atom)->flags, ATOM_FLAG_DRAW );
            SET_FLAG( PARAGRAPH(b->cursor_paragraph)->flags,
                      PARAGRAPH_FLAG_DRAW_ALL |
                      PARAGRAPH_FLAG_H_INVALID );         
            SET_FLAG (PARAGRAPH(llist_prev(b->cursor_paragraph))->flags,
                      PARAGRAPH_FLAG_H_INVALID);

            /* Wrap end of previous paragraph -- inserting a newline could
               cause the end of a line to move up */
            e = wrap(widget, b, 
                     PARAGRAPH(llist_prev(b->cursor_paragraph)), 
                     llist_last(PARAGRAPH(llist_prev(b->cursor_paragraph))->atoms));
            errorcheck;
        } else {
            if (widget_realized) {
                SET_FLAG(a->flags, ATOM_FLAG_DRAW_LAST_CHAR);
            } else {
                SET_FLAG( PARAGRAPH(b->cursor_paragraph)->flags,
                          PARAGRAPH_FLAG_DRAW_ALL);
            }
        }
        e = wrap(widget, b, p, b->cursor_atom);
        errorcheck;

        len -= chunk_len;
        str += chunk_len;
    }

    if (widget_realized) {
        if (new_para)
            widget_cursor_v_y(widget);
        widget_scroll_to_cursor (widget);
        widget_render (widget);
        widget_cursor_stay_on (widget);
    }
    return success;
}


/* Delete len chars before offset in block ll_b. The atom ll_a is the
 * atom where the delete starts, on the right edge. */
static g_error widget_delete_chars ( text_widget * widget,
                                     LList * ll_b,
                                     LList * ll_p,
                                     LList * ll_a,
                                     u32 offset,
                                     u32 len ) {
    g_error e;
    LList * ll_start_b, * ll_start_p, * ll_start_a, * ll_temp;
    LList * cursor_b, * cursor_p, * cursor_a;
    block * b;
    paragraph * p;
    atom * a;
    u8 cursor_del = FALSE; /* Did we delete the cursor? */
    u8 h_invalid = FALSE; /* Did the paragraph height change? */
    u32 count;
    u16 len_start_atom; /* The new length for the start atom */

    if (len == 0)
        return;

    /* Record cursor */
    cursor_b = widget->current;
    cursor_p = BLOCK(cursor_b)->cursor_paragraph;
    cursor_a = BLOCK(cursor_b)->cursor_atom;

    /* Find the start atom/paragraph/block of the delete */
    ll_start_a = ll_a;
    ll_start_p = ll_p;
    ll_start_b = ll_b;

    for (count = ATOM(ll_start_a)->len; ll_start_a && (count < len); ) {
        ll_start_a = prev_atom(ll_start_b, ll_start_p, ll_start_a,
                               &ll_start_b, &ll_start_p);
        if (ll_start_a)
            count += ATOM(ll_start_a)->len;
    }
    if (!ll_start_a) {
        ll_start_b = widget->blocks;
        ll_start_p = BLOCK(ll_start_b)->paragraphs;
        ll_start_a = PARAGRAPH(ll_start_p)->atoms;
        len = count;
    }
    len_start_atom = count - len;

    if (ll_start_b != ll_b) {
        /* Delete across blocks. */
        /* Remove blocks between ll_start_b and ll_b */
        while (llist_next(ll_start_b) != ll_b) {
            if (llist_next(ll_start_b) == cursor_b)
                cursor_del = TRUE;
            b = BLOCK(llist_next(ll_start_b));
            len -= b->len;
            for (ll_temp = b->paragraphs; ll_temp; 
                 ll_temp = llist_next(ll_temp)) {
                widget->v_height -= PARAGRAPH(ll_temp)->height;
            }
            block_destroy(b);
            llist_remove(&widget->blocks, llist_next(ll_start_b));
        } 
        /* Remove paragraphs after ll_start_p */
        for (b = BLOCK(ll_start_b); llist_next(ll_start_p); ) {
            if (llist_next(ll_start_p) == cursor_p)
                cursor_del = TRUE;
            p = PARAGRAPH(llist_next(ll_start_p));
            b->len -= p->len;
            len -= p->len;
            widget->v_height -= p->height;
            paragraph_destroy(p);
            llist_remove(&b->paragraphs, llist_next(ll_start_p)); 
        }
        /* Remove paragraphs before ll_p */
        for (b = BLOCK(ll_b); llist_prev(ll_p); ) {
            if (llist_prev(ll_p)== cursor_p)
                cursor_del = TRUE;
            p = PARAGRAPH(llist_prev(ll_p));
            b->len -= p->len;
            offset -= p->len;
            len -= p->len;
            widget->v_height -= p->height;
            paragraph_destroy(p);
            llist_remove(&b->paragraphs, llist_prev(ll_p)); 
        }
        b = BLOCK(ll_start_b);
        p = PARAGRAPH(ll_p);
        /* Grow ll_start_b so it can hold the first paragraph of
           ll_b. Note that we rely on the idle routine to break large,
           multi-paragraph blocks apart. */
        if (b->data_size - b->len <= p->len + 1) {
            e = block_data_resize (widget, b, 
                                   b->len + p->len + BUFFER_GROW);
            errorcheck;
        }
        block_set_bgap(b, b->len); 
        block_set_bgap(BLOCK(ll_b), 0);

        /* Move para text between blocks */
        TEXTEDIT_MEMCPY(b->data + b->len,
			BLOCK(ll_b)->data + BLOCK(ll_b)->data_size - BLOCK(ll_b)->len,
			p->len);
        offset += b->len;
        b->len += p->len;
        b->b_gap += p->len;
        e = llist_append (&b->paragraphs, b->paragraphs, p);
        errorcheck;

        BLOCK(ll_b)->len -= p->len;
        BLOCK(ll_b)->b_gap += p->len;

        llist_remove(&BLOCK(ll_b)->paragraphs, ll_p);

         /* Remove block if empty */
        if (BLOCK(ll_b)->len == 0) {
            block_destroy(BLOCK(ll_b));
            llist_remove(&widget->blocks, ll_b);
        }
        ll_b = ll_start_b;
        ll_p = llist_last(b->paragraphs);
    }

    /* Delete within current block */
    b = BLOCK(ll_start_b);
    block_set_bgap(b, offset - len);
    b->len -= len;

    /* If  ll_start_a == ll_a, we're done  */
    if (ll_start_a != ll_a) {
        if (ll_start_p != ll_p) {
            h_invalid = TRUE;
            /* Destroy paragraphs between ll_start_p and ll_p */
            while (llist_next(ll_start_p) != ll_p) {
                if (llist_next(ll_start_p) == cursor_p)
                    cursor_del = TRUE;
                p = PARAGRAPH(llist_next(ll_start_p));
                len -= p->len;
                widget->v_height -= p->height;
                paragraph_destroy(p);
                llist_remove(&b->paragraphs, llist_next(ll_start_p)); 
            }
            /* Append ll_p's atoms to ll_start_p */
            p = PARAGRAPH(ll_p);
            llist_concat(&PARAGRAPH(ll_start_p)->atoms,
                         PARAGRAPH(ll_start_p)->atoms, 
                         p->atoms);
            PARAGRAPH(ll_start_p)->len += p->len;
            p->atoms = NULL;
            widget->v_height -= p->height;
            paragraph_destroy(p);
            llist_remove(&b->paragraphs, ll_p);                
        }
        p = PARAGRAPH(ll_start_p);
        /* Remove atoms ll_a */
        while (ll_a != ll_start_a) {
            if (ll_a == cursor_a)
                cursor_del = TRUE;
            ll_temp = ll_a;
            ll_a = llist_prev(ll_a);
            p->len -= ATOM(ll_temp)->len;
            atom_destroy(ATOM(ll_temp));
            llist_remove(&p->atoms, ll_temp);
        }
    }
    p = PARAGRAPH(ll_start_p);
    p->len -= ATOM(ll_start_a)->len - len_start_atom;
    ATOM(ll_start_a)->len =  len_start_atom;

    if ((ATOM(ll_start_a)->len == 0)  && llist_prev(ll_start_a)) {
        if (ll_a == cursor_a)
            cursor_del = TRUE;
        ll_a = ll_start_a;
        ll_start_a = llist_prev(ll_a);
        atom_destroy(ATOM(ll_a));
        llist_remove(&p->atoms, ll_a);
    }

    SET_FLAG(ATOM(ll_start_a)->flags, ATOM_FLAG_DRAW_LAST_CHAR);
    SET_FLAG(PARAGRAPH(ll_start_p)->flags, PARAGRAPH_FLAG_DRAW);
    UNSET_FLAG(ATOM(ll_start_a)->flags, ATOM_FLAG_RIGHT);
    SET_FLAG(ATOM(llist_last(ll_start_a))->flags, ATOM_FLAG_RIGHT);
    if (cursor_del) {
        widget->current = ll_start_b;
        BLOCK(ll_start_b)->cursor_paragraph = ll_start_p;
        BLOCK(ll_start_b)->cursor_atom = ll_start_a;
    }
    if (h_invalid) {
        SET_FLAG(PARAGRAPH(ll_start_p)->flags, 
                 PARAGRAPH_FLAG_DRAW_ALL | PARAGRAPH_FLAG_H_INVALID);
    }

    e = wrap(widget, 
             BLOCK(ll_start_b), 
             PARAGRAPH(ll_start_p),
             ll_start_a);
    errorcheck;
    return success;
}


/**
 * Block methods 
 */
static g_error block_create ( block ** b ) {
    g_error e;

    e = g_malloc ((void **) b, sizeof(block));
    errorcheck;
   
    e = g_malloc ((void **) &((*b)->data),
		  sizeof(TEXTEDIT_CHAR) * FIXED_BUFFER_LEN);
    errorcheck;

    (*b)->data_size = FIXED_BUFFER_LEN;
    (*b)->b_gap = 0;
    (*b)->len = 0;
    (*b)->paragraphs = NULL;
    (*b)->cursor_paragraph = NULL;
    (*b)->cursor_atom = NULL;
    SET_FLAG((*b)->flags, BLOCK_FLAG_MFC);    
    return success;
}


static void block_destroy ( block * b ) {
    LList * l;

    assert(b);
    g_free(b->data);
    for (l = b->paragraphs; l; l = llist_next(l)) 
        paragraph_destroy(PARAGRAPH(l));
    llist_free(b->paragraphs);
    g_free(b);
}


/**
 * Set the buffer gap to start at offset 
 */
static void block_set_bgap ( block * b,
                             u16 offset ) {
    u16 shift;
    u16 gap_len;
    gap_len = b->data_size - b->len;
    assert (b);
    assert (offset <= b->data_size);
    
    if (!gap_len || !b->len) {
        b->b_gap = offset;
        return;
    }
    
    while (b->b_gap > offset) {
        shift = MIN(b->b_gap - offset, gap_len);        
        TEXTEDIT_STRNCPY(b->data + b->b_gap + gap_len - shift,
			 b->data + b->b_gap - shift, 
			 shift);
        b->b_gap -= shift;
    }

    while (b->b_gap < offset) {
        shift = MIN(offset - b->b_gap, gap_len);        
        TEXTEDIT_STRNCPY(b->data + b->b_gap, 
			 b->data + b->b_gap + gap_len, 
			 shift);
        b->b_gap += shift; 
    }
    
}


static TEXTEDIT_CHAR * block_str_at ( block * b,
				      u16 l_offset ) {
    if (l_offset >= b->b_gap) {
        return b->data + l_offset + (b->data_size - b->len);
    }
    return b->data + l_offset;
}


static void block_string_size ( text_widget * widget,
                                block * b,
                                u16 offset,
                                u16 len,
                                s16 * w,
                                s16 * h) {
    s16 t_w, t_h;
    size_t s_len;
    *w = 0;
    *h = 0;
    if (offset < b->b_gap) {
        s_len = MIN(len, b->b_gap - offset);
        textedit_str_size ( widget->self,
                            (TEXTEDIT_UCHAR *) block_str_at(b, offset),
                            s_len, w, h);
        len -= s_len;
        offset += s_len;
    } 
    if (offset >= b->b_gap) {
        textedit_str_size ( widget->self,
                            (TEXTEDIT_UCHAR *) block_str_at(b, offset),
                            len, &t_w, &t_h);
        *w += t_w;
        *h = MAX(*h, t_h);
    }
}


/**
 * Resize the block's data buffer, keeping the buffer gap at the same
 * offset. We cannot resize to a size smaller than the block's data
 * length.
 */
static g_error block_data_resize ( text_widget * widget,
                                   block * b,
                                   u16 new_size ) {
    TEXTEDIT_CHAR * data;
    u16 b_gap;
    g_error e;

    assert (b->len <= new_size);

    b_gap = b->b_gap;
    block_set_bgap (b, b->len);

    e = g_malloc ((void**) &data, sizeof(TEXTEDIT_CHAR) * new_size);
    errorcheck;

    TEXTEDIT_STRNCPY(data, b->data, b->len);
    g_free(b->data);
    b->data = data;
    b->data_size = new_size;
    block_set_bgap (b, b_gap);
    return success;
}


/**
 * Shed the last paragraph in a block to another block. Either append
 * the paragraph to the start of the next block or create a new block
 * and drop the paragraph off there. 
 *
 * For both the old and next blocks, set the buffer gap to the end of
 * the block unless the block contains the cursor.
 *
 * Note that we don't do anything if the block only has one paragraph. 
 */
static g_error block_shed_last_para ( text_widget * widget,
                                   LList * ll_b ) {
    g_error e;
    LList * ll_p, * ll_a, *ll_head;
    block * b, * b_next;
    paragraph * p;
    u16 b_gap, offset; 
    u8 move_cursor;    /* Flag - Are we moving a paragraph that
                          contains the cursor? */
    
    b = BLOCK(ll_b);
    ll_p = llist_last(b->paragraphs);
    p = PARAGRAPH(ll_p);

    b_gap = b->b_gap;
    if ((ll_b == widget->current) && (ll_p == b->cursor_paragraph)) {
        move_cursor = TRUE;
    } else {
        move_cursor = FALSE;
    }

    /* Special case: The very last paragraph in a document may be
       empty, and we can remove it. */
    if ((p->len == 0) && 
        (llist_prev(ll_p)) && 
        (llist_prev(ll_p) != b->paragraphs)) {
        ll_p = llist_prev(ll_p);
        paragraph_destroy(p);
        llist_remove(&ll_head, llist_next(ll_p));
        p = PARAGRAPH(ll_p);
    }

    if (ll_p == b->paragraphs) 
        return; /* Block only has one paragraph */

    block_set_bgap (b, b->len);

    if (!llist_next(ll_b) ||
        (BLOCK(llist_next(ll_b))->data_size - BLOCK(llist_next(ll_b))->len < 
         p->len + 1)) {
        e = block_create(&b_next);
        errorcheck;

        if (b_next->data_size < p->len + BUFFER_GROW) {
            e = block_data_resize (widget, b_next, p->len + BUFFER_GROW);
            errorcheck;
        }

        e = llist_insert_after (&ll_head, ll_b, b_next);
        errorcheck;
    } else {
        b_next = BLOCK(llist_next(ll_b));
        block_set_bgap(b_next, 0);
    }
    TEXTEDIT_STRNCPY(b_next->data, 
		     b->data + b->len - p->len,
		     p->len);
    
    b_next->b_gap = p->len; 
    b_next->len += p->len;    
    b->b_gap -= p->len;
    b->len -= p->len;

    llist_remove(&b->paragraphs, ll_p);
    e = llist_prepend(&b_next->paragraphs, b_next->paragraphs, p);
    errorcheck;

    if (move_cursor) {
        widget->current = llist_next(ll_b);
        b_next->cursor_paragraph = b_next->paragraphs;
        b->cursor_paragraph = NULL;
        b->cursor_atom = NULL;
        block_set_bgap(b_next, b_gap - b->len);
        for (ll_a = PARAGRAPH(b_next->cursor_paragraph)->atoms, offset = 0; 
             ll_a; ll_a = llist_next(ll_a)) {
            b_next->cursor_atom = ll_a;
            offset += ATOM(ll_a)->len;
            if (offset >= b_gap - b->len)
                break;
        }
    } else {
        block_set_bgap(b, b_gap);
    }
    return success;
}


/**
 * Paragraph methods 
 */
static g_error paragraph_create ( paragraph ** p ) {
    g_error e;
    atom * a;

    e = g_malloc((void **) p, sizeof(paragraph));
    errorcheck;

    e = atom_create(&a, ATOM_TEXT, ATOM_FLAG_LEFT | ATOM_FLAG_RIGHT);
    errorcheck;

    e = llist_append(&(*p)->atoms, NULL, a);
    errorcheck;
    
    (*p)->len = 0;
    (*p)->height = 0;
    (*p)->flags = PARAGRAPH_FLAG_H_INVALID;
    return success;
}


static void paragraph_destroy ( paragraph * p) {
    LList * l;
    for (l = p->atoms; l; l = llist_next(l)) 
        atom_destroy(ATOM(l));
    llist_free(p->atoms);
    g_free(p);
}


/**
 * Atom methods 
 */
static g_error atom_create ( atom ** a, 
                             atom_type type, 
                             u8 flags ) {
    g_error e;

    e = g_malloc ((void**) a, sizeof(atom));
    errorcheck;

    (*a)->flags = type | flags;
    (*a)->width = 0;
    (*a)->height = 0;
    (*a)->len = 0;
    return success;
}


static atom_type atom_get_type ( atom * a ) {
    assert ( a );
    return ((atom_type) a->flags & ATOM_TYPE_MASK);
}
 
 
static void atom_destroy ( atom * a ) {
    g_free (a);
}


/**
 * Unite the atom a_ll with the next atom, if there is one and it's
 * within the same line
 */
static void join_atom_next ( text_widget * widget,
                             LList * a_ll ) {
    atom * a, * next;
    LList * l_head;

    if (!llist_next(a_ll))
        return;
    a = ATOM(a_ll);
    if (!GET_FLAG(a->flags, ATOM_FLAG_RIGHT)) {
        next = ATOM(llist_next(a_ll));
        a->flags |= next->flags & (~ATOM_TYPE_MASK);
        a->len += next->len;
        a->width += next->width;
        a->height = MAX(a->height, next->height);
        /* We will remove the next atom. If it is the cursor atom, make
           a_ll the cursor atom */
        if (BLOCK(widget->current)->cursor_atom == llist_next(a_ll)) 
            BLOCK(widget->current)->cursor_atom = a_ll;
        atom_destroy(next);
        llist_remove(&l_head, llist_next(a_ll));
    }
}



/**
 * Split the atom such that the first chunk is len chars. Offset is
 * the offset of the start of a_ll in the block.
 */
static g_error split_atom ( text_widget * widget,
                            block * b,
                            LList * a_ll,
                            size_t offset,
                            size_t len ) {
    g_error e;
    atom * a, * n_atom;
    LList * l_head;

    a = ATOM(a_ll);
    assert(a->len >= len);

    e = atom_create(&n_atom, ATOM_TEXT, a->flags);
    errorcheck;

    n_atom->len = a->len - len;
    n_atom->height = a->height;
    a->len = len;
    block_string_size( widget, b, 
                       offset + a->len, n_atom->len, 
                       &n_atom->width, &n_atom->height);
    e = llist_insert_after(&l_head, a_ll, n_atom);
    errorcheck;
    
    a->width -= n_atom->width;
    UNSET_FLAG(n_atom->flags, ATOM_FLAG_LEFT);
    UNSET_FLAG(a->flags, ATOM_FLAG_RIGHT);
    return success;
}


static LList * next_atom ( LList * b_ll,
                           LList * p_ll,
                           LList * a_ll,
                           LList ** next_b_ll,
                           LList ** next_p_ll) {
    LList * next;
    next = llist_next(a_ll);
    if (!next) {
        p_ll = llist_next(p_ll);
        if (p_ll) {
            next = PARAGRAPH(p_ll)->atoms;
        } else {
            b_ll = llist_next(b_ll);
            if (b_ll) {
                p_ll = BLOCK(b_ll)->paragraphs;
                next = PARAGRAPH(p_ll)->atoms;
            }
        } 
    }
    if (next_b_ll) 
        *next_b_ll = b_ll;
    if (next_p_ll) 
        *next_p_ll = p_ll;
    return next;
}


static LList * prev_atom ( LList * b_ll,
                           LList * p_ll,
                           LList * a_ll,
                           LList ** prev_b_ll,
                           LList ** prev_p_ll ) {
    LList * prev;
    prev = llist_prev(a_ll);
    if (!prev) {
        p_ll = llist_prev(p_ll);
        if (p_ll) {
            prev = llist_last(PARAGRAPH(p_ll)->atoms);
        } else {
            b_ll = llist_prev(b_ll);
            if (b_ll) {
                p_ll = llist_last(BLOCK(b_ll)->paragraphs);
                prev = llist_last(PARAGRAPH(p_ll)->atoms);
            } 
        }
    }
    if (prev_b_ll) 
        *prev_b_ll = b_ll;
    if (prev_p_ll) 
        *prev_p_ll = p_ll;
    return prev;
}

/**
 * Checks the wrapping from atom a_ll on. Assume everything before
 * this atom is properly wrapped. Sets a_ll's width.
 */
static g_error wrap ( text_widget * widget,
                      block * b,
                      paragraph * p, 
                      LList * a_ll ) {
    g_error e;
    atom * a;
    LList * l;
    u8 wrap_para = FALSE;
    u16 offset = 0, t_offset = 0, p_offset = 0;
    u16 len;
    s16 w, h, width, height, line_width, next_line_word_w;
    u16 widget_width = widget->width - 2*widget->border_h;

    assert (widget && b && p && a_ll);

    for (l = b->paragraphs; PARAGRAPH(l) != p; l = llist_next(l)) 
        offset += PARAGRAPH(l)->len;
    p_offset = offset;
    for (l = p->atoms; l != a_ll; l = llist_next(l)) {
        offset += ATOM(l)->len;
    }

    a = ATOM(a_ll);
    /* Case 1: if this atom is the start of a line AND a wrappable
       substring is shorter than the remaining width of the previous
       line, re-wrap from end of previous line. */
    if ( GET_FLAG(a->flags, ATOM_FLAG_LEFT) && 
         llist_prev(a_ll) ) {
        /* Get length of smallest wrappable chunk of current atom */
        for (len = 0; ((len < ATOM(a_ll)->len) && 
                       !(break_char(block_char_at(b, offset + len))));
             len++)
            ;
        block_string_size (widget, b, offset, len, &width, &h);

        for (l = llist_prev(a_ll), line_width = 0, t_offset = offset;
             l;
             l = llist_prev(l)) {
            t_offset -= ATOM(l)->len;
            block_string_size (widget, b, t_offset, ATOM(l)->len, &w, &h);
            line_width += w;
            if (GET_FLAG((ATOM(l)->flags), ATOM_FLAG_LEFT))
                break;
        } 
        
        if (width + line_width < widget_width) {
            a_ll = llist_prev(a_ll);
            e = rewrap(widget, b, p, 
                       a_ll, 
                       offset - ATOM(a_ll)->len,
                       p_offset);
            errorcheck;
            return;
        }
    }

    /* Get the width of the first word of the next line's first atom,
     * if any */
    next_line_word_w = 0;
    for (l = a_ll, t_offset = offset; 
         !GET_FLAG(ATOM(l)->flags, ATOM_FLAG_RIGHT); l = llist_next(l)) 
        t_offset += ATOM(l)->len;
    if (l = llist_next(l)) {
        for (len = 0; ((len < ATOM(l)->len) && 
                       !(break_char(block_char_at(b, t_offset + len))));
             len++)
            ;        
        block_string_size (widget, b, t_offset, len, &next_line_word_w, &h);
    }
    
    /* Case 2: This atom's line is too wide */
    for (width = 0, t_offset = offset, l = a_ll; 
         !GET_FLAG(ATOM(l)->flags, ATOM_FLAG_LEFT); 
         l = llist_prev(l)) {
        t_offset -= ATOM(l)->len;
    }
    while (TRUE) {
        textedit_str_size ( widget->self,
			    (TEXTEDIT_UCHAR *) block_str_at(b, t_offset),
                            ATOM(l)->len, &w, &h);
        t_offset += ATOM(l)->len;
        width += w;
        if (GET_FLAG(ATOM(l)->flags, ATOM_FLAG_RIGHT)) {
            /* If this atom does not end with a break char and there
               is an atom after this one, add the width of the first
               word of the next atom */
            if (llist_next(l) && !break_char(block_char_at(b, t_offset - 1))) {
                width += next_line_word_w;
            }
            break;
        }
        l = llist_next(l);
    }

    if (width >= widget_width) {
        e = rewrap(widget, b, p, a_ll, offset, p_offset);
        errorcheck;
    }
    /* Case 3: This line is too short; we can pull a word from the
       next line */
    else if ((next_line_word_w) && (width + next_line_word_w < widget_width)) { 
        e = rewrap(widget, b, p, a_ll, offset, p_offset);
        errorcheck;
    } else {
        block_string_size(widget, b, offset, a->len, &a->width, &a->height);
    }

    if (GET_FLAG(p->flags, PARAGRAPH_FLAG_H_INVALID)) {
        UNSET_FLAG(p->flags, PARAGRAPH_FLAG_H_INVALID);
        widget->v_height -= p->height;
        for (p->height = 0, l = p->atoms; l; l = llist_next(l)) {
            if (GET_FLAG(ATOM(l)->flags, ATOM_FLAG_RIGHT)) {
                p->height += ATOM(l)->height;
            }
        }
        widget->v_height += p->height;
    }
    return success;
} 


/**
 * Re-wrap text from the atom a_ll on. a_ll's len and flags may be
 * incorrect. Atoms after a_ll will be removed and replaced. Offset is
 * the block offset to the start of a_ll (ie. the last place we know
 * for sure to be accurate), and p_offset is the offset of the start of the 
 * paragraph within block b.
 */
static g_error rewrap ( text_widget * widget,
                        block * b,
                        paragraph * p, 
                        LList * a_ll, 
                        u16 offset,
                        u16 p_offset) {
    g_error e;
    LList * l, * l_head;
    s16 w, h, width;
    u16 p_height;
    u16 a_start, a_len, a_width; 
    u8  atom_redraw, set_cursor;
    atom * a;
        
    /* Set a_ll and offset to be the start of a line */
    while (!GET_FLAG(ATOM(a_ll)->flags, ATOM_FLAG_LEFT)) {
        offset -= ATOM(a_ll)->len;
        a_ll = llist_prev(a_ll);
    }

    /* Destroy atoms after a_ll */
    for (l = llist_next(a_ll); l && (l != a_ll); l = llist_next(a_ll)) {
        atom_destroy(ATOM(l));
        llist_remove(&l_head, l);
    }

    /* Set default width and height to that of a null str */
    textedit_str_size(widget->self, NULL, 0, &w, &h);
    
    /* Scan char-by-char through paragraph to build atoms */
    a = ATOM(a_ll);
    UNSET_FLAG(a->flags, ATOM_FLAG_RIGHT);
    for (a_start = offset, a_len = 0, a_width = 0, width = 0;
         offset < p_offset + p->len; 
         offset++) {
        block_char_size (widget, b, offset, &w, &h);
        width += w;

        if (width >= widget->width - 2*widget->border_h) {
            if (a_len == 0) {
                /* Single wide word */
                a_len = offset - a_start;
                a->width = width - w;
            } else {
                a->width = a_width;
            }
            a->height = h;
            a->len = a_len;
            SET_FLAG(a->flags, ATOM_FLAG_RIGHT);
            a_start = a_start + a_len;
            a_len = 0;
            a_width = 0;
            width = 0;
            
            e = atom_create(&a, ATOM_TEXT, ATOM_FLAG_LEFT);
            errorcheck;

            e = llist_append(&l_head, a_ll, a);
            errorcheck;

            offset = a_start + a_len - 1;
        } else if (break_char(block_char_at(b, offset))) {
            a_len = offset - a_start + 1;
            a_width = width;
        }
    }

    /* Last atom */
    a->len = offset - a_start;
    a->width = width;
    a->height = h;
    SET_FLAG(a->flags, ATOM_FLAG_RIGHT);
    

    /* Mark paragraph for redraw */
    SET_FLAG(p->flags, PARAGRAPH_FLAG_DRAW);

    /* Scan through paragraph atoms to do the following three things:
     *  1. If paragraph contains cursor, find cursor atom and break
     *     atoms apart as needed
     *  2. Mark atoms after a_ll as needing to be redrawn 
     *  3. Calculate paragraph height
     */
    for (atom_redraw = FALSE, set_cursor = FALSE, p_height = 0, 
             l = p->atoms, offset = p_offset; l; l = llist_next(l)) {
        /* 1 */
        if ((p == PARAGRAPH(b->cursor_paragraph)) && (!set_cursor)) {
            if ((offset <= b->b_gap) && 
                (offset + ATOM(l)->len > b->b_gap)) {
                e = atom_create(&a, ATOM_TEXT, ATOM(l)->flags);
                errorcheck;

                e = llist_insert_after(&l_head, l, a);
                errorcheck;

                UNSET_FLAG(ATOM(l)->flags, ATOM_FLAG_RIGHT);
                UNSET_FLAG(a->flags, ATOM_FLAG_LEFT);
                a->height = ATOM(l)->height;
                a->len = offset + ATOM(l)->len - b->b_gap;
                ATOM(l)->len -= a->len;
                block_string_size( widget, b, offset + ATOM(l)->len, a->len, 
                                   &a->width, &h);
                ATOM(l)->width -= a->width;
            } 
            offset += ATOM(l)->len;
            if (offset == b->b_gap) {
                b->cursor_atom = l;
                set_cursor = TRUE;
            }
        }
        /* 2 */
        if (l == a_ll)
            atom_redraw = TRUE;
        if (atom_redraw)
            SET_FLAG(ATOM(l)->flags, ATOM_FLAG_DRAW);
        /* 3 */
        if (GET_FLAG(ATOM(l)->flags, ATOM_FLAG_RIGHT))
            p_height += ATOM(l)->height;
    }
    
    /* If paragraph height changed, modify widget's virtual height and
     * indicate that we need to redraw paragraphs after this one */
    if (p_height != p->height) {
        widget->v_height -= p->height;
        p->height = p_height;
        widget->v_height += p->height;
        UNSET_FLAG(p->flags, PARAGRAPH_FLAG_H_INVALID);
        SET_FLAG(p->flags, PARAGRAPH_FLAG_DRAW_AFTER);
    }

    /* Calculate the cursor's virtual y position */
    widget_cursor_v_y (widget);
    return success;
}
