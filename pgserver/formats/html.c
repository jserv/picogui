/* $Id: html.c,v 1.1 2001/10/14 09:21:59 micahjd Exp $
 *
 * html.c - Use the textbox_document inferface to load HTML markup
 *
 * ---------- Notes:
 * This is not a complete HTML implementation- though this may form the core
 * rendering facilities for a web browser, it is _not_ a standalone web
 * browser. What it can do is use a subset of HTML to represent text
 * formatting in a textbox widget.
 *
 * Currently this parser tries to be a correct subset of the HTML 3.2 spec at:
 *    http://www.w3.org/TR/REC-html32
 *
 * Unknown tags are ignored. The following tags should be supported
 * by this implementation. Tags with an asterisk are not yet implemented:
 *
 *   <body> *
 *   <address> *
 *   <p> *
 *   <ul> *
 *   <ol> *
 *   <dl> *
 *   <pre> *
 *   <div> *
 *   <center> *
 *   <blockquote> *
 *   <form> *
 *   <hr> *
 *   <table> *
 *   <TT> *
 *   <I> *
 *   <B> *
 *   <U> *
 *   <STRIKE> *
 *   <BIG> *
 *   <SMALL> *
 *   <SUB> *
 *   <SUP> *
 *   <EM> *
 *   <STRONG> *
 *   <DFN> *
 *   <CODE> *
 *   <SAMP> *
 *   <KBD> *
 *   <VAR> *
 *   <CITE> *
 *   <INPUT> *
 *   <SELECT> *
 *   <TEXTAREA> *
 *   <A> *
 *   <IMG> *
 *   <FONT> *
 *   <BASEFONT> *
 *   <BR> *
 *   <MAP> *
 *   &nbsp; *
 *   (other ISO Latin-1 character entities) *
 *   UTF-8 encoding *
 *
 * ----------
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000,2001 Micah Dowty <micahjd@users.sourceforge.net>
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
#include <pgserver/textbox.h>

/* Information on the parser state */
struct html_parse {
  struct textbox_cursor *c;
};

void html_dispatch_tag(struct html_parse *hp,const u8 *start,const u8 *end);
void html_dispatch_string(struct html_parse *hp,const u8 *start,const u8 *end);

/************************* Interface */

/* This function performs the highest level of HTML parsing- separating
 * HTML tags and normal text.
 */
g_error html_load(struct textbox_cursor *c, const u8 *data, u32 datalen) {
  const u8 *text_start = NULL;
  const u8 *tag_start = NULL;
  int in_tag_quote = 0;        /* In a quoted string within a tag? */
  struct html_parse hp;

  /* Fill in parse structure */
  memset(&hp,0,sizeof(hp));
  hp.c = c;

  while (*data && datalen) {
    
    /* Are we in an HTML tag now? */
    if (tag_start) {
      if (*data == '"')
	in_tag_quote = !in_tag_quote;

      /* Ending the tag? */
      if ((!in_tag_quote) && *data=='>') {
	html_dispatch_tag(&hp,tag_start,data);
	tag_start = NULL;
      }
    }
    else {
      /* Normal text */

      /* Beginning an HTML tag? */
      if (*data == '<') {

	/* Ending text? */
	if (text_start) {
	  html_dispatch_text(&hp,text_start,data-1);
	  text_start = NULL;
	}

	tag_start = data;
      }
      else if (!text_start)
	text_start = data;
    }

    data++;
    datalen--;
  }

  /* Trailing text string */
  if (text_start)
    html_dispatch_text(&hp,text_start,data-1);

  return sucess;
}

/************************* Utilities */

void html_dispatch_tag(struct html_parse *hp,
		       const u8 *start, const u8 *end) {
  write(1,"HTML tag: ",10);
  write(1,start,end-start+1);
  write(1,"\n",1);
}

void html_dispatch_text(struct html_parse *hp,
			const u8 *start, const u8 *end) {
  write(1,"HTML text: ",11);
  write(1,start,end-start+1);
  write(1,"\n",1);
}

/* The End */



