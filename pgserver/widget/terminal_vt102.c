/* $Id: terminal_vt102.c,v 1.5 2002/11/06 09:08:04 micahjd Exp $
 *
 * terminal.c - a character-cell-oriented display widget for terminal
 *              emulators and things.
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
 * 
 * 
 * 
 */

#include <pgserver/common.h>
#include <pgserver/terminal.h>
#define WIDGET_SUBCLASS 0
#define DATA WIDGET_DATA(terminaldata)

void term_char_escapemode(struct widget *self,u8 c);
void term_ecma48sgr(struct widget *self);
void term_csi(struct widget *self, u8 c);
void term_othercsi(struct widget *self,u8 c);
void term_decset(struct widget *self,int n,int enable);
int term_misc_code(struct widget *self,u8 c);

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

  /****** Modified key translations */

  if (mods & PGMOD_CTRL)
    pgkey -= PGKEY_a - 1;

  /****** Unmodified Key translation */

  /* Not sure that this is 100% correct, and I know it's not complete... */

  if (pgkey > 255)
    switch (pgkey) {
      
    case PGKEY_RETURN:        rtn = "\n";       break;
      
    case PGKEY_UP:            rtn = "\033[A";   break;
    case PGKEY_DOWN:          rtn = "\033[B";   break;
    case PGKEY_RIGHT:         rtn = "\033[C";   break;
    case PGKEY_LEFT:          rtn = "\033[D";   break;
    case PGKEY_HOME:          rtn = "\033[1~";  break;
    case PGKEY_INSERT:        rtn = "\033[2~";  break;
    case PGKEY_DELETE:        rtn = "\033[3~";  break;
    case PGKEY_END:           rtn = "\033[4~";  break;
    case PGKEY_PAGEUP:        rtn = "\033[5~";  break;
    case PGKEY_PAGEDOWN:      rtn = "\033[6~";  break;
      
    case PGKEY_F1:            rtn = "\033OP";  break;
    case PGKEY_F2:            rtn = "\033OQ";  break;
    case PGKEY_F3:            rtn = "\033OR";  break;
    case PGKEY_F4:            rtn = "\033OS";  break;
      
    case PGKEY_F5:            rtn = "\033[15~";break;
    case PGKEY_F6:            rtn = "\033[17~";break;
    case PGKEY_F7:            rtn = "\033[18~";break;
    case PGKEY_F8:            rtn = "\033[19~";break;
    case PGKEY_F9:            rtn = "\033[20~";break;
    case PGKEY_F10:           rtn = "\033[21~";break;
    case PGKEY_F11:           rtn = "\033[23~";break;
    case PGKEY_F12:           rtn = "\033[24~";break;
      
    default:
      return;
    }
    
  else {
    /**** Send an untranslated key */
    *chrstr = pgkey;
    rtn = chrstr;
  }

  /* After translating a key, send back the string. */
  post_event(PG_WE_DATA,self,strlen(rtn),0,rtn); 
}


/********************************************** Character output */

/* Format and/or draw one incoming character. 
 * Handle escape codes if present. */
void term_char(struct widget *self,u8 c) {
  term_setcursor(self,0);  /* Hide */
  
  /* Handling an escape code? */
  if (DATA->escapemode)
    term_char_escapemode(self,c);

  /* Is it a control character? */
  else if (c <= '\033')
    switch (c) {
    case '\033':        /* Escape */
      DATA->escapemode = 1;
      DATA->escbuf_pos = 0;
      return;
      
    case '\a':
      /* Ding! */
      drivermessage(PGDM_SOUNDFX,PG_SND_BEEP,NULL);
      return;
      
    case '\n':
      DATA->current.crsry++;
      break;

    case '\r':
      DATA->current.crsrx = 0;
      break;
      
    case '\t':
      if (DATA->current.crsrx >= DATA->bufferw) {
        DATA->current.crsrx = 8;	/* "magic" wrapping */
        DATA->current.crsry++;
      }
      else
      /* Not sure this is right, but it's consistant with observed behavior */
        DATA->current.crsrx += 8 - (DATA->current.crsrx & 7);
      break;
      
    case '\b':
      if (!DATA->current.crsrx)  /* Can't backspace past the edge */
	return;
      term_plot(self,--DATA->current.crsrx,DATA->current.crsry,' ');
      break;
    }

  /* Normal character */
  else
    {
      if (DATA->current.crsrx >= DATA->bufferw && !DATA->current.no_autowrap) {
        DATA->current.crsrx = 0;	/* "magic" wrapping */
        DATA->current.crsry++;
	if (DATA->current.crsry >= DATA->bufferh) {  /* Scroll vertically */
	  DATA->current.crsry = DATA->bufferh-1;
	  term_scroll(self,DATA->current.scroll_top,DATA->current.scroll_bottom,-1);
	}
      }
      term_plot(self,DATA->current.crsrx++,DATA->current.crsry,c);
    }
  
  /* Handle screen edges */
  if (DATA->current.crsrx < 0)
    DATA->current.crsrx = 0;
  else if (DATA->current.crsrx > DATA->bufferw) {  /* Wrap around the side */
    DATA->current.crsrx = 0;
    DATA->current.crsry++;
  }
  if (DATA->current.crsry < 0)
    DATA->current.crsry = 0;
  else if (DATA->current.crsry >= DATA->bufferh) {  /* Scroll vertically */
    DATA->current.crsry = DATA->bufferh-1;
    term_scroll(self,DATA->current.scroll_top,DATA->current.scroll_bottom,-1);
  }
  
  term_setcursor(self,1);  /* Show cursor */
}


/********************************************** Escape codes */

/* Handle an incoming character while processing an escape sequence */
void term_char_escapemode(struct widget *self,u8 c) {
  /* Yer supposed to ignore nulls in escape codes */
  if (!c)
    return;
  
  /* Too much? */
  if (DATA->escbuf_pos >= ESCAPEBUF_SIZE) {
    DATA->escapemode = 0;
#ifdef BOTHERSOME_TERMINAL
    {
      u8 *p;
      
      /* Keep this from messing up the debug terminal! */
      DATA->escapebuf[ESCAPEBUF_SIZE-1] = 0;
      p = DATA->escapebuf;
      while (*p) {
	if (*p == '\033')
	  *p = '^';
	p++;
      }
      
      printf("term: buffer overflowed before escape was recognized\nterm: buffer = \"%s\"\n",
	     DATA->escapebuf);
    }
#endif
    return;
  }

  /* Append to the escape code buffer */
  DATA->escapebuf[DATA->escbuf_pos++] = c;

  if (term_misc_code(self,c))
      DATA->escapemode = 0;

  /* Handle CSI escape codes that can all be parsed similarly */ 
  if (*DATA->escapebuf == '[' && DATA->escbuf_pos>1) 
    term_csi(self,c);
}


/* Handle codes that start with a CSI (ESC [) 
 * CSI codes start with CSI, an optional question mark, then optionally list numerical arguments
 * separated by a semicolon. A char other than semicolon or a digit identifies
 * and terminates the code
 */
void term_csi(struct widget *self, u8 c) {
  u8 *p,*endp,*num_end;
  
  /* Ignore question mark after CSI */
  if (DATA->escbuf_pos==2 && c=='?') {
    DATA->escbuf_pos--;
    return;
  }
  
  /* If there's more, come back later */
  if (isdigit(c) || c==';')
    return;
  
  /* We're ending the code */
  DATA->escapemode = 0;
  
  /* Numericalize the arguments */
  DATA->num_csiargs = 0;
  /* Unspecified params must be zero */
  memset(&DATA->csiargs,0,sizeof(int) * CSIARGS_SIZE);
  p    = DATA->escapebuf+1;                  /* Start after the CSI */
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
  
  /* If it ended with 'm', we got an ECMA-48 CSI sequence (colors) */
  if (c=='m')
    term_ecma48sgr(self);
  
  /* Other CSI sequence (mainly cursor control) */
  else
    term_othercsi(self,c);
}


/* Handle a parsed ECMA-48 CSI sequence */
void term_ecma48sgr(struct widget *self) {
  int *arg = DATA->csiargs;
  if(!DATA->num_csiargs)	/* no arg = reset */
    DATA->current.attr = DATA->attr_default;
  for (;DATA->num_csiargs;DATA->num_csiargs--,arg++)
    switch (*arg) {

      /* 0 - reset to normal */
    case 0:
      DATA->current.attr = DATA->attr_default;
      break;

      /* 1 - bold */
      /* 4 - underline (we treat it like a bold) */
    case 1:
    case 4:
      DATA->current.attr |= 0x08;
      break;

      /* 5 - blink (not really, it's like bold for backgrounds) */
    case 5:
      DATA->current.attr |= 0x80;
      break;

      /* 7 - reverse video on */
      /* 27 - reverse video off */
    case 7:
    case 27:
      DATA->current.attr = (DATA->current.attr >> 4) | (DATA->current.attr << 4);
      break;

      /* 21 or 22 - normal intensity */
      /* 24 - underline off */
    case 21:
    case 22:
    case 24:
      DATA->current.attr &= 0xF7;
      break;

      /* 25 - blink off */
    case 25:
      DATA->current.attr &= 0x7F;
      break;
      
      /* 30 through 37 - foreground colors */
    case 30: DATA->current.attr = (DATA->current.attr & 0xF8) | 0x00; break;  /* These are kinda funky, */
    case 31: DATA->current.attr = (DATA->current.attr & 0xF8) | 0x04; break;  /* because VT100 uses BGR */
    case 32: DATA->current.attr = (DATA->current.attr & 0xF8) | 0x02; break;  /* for the colors instead */
    case 33: DATA->current.attr = (DATA->current.attr & 0xF8) | 0x06; break;  /* of RGB */
    case 34: DATA->current.attr = (DATA->current.attr & 0xF8) | 0x01; break;
    case 35: DATA->current.attr = (DATA->current.attr & 0xF8) | 0x05; break;
    case 36: DATA->current.attr = (DATA->current.attr & 0xF8) | 0x03; break;
    case 37: DATA->current.attr = (DATA->current.attr & 0xF8) | 0x07; break;

      /* 38 - set default foreground, underline on */
    case 38:
      DATA->current.attr = (DATA->current.attr & 0xF0) | ((DATA->attr_default & 0x0F) | 0x08);
      break;

      /* 39 - set default foreground, underline off */
    case 39:
      DATA->current.attr = (DATA->current.attr & 0xF0) | (DATA->attr_default & 0x0F);
      break;

      /* 40 through 47 - background colors */
    case 40: DATA->current.attr = (DATA->current.attr & 0x1F) | 0x00; break;
    case 41: DATA->current.attr = (DATA->current.attr & 0x1F) | 0x40; break;
    case 42: DATA->current.attr = (DATA->current.attr & 0x1F) | 0x20; break;
    case 43: DATA->current.attr = (DATA->current.attr & 0x1F) | 0x60; break;
    case 44: DATA->current.attr = (DATA->current.attr & 0x1F) | 0x10; break;
    case 45: DATA->current.attr = (DATA->current.attr & 0x1F) | 0x50; break;
    case 46: DATA->current.attr = (DATA->current.attr & 0x1F) | 0x30; break;
    case 47: DATA->current.attr = (DATA->current.attr & 0x1F) | 0x70; break;

      /* 49 - default background */
    case 49:
      DATA->current.attr = (DATA->current.attr & 0x0F) | (DATA->attr_default & 0xF0);
      break;

    default:
#ifdef BOTHERSOME_TERMINAL
      printf("term: Unknown ECMA-48 SGR number = %d\n",*arg);
#endif
      break;
    }
}


/* Handle a parsed CSI sequence other than ECMA-48 SGR, ending
   with the specified character */
void term_othercsi(struct widget *self,u8 c) {
  int i;
  switch (c) {

    /* @ - Insert the indicated # of blank characters */
  case '@':
    for (i=0;i<DATA->csiargs[0];i++)
      term_char(self,' ');
    break;

    /* A - Move cursor up */
  case 'A':
    DATA->current.crsry -= DATA->csiargs[0];
    break;

    /* B - Move cursor down */
  case 'B':
    DATA->current.crsry += DATA->csiargs[0];
    break;

    /* C - Move cursor right */
  case 'C':
    DATA->current.crsrx += DATA->csiargs[0];
    break;

    /* D - Move cursor left */
  case 'D':
    DATA->current.crsrx -= DATA->csiargs[0];
    break;

    /* E - Move cursor down and to column 1 */
  case 'E':
    DATA->current.crsry += DATA->csiargs[0];
    DATA->current.crsrx = 0;
    break;

    /* F - Move cursor up and to column 1 */
  case 'F':
    DATA->current.crsry -= DATA->csiargs[0];
    DATA->current.crsrx = 0;
    break;

    /* G - Set column */
  case 'G':
    DATA->current.crsrx = DATA->csiargs[0] - 1;
    break;

    /* H - Set row,column */
  case 'H':
    DATA->current.crsry = DATA->csiargs[0] - 1;
    DATA->current.crsrx = DATA->csiargs[1] - 1;
    break;

    /* J - Erase */
  case 'J':
    switch (DATA->csiargs[0]) {
    case 1:
      term_clearbuf(self,0,0,DATA->current.crsry * DATA->bufferw + DATA->current.crsrx);
      break;
    case 2:
      term_clearbuf(self,0,0,DATA->bufferw * DATA->bufferh);
      break;
    default:
      term_clearbuf(self,DATA->current.crsrx,DATA->current.crsry,
		    (DATA->bufferw * DATA->bufferh) -
		    (DATA->current.crsrx + DATA->current.crsry * DATA->bufferw));
    }
    break;

    /* K - Erase Line */
  case 'K':
    switch (DATA->csiargs[0]) {
    case 1:
      term_clearbuf(self,0,DATA->current.crsry,DATA->current.crsrx);
      break;
    case 2:
      term_clearbuf(self,0,DATA->current.crsry,DATA->bufferw);
      break;
    default:
      term_clearbuf(self,DATA->current.crsrx,DATA->current.crsry,DATA->bufferw-DATA->current.crsrx);
    }
    break;

    /* L - Insert blank lines */
  case 'L':
    term_scroll(self,DATA->current.crsry,DATA->current.scroll_bottom,DATA->csiargs[0]);
    break;

    /* M - Delete lines */
  case 'M':
    term_scroll(self,DATA->current.crsry,DATA->current.scroll_bottom,-DATA->csiargs[0]);
    break;

    /* c - Identify as a VT102 terminal */
  case 'c':
    {
      static const char *response = "\e[?6c";
      post_event(PG_WE_DATA,self,strlen(response),0,(char*)response);
    }
    break;

    /* d - Set row */
  case 'd':
    DATA->current.crsry = DATA->csiargs[0] - 1;
    break;

    /* f - Set row,column */
  case 'f':
    DATA->current.crsry = DATA->csiargs[0] - 1;
    DATA->current.crsrx = DATA->csiargs[1] - 1;
    break;

    /* r - Set scrolling region */
  case 'r':
    if (DATA->num_csiargs == 2) {
      DATA->current.scroll_top = DATA->csiargs[0] - 1;
      DATA->current.scroll_bottom = DATA->csiargs[1] - 1;
    }
    else {
      DATA->current.scroll_top = 0;
      DATA->current.scroll_bottom = DATA->csiargs[0] - 1;
    }
    break;

    /* s - Save cursor position */
  case 's':
    DATA->current.savcrsry = DATA->current.crsry;
    DATA->current.savcrsrx = DATA->current.crsrx;
    break;

    /* u - Restore cursor position */
  case 'u':
    DATA->current.crsry = DATA->current.savcrsry;
    DATA->current.crsrx = DATA->current.savcrsrx;
    break;
    
    /* l and h - DECSET/DECRST sequence */
  case 'h':  /* SET */
  case 'l':  /* RST */
    term_decset(self,DATA->csiargs[0],c=='h');
    break;

  default:
#ifdef BOTHERSOME_TERMINAL
      printf("term: Unknown final character in CSI escape = %c (%d)\n",c,c);
#endif
      break;

  }
}


/* Handle various miscellaneous escape codes,
 * return 1 if a code is handled
 */
int term_misc_code(struct widget *self,u8 c) {

  /****** Single-character codes */

  if (DATA->escbuf_pos == 1)
    switch (c) { 
      /* ESC 7 - save state */
    case '7':
      memcpy(&DATA->saved,&DATA->current,sizeof(DATA->saved));
      return 1;

      /* ESC 8 - restore state */
    case '8':
      memcpy(&DATA->saved,&DATA->saved,sizeof(DATA->current));
      return 1;

      /* ESC c - reset */
    case 'c':
      /* Hmm.. what to do here? */
      return 1;

      /* ESC D - linefeed */
    case 'D':
      DATA->current.crsry++;
      return 1;

      /* ESC E - newline */
    case 'E':
      DATA->current.crsrx = 0;
      return 1;

      /* ESC H - set tabstop */
    case 'H':
      /* FIXME: Implement this */
      return 1;

      /* ESC M - reverse linefeed */
    case 'M':
      DATA->current.crsry--;
      return 1;
    }

  /****** Two-character codes */

  if (DATA->escbuf_pos == 2)
    switch (DATA->escapebuf[0]) { 
      /* ( - Set G0 character set */
    case '(':
      DATA->current.g[0] = c;
      return 1;

      /* ) - set G1 character set */
    case ')':
      DATA->current.g[1] = c;
      return 1;

      /* * - set G2 character set */
    case '*':
      DATA->current.g[2] = c;
      return 1;

      /* + - set G3 character set */
    case '+':
      DATA->current.g[3] = c;
      return 1;

    case '#':
      /* #8 - DEC screen alignment test (fill screen with E's) */
      if (c == '8') {
	int i,j;
	for (j=0;j<DATA->bufferh;j++)
	  for (i=0;i<DATA->bufferw;i++)
	    term_plot(self,i,j,'E');
	return 1;
      }
      /* The rest of the codes starting with ESC # are for 
       * double-height/width chars, not supported */
      return 1;

    }

  return 0;
}


void term_decset(struct widget *self,int n,int enable) {
  switch (n) {

    /* ESC [ ? 3 h - 80/132 column mode */
  case 3:
    /* Ignore this */
    break;
    
    /* ESC [ ? 7 h - Autowrap on/off */
  case 7:
    DATA->current.no_autowrap = !enable;
    break;

    /* ESC [ ? 25 h - Cursor on/off */
  case 25:
    if (enable)
      DATA->current.cursor_hidden = 0;
    else {
      DATA->current.cursor_hidden = 1;
      term_setcursor(self,0);
    }
    break;

  default:
#ifdef BOTHERSOME_TERMINAL
    printf("term: Unknown DECSET/DECRST number = %d\n",DATA->csiargs[0]);
#endif
    break;
  }
}

/* The End */
