/* $Id$
 *
 * terminal.c - a character-cell-oriented display widget for terminal
 *              emulators and things.
 *
 *     References:
 *         - linux console_codes manpage
 *         - http://vt100.net/docs/vt102-ug/contents.html
 *         - http://www.ibiblio.org/pub/historic-linux/ftp-archives/tsx-11.mit.edu/Oct-07-1996/info/xterm-seqs2.txt
 *         - The (messy) source to several other terminal emulators
 *
 *     FIXME: Writing a terminal emulator is a pain... every terminal
 *            emulator I've seen is so messy and convoluted that its
 *            code can't be reused. This file should eventually be
 *            turned into a library so that everybody who wants to
 *            write a terminal doesn't have to do their own emulation
 *            code.
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

#include <pgserver/common.h>
#include <pgserver/terminal.h>
#define WIDGET_SUBCLASS 0
#define DATA WIDGET_DATA(terminaldata)

#ifdef DEBUG_TERMINAL_VT102
#define DEBUG_FILE
#endif
#include <pgserver/debug.h>

void term_char_escapemode(struct widget *self,u8 c);
void term_ecma48sgr(struct widget *self);
void term_csi(struct widget *self, u8 c);
void term_decset(struct widget *self,int n,int enable);
void term_ecmaset(struct widget *self,int n,int enable);
void term_ecmastatus(struct widget *self,int n);
int term_misc_code(struct widget *self,u8 c);
void term_xterm(struct widget *self);

#ifdef DEBUG_FILE
void term_debug_printbuffer(struct widget *self) {
  u8 *p;
  
  /* properly terminate the buffer */
  if (DATA->escbuf_pos < ESCAPEBUF_SIZE)
    DATA->escapebuf[DATA->escbuf_pos-1] = 0;
  else
    DATA->escapebuf[ESCAPEBUF_SIZE] = 0;

  /* Convert escapes to "^" characters so they don't mess up our terminal */
  p = DATA->escapebuf;
  while (*p) {
    if (*p == '\033')
      *p = '^';
    p++;
  }
  
  DBG("buffer = \"%s\"\n", DATA->escapebuf);
}
#else
#define term_debug_printbuffer
#endif


/********************************************** Keyboard input */

/* Handle a single key event as received from the server */
void kbd_event(struct widget *self, int pgkey,int mods) {
  u8 *rtn;

  /* FIXME: using a static buffer here breaks if there is more than one
   *        character on the event queue at once, even in different instances!
   */
  static u8 chrstr[] = " ";      /* Must be static - if the event is put on
				      the queue it might be a while before this
				      is used. */

  /* Control keys */
  if ((mods & PGMOD_CTRL) && pgkey >= PGKEY_a && pgkey <= PGKEY_z)
    pgkey -= PGKEY_a - 1;

  /* ASCII keys */
  if (pgkey < 127)
    *chrstr = pgkey;
  else
    *chrstr = 0;
  rtn = chrstr;

  /* Special keys */
  switch (pgkey) {
    
  case PGKEY_RETURN:        rtn = "\n";       break;
    
  case PGKEY_UP:            rtn = "\033[A";   break;
  case PGKEY_DOWN:          rtn = "\033[B";   break;
  case PGKEY_RIGHT:         rtn = "\033[C";   break;
  case PGKEY_LEFT:          rtn = "\033[D";   break;
  case PGKEY_HOME:          rtn = "\033[H";  break;
  case PGKEY_INSERT:        rtn = "\033[2~";  break;
  case PGKEY_DELETE:        rtn = "\033[3~";  break;
  case PGKEY_END:           rtn = "\033[F";  break;
  case PGKEY_PAGEUP:        rtn = "\033[5~";  break;
  case PGKEY_PAGEDOWN:      rtn = "\033[6~";  break;
      
  case PGKEY_F1:            rtn = "\033[11~";break;
  case PGKEY_F2:            rtn = "\033[12~";break;
  case PGKEY_F3:            rtn = "\033[13~";break;
  case PGKEY_F4:            rtn = "\033[14~";break;
  case PGKEY_F5:            rtn = "\033[15~";break;
  case PGKEY_F6:            rtn = "\033[17~";break;   /* Yes, this isn't supposed to be 16 */
  case PGKEY_F7:            rtn = "\033[18~";break;
  case PGKEY_F8:            rtn = "\033[19~";break;
  case PGKEY_F9:            rtn = "\033[20~";break;
  case PGKEY_F10:           rtn = "\033[21~";break;
  case PGKEY_F11:           rtn = "\033[23~";break;
  case PGKEY_F12:           rtn = "\033[24~";break;
  }

  /* Implement key prefix switch
   *
   *  Note: This is supposed to replace ESC [ with ESC O in the above
   *        sequences, but that doesn't seem to please emacs or vim.
   *        It seems to work correctly if instead this replaces backspace
   *        with '\177'.
   */
  if (DATA->current.key_prefix_switch && pgkey == PGKEY_BACKSPACE)
    rtn[0] = '\177';

  if (*rtn) {
    /* After translating a key, send back the string. */
    post_event(PG_WE_DATA,self,strlen(rtn),0,rtn); 
  }
}


/********************************************** Character output */

/* Format and/or draw one incoming character. 
 * Handle escape codes if present. */
void term_char(struct widget *self,u8 c) {
  term_setcursor(self,0);  /* Hide */
  
  /* Clamp by default rather than wrapping/scrolling */
  DATA->clamp_flag = 1;

  /* Is it a control character? 
   * Note:
   *  Normally control characters and escapes are completely
   *  separate, but xterm extended codes contain the BEL
   *  character, so we have to exclude that if we're in escape mode.
   */
  if (c <= '\033' && (c != '\a' || !DATA->escapemode))
    switch (c) {
    case '\033':        /* Escape */
      if (DATA->escapemode) {
	DBG("-ERROR- beginning escape code while already in escape mode\n");
	term_debug_printbuffer(self);
      }
      DATA->escapemode = 1;
      DATA->escbuf_pos = 0;
      return;
      
    case '\a':
      /* Ding! */
      DBG("beep\n");
      drivermessage(PGDM_SOUNDFX,PG_SND_BEEP,NULL);
      return;
      
    case '\n':
    case '\013':
    case '\014':
      DBG("newline\n");
      DATA->current.crsry++;
      DATA->clamp_flag = 0;
      if (DATA->current.crlf_mode)
	DATA->current.crsrx = 0;
      break;

    case '\r':
      DBG("carriage return\n");
      DATA->current.crsrx = 0;
      break;
      
    case '\t':
      DBG("tab\n");
      if (DATA->current.crsrx >= DATA->bufferw) {
        DATA->current.crsrx = 8;	/* "magic" wrapping */
        DATA->current.crsry++;
	DATA->clamp_flag = 0;
      }
      else
      /* Not sure this is right, but it's consistant with observed behavior */
        DATA->current.crsrx += 8 - (DATA->current.crsrx & 7);
      break;
      
    case '\b':
      DBG("backspace\n");
      if (!DATA->current.crsrx)  /* Can't backspace past the edge */
	return;
      DATA->current.crsrx--;
      break;

    case '\017':   /* SI, select the G0 character set */
      DBG("Select G0 character set\n");
      DATA->current.charset = DATA->current.g[0];
      break;

    case '\016':   /* SO, select the G1 character set */
      DBG("Select G1 character set\n");
      DATA->current.charset = DATA->current.g[1];
      break;

    }

  /* Handling an escape code? */
  else if (DATA->escapemode)
    term_char_escapemode(self,c);

  /* Normal character */
  else
    {
      if (DATA->current.crsrx >= DATA->bufferw && !DATA->current.no_autowrap) {
        DATA->current.crsrx = 0;	/* "magic" wrapping */
        DATA->current.crsry++;
	DATA->clamp_flag = 0;
	if (DATA->current.crsry >= DATA->bufferh) {  /* Scroll vertically */
	  DATA->current.crsry = DATA->bufferh-1;
	  term_scroll(self,DATA->current.scroll_top,DATA->current.scroll_bottom,-1);
	}
      }
      DBG("character '%c' (%d)\n", c, c);
      if (DATA->current.insert_mode)
	term_insert(self, 1);

      /* Map the line drawing character set into the upper half of a byte
       * not used by ASCII. This should work fine even if we're doing Unicode.
       */
      if (DATA->current.charset == '0')
	c |= 0x80;

      term_plot(self,DATA->current.crsrx++,DATA->current.crsry,c);
    }

  if (DATA->clamp_flag) {
    /* Clamp the cursor to the screen edges */
    if (DATA->current.crsrx < 0)
      DATA->current.crsrx = 0;
    if (DATA->current.crsry < 0)
      DATA->current.crsry = 0;
    if (DATA->current.crsrx >= DATA->bufferw)
      DATA->current.crsrx = DATA->bufferw - 1;
    if (DATA->current.crsry >= DATA->bufferh)
      DATA->current.crsry = DATA->bufferh - 1;
  }
  else {
    /* Wrap around the right edge, and scroll at the top and bottom edges
     * of the scrolling region.
     */
    if (DATA->current.crsrx < 0)
      DATA->current.crsrx = 0;
    else if (DATA->current.crsrx > DATA->bufferw) {  /* Wrap around the side */
      DATA->current.crsrx = 0;
      DATA->current.crsry++;
    }
    if (DATA->current.crsry < DATA->current.scroll_top) {
      DATA->current.crsry = DATA->current.scroll_top;
      term_scroll(self,DATA->current.scroll_top,DATA->current.scroll_bottom,1);
    }
    else if (DATA->current.crsry > DATA->current.scroll_bottom) {  /* Scroll vertically */
      DATA->current.crsry = DATA->current.scroll_bottom;
      term_scroll(self,DATA->current.scroll_top,DATA->current.scroll_bottom,-1);
    }
  }
  
  term_setcursor(self,1);  /* Show cursor */
}


/********************************************** Escape codes */

/* Reset the terminal emulator */
void term_reset(struct widget *self) {
  memset(&DATA->current, 0, sizeof(DATA->current));
  DATA->current.attr = DATA->attr_default;
  DATA->current.scroll_bottom = DATA->bufferh - 1;
  term_clearbuf(self,0,0,DATA->bufferw * DATA->bufferh);
}    

/* Handle an incoming character while processing an escape sequence */
void term_char_escapemode(struct widget *self,u8 c) {
  /* Yer supposed to ignore nulls in escape codes */
  if (!c)
    return;
  
  /* Too much? */
  if (DATA->escbuf_pos >= ESCAPEBUF_SIZE) {
    DATA->escapemode = 0;
    DBG("-ERROR- : buffer overflowed before escape was recognized\n");
    term_debug_printbuffer(self);
    return;
  }

  /* Append to the escape code buffer */
  DATA->escapebuf[DATA->escbuf_pos++] = c;

  /* Handle miscellaneous (non-CSI) escapes */
  if (term_misc_code(self,c))
      DATA->escapemode = 0;

  /* Handle CSI escape codes that can all be parsed similarly */ 
  else if (*DATA->escapebuf == '[' && DATA->escbuf_pos>1) 
    term_csi(self,c);

  /* Handle xterm extended escapes */
  else if (*DATA->escapebuf == ']' && c=='\a')
    term_xterm(self);
}


/* Handle codes that start with a CSI (ESC [) 
 * CSI codes start with CSI, an optional question mark, then optionally list numerical arguments
 * separated by a semicolon. A char other than semicolon or a digit identifies
 * and terminates the code
 */
void term_csi(struct widget *self, u8 c) {
  u8 *p,*endp,*num_end;
  int i;
  
  /* If there's more, come back later */
  if (isdigit(c) || c==';' || c=='?')
    return;
  
  /* We're ending the code */
  DATA->escapemode = 0;
  
  /* Numericalize the arguments */
  DATA->num_csiargs = 0;
  /* Unspecified params must be zero */
  memset(&DATA->csiargs,0,sizeof(int) * CSIARGS_SIZE);
  p = DATA->escapebuf+1;                     /* Start after the CSI */
  if (*p == '?')                             /* Ignore the '?' for now */
    p++;
  endp = DATA->escapebuf+DATA->escbuf_pos-1; /* Stop before the end */
  while (p<endp) {
    /* Convert the number */
    DATA->csiargs[DATA->num_csiargs] = strtol(p,(char**)&num_end,10);
    
    /* Got nothing good? */
    if (p==num_end)
      break;
    
    /* Increment but check for overflow */
    DATA->num_csiargs++;
    if (DATA->num_csiargs >= CSIARGS_SIZE)
      break;
    
    /* Skip to the next one */
    p = num_end+1;
  }
  
  /* Do the code-specific processing */

  switch (c) {

    /* @ - Insert the indicated # of blank characters */
  case '@':
    DBG("Insert %d blank characters\n", DATA->csiargs[0]);
    term_insert(self, DATA->csiargs[0] ? DATA->csiargs[0] : 1);
    break;

    /* A - Move cursor up */
  case 'A':
    DBG("move cursor up\n");
    DATA->current.crsry -= DATA->csiargs[0] ? DATA->csiargs[0] : 1;
    break;

    /* B - Move cursor down */
  case 'B':
    DBG("move cursor down\n");
    DATA->current.crsry += DATA->csiargs[0] ? DATA->csiargs[0] : 1;
    break;

    /* C - Move cursor right */
  case 'C':
    DBG("move cursor right by %d\n", DATA->csiargs[0]);
    DATA->current.crsrx += DATA->csiargs[0] ? DATA->csiargs[0] : 1;
    break;

    /* D - Move cursor left */
  case 'D':
    DBG("move cursor left by %d\n", DATA->csiargs[0]);
    DATA->current.crsrx -= DATA->csiargs[0] ? DATA->csiargs[0] : 1;
    break;

    /* E - Move cursor down and to column 1 */
  case 'E':
    DBG("move cursor down and to column 1\n");
    DATA->current.crsry += DATA->csiargs[0] ? DATA->csiargs[0] : 1;
    DATA->current.crsrx = 0;
    break;

    /* F - Move cursor up and to column 1 */
  case 'F':
    DBG("move cursor up and to column 1\n");
    DATA->current.crsry -= DATA->csiargs[0] ? DATA->csiargs[0] : 1;
    DATA->current.crsrx = 0;
    break;

    /* G - Set column */
  case 'G':
    DBG("set column to %d\n", DATA->csiargs[0]);
    DATA->current.crsrx = DATA->csiargs[0] ? DATA->csiargs[0] - 1 : 0;
    break;

    /* H - Set row,column */
  case 'H':
    DBG("set row,column to %d, %d\n", DATA->csiargs[0], DATA->csiargs[1]);
    DATA->current.crsry = DATA->csiargs[0] ? DATA->csiargs[0] - 1 : 0;
    DATA->current.crsrx = DATA->csiargs[1] ? DATA->csiargs[1] - 1 : 0;
    break;

    /* J - Erase */
  case 'J':
    switch (DATA->csiargs[0]) {
    case 1:
      DBG("erase from start to cursor\n");
      term_clearbuf(self,0,0,DATA->current.crsry * DATA->bufferw + DATA->current.crsrx + 1);
      break;
    case 2:
      DBG("erase whole display\n");
      term_clearbuf(self,0,0,DATA->bufferw * DATA->bufferh);
      break;
    default:
      DBG("erase from cursor to end of display\n");
      term_clearbuf(self,DATA->current.crsrx,DATA->current.crsry,
		    (DATA->bufferw * DATA->bufferh) -
		    (DATA->current.crsrx + DATA->current.crsry * DATA->bufferw));
    }
    break;

    /* K - Erase Line */
  case 'K':
    switch (DATA->csiargs[0]) {
    case 1:
      DBG("erase from start of line to cursor\n");
      term_clearbuf(self,0,DATA->current.crsry,DATA->current.crsrx + 1);
      break;
    case 2:
      DBG("erase whole line\n");
      term_clearbuf(self,0,DATA->current.crsry,DATA->bufferw);
      break;
    default:
      DBG("erase from cursor to end of line\n");
      term_clearbuf(self,DATA->current.crsrx,DATA->current.crsry,DATA->bufferw-DATA->current.crsrx);
    }
    break;

    /* L - Insert blank lines */
  case 'L':
    DBG("insert %d blank lines\n", DATA->csiargs[0]);
    term_scroll(self,DATA->current.crsry,DATA->current.scroll_bottom,DATA->csiargs[0] ? DATA->csiargs[0] : 1);
    break;

    /* M - Delete lines */
  case 'M':
    DBG("delete %d lines\n", DATA->csiargs[0]);
    term_scroll(self,DATA->current.crsry,DATA->current.scroll_bottom,-(DATA->csiargs[0] ? DATA->csiargs[0] : 1));
    break;

    /* P - Delete characters */
  case 'P':
    DBG("delete %d characters\n", DATA->csiargs[0] ? DATA->csiargs[0] : 1);
    term_delete(self, DATA->csiargs[0]);
    break;
    
    /* X - Erase characters */
  case 'X':
    DBG("erase %d characters\n", DATA->csiargs[0] ? DATA->csiargs[0] : 1);
    for (i=0;i<DATA->csiargs[0];i++)
      term_char(self,' ');
    break;

    /* a - Move cursor right */
  case 'a':
    DBG("move right %d columns\n", DATA->csiargs[0]);
    DATA->current.crsrx += DATA->csiargs[0] ? DATA->csiargs[0] : 1;
    break;
    
    /* c - Identify as a VT102 terminal */
  case 'c':
    {
      static const char *response = "\e[?6c";
      DBG("identifying as a VT102 terminal\n");
      post_event(PG_WE_DATA,self,strlen(response),0,(char*)response);
    }
    break;

    /* d - Set row */
  case 'd':
    DBG("set row to %d\n", DATA->csiargs[0]);
    DATA->current.crsry = DATA->csiargs[0] ? DATA->csiargs[0] - 1 : 0;
    break;

    /* e - Move cursor down */
  case 'e':
    DBG("move down %d rows\n", DATA->csiargs[0]);
    DATA->current.crsry += DATA->csiargs[0] ? DATA->csiargs[0] : 1;
    break;

    /* f - Set row,column */
  case 'f':
    DBG("set row, column to %d, %d\n", DATA->csiargs[0], DATA->csiargs[1]);
    DATA->current.crsry = DATA->csiargs[0] ? DATA->csiargs[0] - 1 : 0;
    DATA->current.crsrx = DATA->csiargs[1] ? DATA->csiargs[1] - 1 : 0;
    break;

    /* g - Delete tab stops */
  case 'g':
    if (DATA->num_csiargs == 0) {
      DBG("-UNIMPLEMENTED- delete current tab stop\n");
    }
    else if (DATA->num_csiargs == 1 && DATA->csiargs[0] == 3) {
      DBG("-UNIMPLEMENTED- delete all tab stops\n");
    }
    else {
      DBG("-ERROR- Unknown tab stop deletion escape\n");
    }
    break;

    /* l and h - DECSET/DECRST sequence */
  case 'h':  /* SET */
  case 'l':  /* RST */
    for (i=0; i<DATA->num_csiargs; i++) {
      if (DATA->escapebuf[1]=='?')
	term_decset(self,DATA->csiargs[i],c=='h');
      else
	term_ecmaset(self,DATA->csiargs[i],c=='h');
    }
    break;

    /* m - ECMA48 SGR sequence */
  case 'm':
    term_ecma48sgr(self);
    break;

    /* n - Status report */
  case 'n':
    term_ecmastatus(self,DATA->csiargs[0]);
    break;

    /* q - Keyboard LEDs */
  case 'q':
    DBG("ignoring request to set keyboard LEDs\n");
    break;

    /* r - Set scrolling region */
  case 'r':
    if (DATA->num_csiargs == 2) {
      DBG("set scrolling region to %d, %d\n", DATA->csiargs[0],DATA->csiargs[1]);
      DATA->current.scroll_top = DATA->csiargs[0] - 1;
      DATA->current.scroll_bottom = DATA->csiargs[1] ? DATA->csiargs[1] - 1 : DATA->bufferh - 1;
    }
    else {
      DBG("set scrolling region to (0), %d\n", DATA->csiargs[0]);
      DATA->current.scroll_top = 0;
      DATA->current.scroll_bottom = DATA->csiargs[0] ? DATA->csiargs[0] - 1 : DATA->bufferh - 1;
    }
    break;

    /* s - Save cursor position */
  case 's':
    DBG("save cursor position\n");
    DATA->current.savcrsry = DATA->current.crsry;
    DATA->current.savcrsrx = DATA->current.crsrx;
    break;

    /* u - Restore cursor position */
  case 'u':
    DBG("restore cursor position\n");
    DATA->current.crsry = DATA->current.savcrsry;
    DATA->current.crsrx = DATA->current.savcrsrx;
    break;

    /* ` - Move cursor to indicated column in current row */
  case '`':
    DBG("move cursor to column %d in current row\n", DATA->csiargs[0]);
    DATA->current.crsrx = DATA->csiargs[0] - 1;
    break;
    
  default:
      DBG("-ERROR- : Unknown final character in CSI escape = %c (%d)\n",c,c);
      break;

  }
}


/* Handle a parsed ECMA-48 CSI sequence */
void term_ecma48sgr(struct widget *self) {
  int *arg = DATA->csiargs;
  if(!DATA->num_csiargs)	/* no arg = reset */
    DATA->current.attr = DATA->attr_default;

  /* If this is an empty CSI m escape, treat it like a reset */
  if (DATA->num_csiargs == 0)
    DATA->num_csiargs = 1;

  for (;DATA->num_csiargs;DATA->num_csiargs--,arg++) {
    switch (*arg) {

      /* 0 - reset to normal */
    case 0:
      DBG("reset\n");
      DATA->current.attr = DATA->attr_default;
      DATA->current.reverse_video = 0;
      break;

      /* 1 - bold */
      /* 4 - underline (we treat it like a bold) */
    case 1:
    case 4:
      DBG("bold/underline\n");
      DATA->current.attr |= 0x08;
      break;

      /* 5 - blink (not really, it's like bold for backgrounds) */
    case 5:
      DBG("blink\n");
      DATA->current.attr |= 0x80;
      break;

    case 10:  /* 10 - reset selected mapping, display control flag, toggle meta flag */
    case 11:  /* 11 - select null mapping, set display control flag, reset toggle meta flag */
    case 12:  /* 12 - select null mapping, set display control flag, set toggle meta flag */
      DBG("-UNIMPLEMENTED- CSI mapping flag\n");
      break;

      /* 7 - reverse video on */
    case 7:
      DBG("reverse video on\n");
      DATA->current.reverse_video = 1;
      break;

      /* 27 - reverse video off */
    case 27:
      DBG("reverse video off\n");
      DATA->current.reverse_video = 0;
      break;

      /* 21 or 22 - normal intensity */
      /* 24 - underline off */
    case 21:
    case 22:
    case 24:
      DBG("normal intensity, underline off\n");
      DATA->current.attr &= 0xF7;
      break;

      /* 25 - blink off */
    case 25:
      DBG("blink off\n");
      DATA->current.attr &= 0x7F;
      break;
      
      /* 30 through 37 - foreground colors */
    case 30: DBG("fg black\n");   DATA->current.attr = (DATA->current.attr & 0xF8) | 0x00; break;
    case 31: DBG("fg red\n");     DATA->current.attr = (DATA->current.attr & 0xF8) | 0x01; break;
    case 32: DBG("fg green\n");   DATA->current.attr = (DATA->current.attr & 0xF8) | 0x02; break;
    case 33: DBG("fg brown\n");   DATA->current.attr = (DATA->current.attr & 0xF8) | 0x03; break;
    case 34: DBG("fg blue\n");    DATA->current.attr = (DATA->current.attr & 0xF8) | 0x04; break;
    case 35: DBG("fg magenta\n"); DATA->current.attr = (DATA->current.attr & 0xF8) | 0x05; break;
    case 36: DBG("fg cyan\n");    DATA->current.attr = (DATA->current.attr & 0xF8) | 0x06; break;
    case 37: DBG("fg white\n");   DATA->current.attr = (DATA->current.attr & 0xF8) | 0x07; break;

      /* 38 - set default foreground, underline on */
    case 38:
      DBG("default foreground, underline on\n");
      DATA->current.attr = (DATA->current.attr & 0xF0) | ((DATA->attr_default & 0x0F) | 0x08);
      break;

      /* 39 - set default foreground, underline off */
    case 39:
      DBG("default foreground, underline off\n");
      DATA->current.attr = (DATA->current.attr & 0xF0) | (DATA->attr_default & 0x0F);
      break;

      /* 40 through 47 - background colors */
    case 40: DBG("bg black\n");   DATA->current.attr = (DATA->current.attr & 0x8F) | 0x00; break;
    case 41: DBG("bg red\n");     DATA->current.attr = (DATA->current.attr & 0x8F) | 0x10; break;
    case 42: DBG("bg green\n");   DATA->current.attr = (DATA->current.attr & 0x8F) | 0x20; break;
    case 43: DBG("bg brown\n");   DATA->current.attr = (DATA->current.attr & 0x8F) | 0x30; break;
    case 44: DBG("bg blue\n");    DATA->current.attr = (DATA->current.attr & 0x8F) | 0x40; break;
    case 45: DBG("bg magenta\n"); DATA->current.attr = (DATA->current.attr & 0x8F) | 0x50; break;
    case 46: DBG("bg cyan\n");    DATA->current.attr = (DATA->current.attr & 0x8F) | 0x60; break;
    case 47: DBG("bg white\n");   DATA->current.attr = (DATA->current.attr & 0x8F) | 0x70; break;

      /* 49 - default background */
    case 49:
      DBG("default background\n");
      DATA->current.attr = (DATA->current.attr & 0x0F) | (DATA->attr_default & 0xF0);
      break;

    default:
      DBG("-ERROR- : Unknown ECMA-48 SGR number = %d\n",*arg);
      break;
    }
  }
}


/* Handle various miscellaneous escape codes,
 * return 1 if a code is handled
 */
int term_misc_code(struct widget *self,u8 c) {
  /****** Single-character codes */

  if (DATA->escbuf_pos == 1)

    switch (c) { 

      /* ESC c - reset */
    case 'c':
      DBG("reset\n");
      term_reset(self);
      return 1;

      /* ESC D - linefeed */
    case 'D':
      DBG("linefeed\n");
      DATA->current.crsry++;
      DATA->clamp_flag = 0;
      return 1;

      /* ESC E - newline */
    case 'E':
      DBG("newline\n");
      DATA->current.crsrx = 0;
      DATA->clamp_flag = 0;
      return 1;

      /* ESC H - set tabstop */
    case 'H':
      DBG("-UNIMPLEMENTED- set tabstop\n");
      return 1;

      /* ESC M - reverse linefeed */
    case 'M':
      DBG("reverse linefeed\n");
      DATA->current.crsry--;
      DATA->clamp_flag = 0;
      return 1;

      /* ESC Z - DEC private information, return ESC [ ? 6 c */
    case 'Z':
      {
      static const char *response = "\e[?6c";
      DBG("DEC private information, identify as VT102\n");
      post_event(PG_WE_DATA,self,strlen(response),0,(char*)response);      
      }
      return 1;

      /* ESC 7 - save state */
    case '7':
      DBG("save state\n");
      memcpy(&DATA->saved,&DATA->current,sizeof(DATA->saved));
      return 1;

      /* ESC 8 - restore state */
    case '8':
      DBG("restore state\n");
      memcpy(&DATA->saved,&DATA->saved,sizeof(DATA->current));
      return 1;

      /* ESC > - Set numeric keypad mode */
    case '>':
      DBG("ignoring request to set numeric keypad mode\n");
      return 1;

      /* ESC = - Set application keypad mode */
    case '=':
      DBG("ignoring request to set application keypad mode\n");
      return 1;
    }


  /****** Two-character codes */

  if (DATA->escbuf_pos == 2)
    switch (DATA->escapebuf[0]) { 

      /* % - Select ISO 646/8859 vs Unicode */
    case '%':
      switch (c) {
      case '@':
	DBG("-UNIMPLEMENTED- disable UTF-8 mode\n");
	break;
      case 'G':
      case '8':
	DBG("-UNIMPLEMENTED- enable UTF-8 mode\n");
	break;
      default:
	DBG("-ERROR- Unknown ESC % code\n");
	break;
      }
      return 1;

      /* ( - Set G0 character set */
    case '(':
      DBG("Set G0 character set to '%c'\n", c);
      DATA->current.g[0] = c;
      return 1;

      /* ) - set G1 character set */
    case ')':
      DBG("Set G1 character set to '%c'\n", c);
      DATA->current.g[1] = c;
      return 1;

      /* * - set G2 character set */
    case '*':
      DBG("Set G2 character set to '%c'\n", c);
      DATA->current.g[2] = c;
      return 1;

      /* + - set G3 character set */
    case '+':
      DBG("Set G3 character set to '%c'\n", c);
      DATA->current.g[3] = c;
      return 1;

    case '#':
      /* #8 - DEC screen alignment test (fill screen with E's) */
      if (c == '8') {
	int i,j;
	DBG("DEC screen alignment test\n");
	for (j=0;j<DATA->bufferh;j++)
	  for (i=0;i<DATA->bufferw;i++)
	    term_plot(self,i,j,'E');
	return 1;
      }
      DBG("ignoring double-height/width code\n");
      /* The rest of the codes starting with ESC # are for 
       * double-height/width chars, not supported */
      return 1;

      /* ]R - Reset palette */
    case ']':
      if (c == 'R') {
	DBG("reset palette\n");
	handle_free(-1, DATA->htextcolors); 	/* Delete our copy of the palette if we have one */
	DATA->htextcolors = 0;
	set_widget_rebuild(self);               /* Necessary to update background color */
	return 1;
      }
      break;
    }

  /* Wonky code that doesn't fit anywhere else... */
  if (DATA->escbuf_pos == 9 && DATA->escapebuf[0] == ']' && DATA->escapebuf[1] == 'P') {
    char digits[8];
    int n;
    pgcolor color;

    /* Slurp the 7 digits after the escape into our buffer */
    memcpy(digits, DATA->escapebuf+2, 6);
    digits[6] = c;
    digits[7] = 0;

    /* All digits except the first are the color */
    sscanf(digits+1, "%X", &color);

    /* The first is the index */
    digits[1] = 0;
    sscanf(digits, "%X", &n);

    DBG("set palette entry %d to 0x%06X\n", n, color);
    term_setpalette(self, n, color);
    return 1;
  }

  return 0;
}

void term_ecmaset(struct widget *self,int n,int enable) {
  switch (n) {

    /* ESC [ 3 h - Display control characters */
  case 3:
    DBG("ignoring request to set display of control characters to %d\n", enable);
    break;

    /* ESC [ 4 h - Display control characters */
  case 4:
    DBG("setting insert mode to %d\n", enable);
    DATA->current.insert_mode = enable;
    break;
    
    /* ESC [ 20 h - Automatically follow echo of LF, VT, or FF with CR */
  case 20:
    DBG("CR echo set to %d\n", enable);
    DATA->current.crlf_mode = enable;
    break;

  default:
    DBG("-ERROR- : Unknown ECMA mode switch number = %d\n",DATA->csiargs[0]);
    break;
  }
}

void term_ecmastatus(struct widget *self,int n) {
  switch (n) {
    
    /* ESC [ 5 n - Device status report, answer ESC [ 0 n */
  case 5:
    {
      static const char *response = "\e[0n";
      DBG("answering device status report\n");
      post_event(PG_WE_DATA,self,strlen(response),0,(char*)response);
    }
    break;

    /* ESC [ 6 n - Cursor position report, answer ESC [ y ; x R */
  case 6:
    DBG("-UNIMPLEMENTED- cursor position report\n");
    break;

  default:
    DBG("-ERROR- : Unknown ECMA status report number = %d\n",DATA->csiargs[0]);
    break;
  }
}

void term_decset(struct widget *self,int n,int enable) {
  switch (n) {

    /* ESC [ ? 1 h - Cursor keys send ESC O rather than ESC [ when set */
  case 1:
    DBG("set cursor key prefix switch to %d\n", DATA->csiargs[0]);
    DATA->current.key_prefix_switch = DATA->csiargs[0];
    break;
    
    /* ESC [ ? 3 h - 80/132 column mode */
  case 3:
    DBG("ignoring 80/132 column switch\n");
    break;
    
    /* ESC [ ? 5 h - Set reverse video mode */
  case 5:
    DBG("-UNIMPLEMENTED- set reverse video mode\n");
    break;
    
    /* ESC [ ? 6 h - When set, cursor addressing is relative to the upper left corner of the scroll region */
  case 6:
    DBG("-UNIMPLEMENTED- scroll-relative cursor addressing\n");
    break;
    
    /* ESC [ ? 7 h - Autowrap on/off */
  case 7:
    DBG("setting autowrap to %d\n", enable);
    DATA->current.no_autowrap = !enable;
    break;

    /* ESC [ ? 8 h - Keyboard autorepeat */
  case 8:
    DBG("ignoring request to set keyboard autorepeat to %d\n", enable);
    break;

    /* ESC [ ? 9 h - X10 mouse reporting */
  case 9:
    DBG("setting X10 mouse reporting to %d\n", enable);
    DATA->current.x10_mouse = enable;
    break;

    /* ESC [ ? 25 h - Cursor on/off */
  case 25:
    DBG("setting cursor visibility to %d\n", enable);
    if (enable)
      DATA->current.cursor_hidden = 0;
    else {
      DATA->current.cursor_hidden = 1;
      term_setcursor(self,0);
    }
    break;

    /* ESC [ ? 47 h - Alternate screen buffer */
  case 47:
    DBG("-UNIMPLEMENTED- alternate screen buffer\n");
    break;

    /* ESC [ ? 1000 h - X11 mouse reporting */
  case 1000:
    DBG("setting X11 mouse reporting to %d\n", enable);
    DATA->current.x11_mouse = enable;
    break;

  default:
    DBG("-ERROR- : Unknown DECSET/DECRST number = %d\n",DATA->csiargs[0]);
    break;
  }
}

void term_xterm(struct widget *self) {
  int n;
  char *txt;

  /* xterm extended codes are all of the form:
   *   ESC ] n ; txt BEL
   */
  n = strtol(DATA->escapebuf+1, &txt, 10);
  txt++;
  DATA->escapebuf[DATA->escbuf_pos-1] = 0;
  DATA->escapemode = 0;

  switch (n) {

  case 0:   /* ESC ] 0 ; txt BEL - Set icon name and window title to txt */
  case 2:   /* ESC ] 2 ; txt BEL - Set window title to txt */
    DBG("setting window title to \"%s\"\n", txt);
    post_event(PG_WE_TITLECHANGE, self, strlen(txt), 0, txt); 
    break;

    /* ESC ] 0 ; txt BEL - Set icon name to txt */
  case 1:
    DBG("ignoring request to set icon name to \"%s\"\n", txt);
    break;

    /* ESC ] 4 6 ; name BEL - Change log file to name */
  case 46:
    DBG("ignoring request to set log file name to \"%s\"\n", txt);
    break;

    /* ESC ] 5 0 ; txt BEL - Set font to txt */
  case 50:
    DBG("ignoring request to set font to \"%s\"\n", txt);
    break;
    
  default:
    DBG("-ERROR- unknown xterm code %d - \"%s\"\n", n, txt);
  }
}

/* Send an x10/x11 mouse reporting code */
void term_mouse_event(struct widget *self, int press, int button) {
  static char event[] = "\033[Mbxy";

  DBG("press=%d, x=%d, y=%d, button=%d, modifiers=%d\n", 
      press, DATA->mouse_x, DATA->mouse_y, button, DATA->key_mods);

  /* X11 mouse reporting */
  if (DATA->current.x11_mouse) {
    int buttoncode = 0;

    if (press && button < 3)              buttoncode |= button; 
                                     else buttoncode |= 3;
    if (DATA->key_mods & PGMOD_SHIFT)     buttoncode |= 4;
    if (DATA->key_mods & PGMOD_ALT)       buttoncode |= 8;
    if (DATA->key_mods & PGMOD_CTRL)      buttoncode |= 16;

    DBG("using X11 mouse reporting\n");
    event[3] = '\040' + buttoncode;
    event[4] = '\040' + DATA->mouse_x;
    event[5] = '\040' + DATA->mouse_y;
    post_event(PG_WE_DATA, self, strlen(event), 0, event);    
  }

  /* X10 mouse reporting */
  else if (DATA->current.x10_mouse) {
    DBG("using X10-compatibility mouse reporting\n");
    if (press) {
      event[3] = '\040' + button;
      event[4] = '\040' + DATA->mouse_x;
      event[5] = '\040' + DATA->mouse_y;
      post_event(PG_WE_DATA, self, strlen(event), 0, event);
    }
  }
}

/* The End */
