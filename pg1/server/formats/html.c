/* -*- mode: c; c-basic-offset: 2 -*-
 * $Id$
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
 *   <head>
 *   <address> *
 *   <h1> to <h6>
 *   <p>
 *   <a>
 *   <ul> *
 *   <ol> *
 *   <dl> *
 *   <pre>
 *   <div> *
 *   <center> *
 *   <blockquote> *
 *   <form> *
 *   <hr>
 *   <table> *
 *   <TT>
 *   <I>
 *   <B>
 *   <U>
 *   <STRIKE>
 *   <BIG>
 *   <SMALL>
 *   <SUB> *
 *   <SUP> *
 *   <EM>
 *   <STRONG>
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
 *   <FONT>
 *   <BASEFONT> *
 *   <BR>
 *   <MAP> *
 *   ISO Latin-1 character entities
 *   UTF-8 encoding
 *
 * ----------
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
#include <pgserver/textbox.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#if 0 /**** BROKE ****/

/*************************************** Font/color options */

/* Font constants. These should probably be incorporated into the theme
 * system at some point.
 */
#define HTML_BIG_DELTA    5                      /* Amount to change font for <big> */
#define HTML_SMALL_DELTA  -5                     /* Amount to change font for <small> */
#define LINK_COLOR        0x0000FF
const int heading_fonts[] = { 10,5,0,0,0,0 };    /* Delta font for H1 through H6 */

/*************************************** Definitions */

/* Information on the parser state */
struct html_parse {
  /* This is the textbox insertion point where text is output to.
   * Like a rendering context for the textbox.
   */
  struct textbox_cursor *c;

  /* The function to send text to during parsing.
   * Normally this is &html_dispatch_text, but it may be changed to 
   * reflect an alternate parsing mode.
   */
  g_error (*parsemode)(struct html_parse *hp, const u8 *start, const u8 *end);

  /* Keep track of the number of blank lines we've added since adding
   * text, so we can use linebreaks to separate paragraphs without
   * adding too much space.
   */
  int blank_lines;
};

/* Parameters for HTML tags */
struct html_tag_params {
  /* Raw text of the HTML parameters */
  const u8 *paramtext;
  int paramtext_len;

  /* HTML tag name */
  const u8 *tag;
  int tag_len;

  /* Pointer for traversing the tag, looking for parameters */
  const u8 *pparam;
  int pparam_len;
};

/* One tag parameter */
struct html_param {
  const u8 *name;
  int name_len;
  const u8 *value;
  int value_len;
};

/* Table to hold tag -> handler associations */

struct html_taghandler {
  const char *name;
  g_error (*handler)(struct html_parse *hp, struct html_tag_params *tag);
};

/* Parsing modes */
g_error html_parse_text(struct html_parse *hp,
			   const u8 *start,const u8 *end);
g_error html_parse_pre(struct html_parse *hp,
			  const u8 *start,const u8 *end);
g_error html_parse_ignore(struct html_parse *hp,
			     const u8 *start,const u8 *end);

/* Utilities */
g_error html_dispatch_tag(struct html_parse *hp,
			  const u8 *start,const u8 *end);
g_error html_textfragment(struct html_parse *hp,
			  const u8 *start,const u8 *end);
char html_findchar(const u8 *charname, int namelen);
int html_nextarg(struct html_tag_params *tag, struct html_param *par);
pgcolor html_findcolor(const u8 *colorname, int namelen);

/*************************************** Tables */

/* This table is used to convert character names like &nbsp; to character
 * numbers, as defined by the HTML standard.
 *
 * NB: These tables are searched using bsearch, and must therefore be
 * sorted by name.
 */

struct html_charname {
  const u8 *name;
  char ch;
} html_chartable[] = {
  { "AElig",	198 },	/* capital AE diphthong (ligature) */
  { "Aacute",	193 },	/* capital A, acute accent */
  { "Acirc",	194 },	/* capital A, circumflex accent */
  { "Agrave",	192 },	/* capital A, grave accent */
  { "Aring",	197 },	/* capital A, ring */
  { "Atilde",	195 },	/* capital A, tilde */
  { "Auml",	196 },	/* capital A, dieresis or umlaut mark */
  { "Ccedil",	199 },	/* capital C, cedilla */
  { "ETH",	208 },	/* capital Eth, Icelandic */
  { "Eacute",	201 },	/* capital E, acute accent */
  { "Ecirc",	202 },	/* capital E, circumflex accent */
  { "Egrave",	200 },	/* capital E, grave accent */
  { "Euml",	203 },	/* capital E, dieresis or umlaut mark */
  { "Iacute",	205 },	/* capital I, acute accent */
  { "Icirc",	206 },	/* capital I, circumflex accent */
  { "Igrave",	204 },	/* capital I, grave accent */
  { "Iuml",	207 },	/* capital I, dieresis or umlaut mark */
  { "Ntilde",	209 },	/* capital N, tilde */
  { "Oacute",	211 },	/* capital O, acute accent */
  { "Ocirc",	212 },	/* capital O, circumflex accent */
  { "Ograve",	210 },	/* capital O, grave accent */
  { "Oslash",	216 },	/* capital O, slash */
  { "Otilde",	213 },	/* capital O, tilde */
  { "Ouml",	214 },	/* capital O, dieresis or umlaut mark */
  { "THORN",	222 },	/* capital THORN, Icelandic */
  { "Uacute",	218 },	/* capital U, acute accent */
  { "Ucirc",	219 },	/* capital U, circumflex accent */
  { "Ugrave",	217 },	/* capital U, grave accent */
  { "Uuml",	220 },	/* capital U, dieresis or umlaut mark */
  { "Yacute",	221 },	/* capital Y, acute accent */
  { "aacute",	225 },	/* small a, acute accent */
  { "acirc",	226 },	/* small a, circumflex accent */
  { "acute",	180 },	/* acute accent */
  { "aelig",	230 },	/* small ae diphthong (ligature) */
  { "agrave",	224 },	/* small a, grave accent */
  { "amp",      '&' },  /* Ampersand */
  { "aring",	229 },	/* small a, ring */
  { "atilde",	227 },	/* small a, tilde */
  { "auml",	228 },	/* small a, dieresis or umlaut mark */
  { "brvbar",	166 },	/* broken (vertical) bar */
  { "ccedil",	231 },	/* small c, cedilla */
  { "cedil",	184 },	/* cedilla */
  { "cent",	162 },	/* cent sign */
  { "copy",	169 },	/* copyright sign */
  { "curren",	164 },	/* general currency sign */
  { "deg",	176 },	/* degree sign */
  { "divide",	247 },	/* divide sign */
  { "eacute",	233 },	/* small e, acute accent */
  { "ecirc",	234 },	/* small e, circumflex accent */
  { "egrave",	232 },	/* small e, grave accent */
  { "eth",	240 },	/* small eth, Icelandic */
  { "euml",	235 },	/* small e, dieresis or umlaut mark */
  { "frac12",	189 },	/* fraction one-half */
  { "frac14",	188 },	/* fraction one-quarter */
  { "frac34",	190 },	/* fraction three-quarters */
  { "gt",       '>' },  /* greater-than */
  { "iacute",	237 },	/* small i, acute accent */
  { "icirc",	238 },	/* small i, circumflex accent */
  { "iexcl",	161 },	/* inverted exclamation mark */
  { "igrave",	236 },	/* small i, grave accent */
  { "iquest",	191 },	/* inverted question mark */
  { "iuml",	239 },	/* small i, dieresis or umlaut mark */
  { "laquo",	171 },	/* angle quotation mark, left */
  { "lt",       '<' },  /* less-than */
  { "macr",	175 },	/* macron */
  { "micro",	181 },	/* micro sign */
  { "middot",	183 },	/* middle dot */
  { "nbsp",	' ' },	/* no-break space */
  { "not",	172 },	/* not sign */
  { "ntilde",	241 },	/* small n, tilde */
  { "oacute",	243 },	/* small o, acute accent */
  { "ocirc",	244 },	/* small o, circumflex accent */
  { "ograve",	242 },	/* small o, grave accent */
  { "ordf",	170 },	/* ordinal indicator, feminine */
  { "ordm",	186 },	/* ordinal indicator, masculine */
  { "oslash",	248 },	/* small o, slash */
  { "otilde",	245 },	/* small o, tilde */
  { "ouml",	246 },	/* small o, dieresis or umlaut mark */
  { "para",	182 },	/* pilcrow (paragraph sign) */
  { "plusmn",	177 },	/* plus-or-minus sign */
  { "pound",	163 },	/* pound sterling sign */
  { "quot",     '"' },  /* Quotation mark */
  { "raquo",	187 },	/* angle quotation mark, right */
  { "reg",	174 },	/* registered sign */
  { "sect",	167 },	/* section sign */
  { "shy",	173 },	/* soft hyphen */
  { "sup1",	185 },	/* superscript one */
  { "sup2",	178 },	/* superscript two */
  { "sup3",	179 },	/* superscript three */
  { "szlig",	223 },	/* small sharp s, German (sz ligature) */
  { "thorn",	254 },	/* small thorn, Icelandic */
  { "times",	215 },	/* multiply sign */
  { "uacute",	250 },	/* small u, acute accent */
  { "ucirc",	251 },	/* small u, circumflex accent */
  { "ugrave",	249 },	/* small u, grave accent */
  { "uml",	168 },	/* umlaut (dieresis) */
  { "uuml",	252 },	/* small u, dieresis or umlaut mark */
  { "yacute",	253 },	/* small y, acute accent */
  { "yen",	165 },	/* yen sign */
  { "yuml",	255 },	/* small y, dieresis or umlaut mark */
};

int nchar = sizeof(html_chartable) / sizeof(*html_chartable);

/*
 * Another table to convert HTML color names to RGB colors
 */

struct html_colorname {
  const u8 *name;
  pgcolor c;
} html_colortable[] = {
  { "Aqua",     0x00FFFF },
  { "Black",    0x000000 },
  { "Blue",     0x0000FF },  
  { "Fuchsia",  0xFF00FF },
  { "Gray",     0x808080 },
  { "Green",    0x008000 },
  { "Lime",     0x00FF00 },
  { "Maroon",   0x800000 },
  { "Navy",     0x000080 },
  { "Olive",    0x808000 },
  { "Purple",   0x800080 },
  { "Red",      0xFF0000 },  
  { "Silver",   0xC0C0C0 }, 
  { "Teal",     0x008080 },
  { "White",    0xFFFFFF },
  { "Yellow",   0xFFFF00 },
};   
   
int ncolor = sizeof(html_colortable) / sizeof(*html_colortable);

/*
 * Functions passed to bsearch for various tables
 */

int keylen = -1;

int html_charcmp(const void *p0, const void *p1)
{
  return strncmp(((struct html_charname *)p0)->name,
		 ((struct html_charname *)p1)->name,
		 keylen);
}


int html_colorcmp(const void *p0, const void *p1)
{
  return strncmp(((struct html_colorname *)p0)->name,
		 ((struct html_colorname *)p1)->name,
		 keylen);
}

int html_tagcmp(const void *p0, const void *p1)
{
  return strncasecmp(((struct html_taghandler *)p0)->name,
		     ((struct html_taghandler *)p1)->name,
		     keylen);
}

/*************************************** HTML tag handlers */

/* Start a new block of text (paragraph)
 * Insert linebreaks until this line and the one preceeding it are blank
 */
g_error html_tag_p(struct html_parse *hp, struct html_tag_params *tag) {
  g_error e;
  for (;hp->blank_lines < 2;hp->blank_lines++) {
    e = text_insert_linebreak(hp->c);
    errorcheck;
  }
  return success;
}

/* Line break */
g_error html_tag_br(struct html_parse *hp, struct html_tag_params *tag) {
  hp->blank_lines++;
  return text_insert_linebreak(hp->c);
}

/* Pop the most recent font change off the stack */
g_error html_tag_unformat(struct html_parse *hp, struct html_tag_params *tag) {
  text_unformat_top(hp->c);
  return success;
}

/* New paragraph, big font */
g_error html_tag_h(struct html_parse *hp, struct html_tag_params *tag) {
  g_error e;
  e = html_tag_p(hp,tag);
  errorcheck;
  return text_format_modifyfont(hp->c,PG_FSTYLE_BOLD,0,heading_fonts[tag->tag[1]-'1']);
}

/* Turn off big font, new paragraph */
g_error html_tag_end_h(struct html_parse *hp, struct html_tag_params *tag) {
  text_unformat_top(hp->c);
  return html_tag_p(hp,tag);
}

/* Simple font flags */
g_error html_tag_b(struct html_parse *hp, struct html_tag_params *tag) {
  return text_format_modifyfont(hp->c,PG_FSTYLE_BOLD,0,0);
}
g_error html_tag_i(struct html_parse *hp, struct html_tag_params *tag) {
  return text_format_modifyfont(hp->c,PG_FSTYLE_ITALIC,0,0);
}
g_error html_tag_u(struct html_parse *hp, struct html_tag_params *tag) {
  return text_format_modifyfont(hp->c,PG_FSTYLE_UNDERLINE,0,0);
}
g_error html_tag_tt(struct html_parse *hp, struct html_tag_params *tag) {
  return text_format_modifyfont(hp->c,PG_FSTYLE_FIXED,0,0);
}
g_error html_tag_strike(struct html_parse *hp, struct html_tag_params *tag) {
  return text_format_modifyfont(hp->c,PG_FSTYLE_STRIKEOUT,0,0);
}
g_error html_tag_big(struct html_parse *hp, struct html_tag_params *tag) {
  return text_format_modifyfont(hp->c,0,0,HTML_BIG_DELTA);
}
g_error html_tag_small(struct html_parse *hp, struct html_tag_params *tag) {
  return text_format_modifyfont(hp->c,0,0,HTML_SMALL_DELTA);
}

/* Preformatted text */
g_error html_tag_pre(struct html_parse *hp, struct html_tag_params *tag) {
  g_error e;

  /* Preformatted text mode */
  hp->parsemode = &html_parse_pre;

  /* New paragraph */
  e = html_tag_p(hp,tag);
  errorcheck;

  /* Teletype mode */
  return html_tag_tt(hp,tag);
}
g_error html_tag_end_pre(struct html_parse *hp, struct html_tag_params *tag) {
  g_error e;

  /* Normal text mode */
  hp->parsemode = &html_parse_text;

  /* Teletype mode off */
  e = html_tag_unformat(hp,tag);
  errorcheck;

  /* New paragraph */
  return html_tag_p(hp,tag);
}

/* Ignore the headers */
g_error html_tag_head(struct html_parse *hp, struct html_tag_params *tag) {
  hp->parsemode = &html_parse_ignore;
  return success;
}
g_error html_tag_end_head(struct html_parse *hp, struct html_tag_params *tag) {
  hp->parsemode = &html_parse_text;
  return success;
}

g_error html_tag_font(struct html_parse *hp, struct html_tag_params *tag) {
  struct html_param p;
  pgcolor newcolor;
  int setcolor = 0;
  int size = 0;
  
  /* Process each tag parameter */
  while (html_nextarg(tag,&p)) {
    
    if (p.name_len==5 && !strncasecmp(p.name,"color",p.name_len)) {
      setcolor = 1;
      newcolor = html_findcolor(p.value,p.value_len);
      if (*p.value=='#') {
	/* Was it a hex color? */
	setcolor = 1;
	newcolor = strtoul(p.value+1,NULL,16);
      }
    }

    /* FIXME: more <font> parameters */
  }

  /* Set the new font */
  text_format_modifyfont(hp->c,0,0,size);
  if (setcolor)
    hp->c->f_top->color = VID(color_pgtohwr)(newcolor);

  return success;  
}

/* Construct a horizontal rule divnode, and insert it on a line by itself */
g_error html_tag_hr(struct html_parse *hp, struct html_tag_params *tag) {
  g_error e;
  struct divnode *div;
  struct gropctxt gc;

  /* Simple horizontal line divnode.
   * FIXME: Make this more configurable via themes, process the
   *        parameters to <hr>
   *
   * This creates a new divnode with a preferred height of 3 pixels,
   * and draws a line through the middle pixel of that
   */
  e = newdiv(&div,hp->c->widget);
  errorcheck;
  div->ph = 3;
  gropctxt_init(&gc,div);
  addgropsz(&gc,PG_GROP_SLAB,0,1,0x7FFF,1);
  
  /* This resets our blankness counter, but
   * text_insert_line_div() counts as a blank line 
   */
  hp->blank_lines = 1;

  return text_insert_line_div(hp->c,div);
}

/* FIXME: Right now these look like links but aren't really links
 */
g_error html_tag_a(struct html_parse *hp, struct html_tag_params *tag) {
  g_error e;

  e = text_format_modifyfont(hp->c,PG_FSTYLE_UNDERLINE,0,0);
  errorcheck;
  hp->c->f_top->color = VID(color_pgtohwr)(LINK_COLOR);
  return success;
}
g_error html_tag_end_a(struct html_parse *hp, struct html_tag_params *tag) {
  g_error e;
  
  e = html_tag_unformat(hp,tag);
  errorcheck;
  return success;
}

/*************************************** HTML tag table */

struct html_taghandler html_tagtable[] = {
  
  { "/a",      &html_tag_end_a },
  { "/b",      &html_tag_unformat },
  { "/big",    &html_tag_unformat },
  { "/em",     &html_tag_unformat },
  { "/font",   &html_tag_unformat },
  { "/h1",     &html_tag_end_h },
  { "/h2",     &html_tag_end_h },
  { "/h3",     &html_tag_end_h },
  { "/h4",     &html_tag_end_h },
  { "/h5",     &html_tag_end_h },
  { "/h6",     &html_tag_end_h },
  { "/head",   &html_tag_end_head },
  { "/i",      &html_tag_unformat },
  { "/pre",    &html_tag_end_pre },
  { "/small",  &html_tag_unformat },
  { "/strike", &html_tag_unformat },
  { "/strong", &html_tag_unformat },
  { "/tt",     &html_tag_unformat },
  { "/u",      &html_tag_unformat },
  { "a",       &html_tag_a },
  { "b",       &html_tag_b },
  { "big",     &html_tag_big },
  { "br",      &html_tag_br },
  { "em",      &html_tag_i },
  { "font",    &html_tag_font },
  { "h1",      &html_tag_h },
  { "h2",      &html_tag_h },
  { "h3",      &html_tag_h },
  { "h4",      &html_tag_h },
  { "h5",      &html_tag_h },
  { "h6",      &html_tag_h },
  { "head",    &html_tag_head },
  { "hr",      &html_tag_hr },
  { "i",       &html_tag_i },
  { "p",       &html_tag_p },
  { "pre",     &html_tag_pre },
  { "small",   &html_tag_small },
  { "strike",  &html_tag_strike },
  { "strong",  &html_tag_b },
  { "tt",      &html_tag_tt },
  { "u",       &html_tag_u },
};

int ntags = sizeof(html_tagtable) / sizeof(*html_tagtable);

/*************************************** Parsing engine*/

/* This function performs the highest level of HTML parsing- separating
 * HTML tags and normal text.
 */
g_error html_load(struct textbox_cursor *c, const u8 *data, u32 datalen) {
  const u8 *text_start = NULL;
  const u8 *tag_start = NULL;
  int in_tag_quote = 0;        /* In a quoted string within a tag? */
  struct html_parse hp;
  g_error e;

  /* Fill in parse structure */
  memset(&hp,0,sizeof(hp));
  hp.c = c;
  hp.parsemode = &html_parse_text;

  while (*data && datalen) {
    
    /* Are we in an HTML tag now? */
    if (tag_start) {
      if (*data == '"')
	in_tag_quote = !in_tag_quote;

      /* Ending the tag? */
      if ((!in_tag_quote) && *data=='>') {
	e = html_dispatch_tag(&hp,tag_start,data-1);
	errorcheck;
	tag_start = NULL;

	/* If the next character is a newline, we're supposed to skip it */
	if (datalen>1 && data[1]=='\n') {
	  data++;
	  datalen--;
	}
      }
    }
    else {
      /* Normal text */

      /* Beginning an HTML tag? */
      if (*data == '<') {

 	/* Ending text? */
	if (text_start) {
	  
	  /* We need to ignore newlines occurring immediately before tags.
	   * If the last character of this string was \n, chop it off.
	   */
	  if (data[-1]=='\n')
	    e = (*hp.parsemode)(&hp,text_start,data-2);
	  else
	    e = (*hp.parsemode)(&hp,text_start,data-1);
	  errorcheck;
	  text_start = NULL;
	}

	tag_start = data+1;
      }
      else if (!text_start)
	text_start = data;
    }

    data++;
    datalen--;
  }

  /* Trailing text string */
  if (text_start) {
    e = (*hp.parsemode)(&hp,text_start,data-1);
    errorcheck;
  }

  return success;
}

/* Find the handler, prepare parameters, and call it */
g_error html_dispatch_tag(struct html_parse *hp,
			  const u8 *start, const u8 *end) {
  struct html_taghandler *p;
  struct html_taghandler key;
  struct html_tag_params tag_params;
  const u8 *tag;
  int tag_len;

  /* Skip leading whitespace */
  while (start<=end && isspace(*start))
    start++;
  
  /* Measure the length of the HTML tag */
  tag = start;
  tag_len = 0;
  while (start<=end && !isspace(*start)) {
    start++;
    tag_len++;
  }

  key.name = tag;
  keylen = tag_len;

  /* Find a handler for the tag */
  if (((p = bsearch(&key,
		   (void *) html_tagtable,
		    ntags,
		    sizeof(struct html_taghandler),
		    html_tagcmp)) == NULL) ||
      (strlen(p->name) != tag_len)) {

    /* No handler, ignore tag */
#ifdef DEBUG_HTML
    write(1,"html: ignoring unhandled tag <",30);
    write(1,tag,tag_len);
    write(1,">\n",2);
#endif

    return success;
  }

  /* Got a handler */

  /* Skip leading whitespace */
  while (start<=end && isspace(*start))
    start++;

  /* Fill in the html_tag_params structure */
  memset(&tag_params,0,sizeof(tag_params));
  tag_params.paramtext = start;
  tag_params.paramtext_len = end-start+1;
  tag_params.tag = tag;
  tag_params.tag_len = tag_len;

  /* Call handler */
  return (*p->handler)(hp,&tag_params);
}

/* Chop up text into fragments, sending wordbreaks
 * between them when necessary */
g_error html_parse_text(struct html_parse *hp,
			const u8 *start, const u8 *end) {
  const u8 *fragment;
  g_error e;

  for (fragment = NULL;start <= end;start++) {
    if (fragment) {
      /* Look for the end */

      if (isspace(*start)) {
	e = html_textfragment(hp,fragment,start-1);
	errorcheck;
	fragment = NULL;

	e = text_insert_wordbreak(hp->c);
	errorcheck;
      }
    }
    else {
      /* Look for the beginning */

      if (isspace(*start)) {
	e = text_insert_wordbreak(hp->c);
	errorcheck;
      }
      else
	fragment = start;
    }
  }
  if (fragment) {
    e = html_textfragment(hp,fragment,start-1);
    errorcheck;
  }
  return success;
}

/* Dispatch preformatted text. Each line is translated into one textfragment-
 * never send wordbreaks, send a line break for '\n'
 */
g_error html_parse_pre(struct html_parse *hp,
		       const u8 *start, const u8 *end) {
  const u8 *fragment;
  g_error e;

  for (fragment = NULL;start <= end;start++) {
    if (fragment) {
      /* Look for the end */

      if (*start == '\n') {
	e = html_textfragment(hp,fragment,start-1);
	errorcheck;
	fragment = NULL;

	hp->blank_lines++;
	e = text_insert_linebreak(hp->c);
	errorcheck;
      }
    }
    else {
      /* Look for the beginning */

      if (*start=='\n') {
	hp->blank_lines++;
	e = text_insert_linebreak(hp->c);
	errorcheck;
      }
      else
	fragment = start;
    }
  }
  if (fragment) {
    e = html_textfragment(hp,fragment,start-1);
    errorcheck;
  }
  return success;
}

/* Ignore text */
g_error html_parse_ignore(struct html_parse *hp,
			  const u8 *start, const u8 *end) {
  return success;
}

/* Convert characters like &nbsp; to characters, store and send the string
 * to the textbox.
 */
g_error html_textfragment(struct html_parse *hp,
			  const u8 *start,const u8 *end) {
  int length;
  const u8 *p,*cname;
  char *str, *q;
  g_error e;

  hp->blank_lines = 0;

  /* Count the number of characters in the string, treating &foo; sequences
   * as 1 character.
   */
  for (p=start,length=0; p<=end; p++, length++)
    if (*p == '&')
      for (;p<=end && *p!=';';p++);
  
  /* Transcribe it into a new string, converting &foo; using html_findchar() 
   */
  if (!length) 
    return success;
  e = g_malloc((void**)&str, length+1);
  errorcheck;
  str[length] = 0;
  p = start;
  q = str;
  while (p<=end && length) {
    if (*p == '&') {                       /* Character name */     
      cname = p+1;
      for (;p<=end && *p!=';';p++);
      *q = html_findchar(cname,p-cname);
    }
    else                                   /* Normal character */      
      *q = *p;

    p++;
    q++;
    length--;
  }
  
  /* Send it to the textbox */
  return text_insert_string(hp->c,str,0);
}

char html_findchar(const u8 *charname, int namelen) {
  struct html_charname *p;
  struct html_charname key;

  /* If it's a numeric code, return it */

  if (charname[0]=='#')
    return atoi(charname+1);

  key.name = charname;
  keylen = namelen;

  if ((p = (struct html_charname *)
       bsearch(&key,
	       (void *) html_chartable,
	       nchar,
	       sizeof(struct html_charname),
	       html_charcmp)) == NULL) {
    return '?';
  }

  return p->ch;
}

/* Find the next tag argument. If there is a tag parameter remaining,
 * returns one and puts the relevant info in 'par'. otherwise, returns zero.
 */
int html_nextarg(struct html_tag_params *tag, struct html_param *par) {
  memset(par,0,sizeof(struct html_param));

  /* First time? */
  if (!tag->pparam) {
    tag->pparam = tag->paramtext;
    tag->pparam_len = tag->paramtext_len;
  }

  /* Skip leading whitespace */
  while (tag->pparam_len && isspace(*tag->pparam)) {
      tag->pparam++;
      tag->pparam_len--;
  }

  /* Anything left? */
  if (!tag->pparam_len)
    return 0;

  /* We're at the beginning of the tag name now */
  par->name = tag->pparam;
 
  /* Keep going until we get to a whitespace (end of tag, has no param)
   * or to an '=' sign
   */
  while (tag->pparam_len) {
    if (*tag->pparam == '=')
      break;
    if (isspace(*tag->pparam)) {
      /* Done with this tag, it has no value */
      par->value = NULL;
      par->value_len = 0;
      return 1;
    }
    par->name_len++;
    tag->pparam++;
    tag->pparam_len--;
  }

  /* Tag has a parameter. Skip the '=', see if it's quoted */
  tag->pparam++;
  tag->pparam_len--;
  if (*tag->pparam == '"' && tag->pparam_len>1) {

    /* Quoted parameter, it extends until the next '"' character */
    tag->pparam++;
    tag->pparam_len--;
    par->value = tag->pparam;
    while (tag->pparam_len && *tag->pparam!='"') {
      tag->pparam++;
      tag->pparam_len--;
      par->value_len++;
    }

    /* Skip over the closing quote */
    tag->pparam++;
    tag->pparam_len--;
  }
  else {

    /* Unquoted parameter, extends until a whitespace */    
    par->value = tag->pparam;
    while (tag->pparam_len && !isspace(*tag->pparam)) {
      tag->pparam++;
      tag->pparam_len--;
      par->value_len++;
    }
  }
  return 1;
}

pgcolor html_findcolor(const u8 *colorname, int namelen) {
  struct html_colorname *p;
  struct html_colorname key;
  
  /* If it's a numeric code, return it */
  if (colorname[0]=='#')
    return strtoul(colorname+1,NULL,16);
  
  key.name = colorname;
  keylen = namelen;

  /* Search our table of color names */
  if ((p = (struct html_colorname *)
       bsearch(&key,
	       (void *) html_colortable,
	       ncolor,
	       sizeof(struct html_colorname),
	       html_colorcmp)) == NULL) {
    /* Not found */
    return 0;
  }

  return p->c;
}

#endif

/* The End */
