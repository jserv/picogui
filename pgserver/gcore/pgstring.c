/* $Id: pgstring.c,v 1.3 2002/09/17 22:50:18 micahjd Exp $
 *
 * pgstring.c - String data type to handle various encodings
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
#include <pgserver/pgstring.h>
#include <string.h>
#include <stdio.h>

#ifdef DEBUG_PGSTRING_MEM
#define DEBUG_FILE
#endif
#include <pgserver/debug.h>

static const struct pgstr_format pgstrf_ascii;
static const struct pgstr_format pgstrf_utf8;
static const struct pgstr_format pgstrf_term16;
static const struct pgstr_format pgstrf_term32;
extern const struct pgstr_format pgstrf_textbuffers; /* In its own file */

static const struct pgstr_format *pgstr_format_table[] = {
  &pgstrf_ascii,
  &pgstrf_utf8,
  &pgstrf_term16,
  &pgstrf_term32,
#ifdef CONFIG_WIDGET_TEXTBOX
  &pgstrf_textbuffers,
#else
  NULL,
#endif
};

/* Memory management constants for pgstring insert/delete
 */
#define BUFFER_GROW_MARGIN    1024
#define BUFFER_MAX_EMPTY      2048
#define BUFFER_SHRINK_MARGIN  256


/******************************************************** Internal functions */

const struct pgstr_format *pgstr_getformat(const struct pgstring *str) {
  return pgstr_format_table[str->flags & PGSTR_ENCODE_MASK];
}

/* Update the invalid flag in the iterator */
void pgstr_boundscheck(const struct pgstring *str, struct pgstr_iterator *p) {
  p->invalid = p->offset < 0 || p->offset >= str->num_bytes;
}

/******************************************************** General string functions */

/* Create a new empty string of the given encoding and length (in bytes, not characters)
 * If the given data is non-NULL, copy and use that as the initial contents of the string.
 * The data doesn't have to be null-terminated.
 */
g_error pgstring_new(struct pgstring **str, int encoding, int length, const u8 *data) {
  g_error e;

  e = g_malloc((void**)str, sizeof(struct pgstring));
  errorcheck;

  if (length) {
    e = g_malloc((void**)&(*str)->buffer, length);
    errorcheck;
    
    if (data)
      memcpy((*str)->buffer, data, length);
    else
      memset((*str)->buffer,0,length);
  }
  else
    (*str)->buffer = NULL;

  (*str)->buffer_bytes = length;
  (*str)->num_bytes = length;
  (*str)->flags = encoding;
  (*str)->num_chars = pgstr_getformat(*str)->length(*str);
  
  return success;
}

/* Delete the buffer and string structure */
void pgstring_delete(struct pgstring *str) {
  if (!(str->flags & PGSTR_STORAGE_NOFREE)) {
    if (pgstr_getformat(str)->delete)
      pgstr_getformat(str)->delete(str);
    g_free(str->buffer);
  }
  g_free(str);
}

/* Wrap an existing C string in a pgstring structure. The encoding should only be something
 * compatible with C strings, like UTF-8 or ASCII. This is primarily used to make handles to
 * pgserver's internal strings. This function already assumes use of the 
 * PGSTR_STORAGE_NOFREE flag. The resulting handle is owned by pgserver.
 */
g_error pgstring_wrap(handle *str, int encoding, const char *cstring) {
  g_error e;
  struct pgstring *pgstr;

  e = g_malloc((void**)&pgstr, sizeof(struct pgstring));
  errorcheck;

  pgstr->buffer = (u8*) cstring;
  pgstr->buffer_bytes = strlen(cstring);
  pgstr->num_bytes = pgstr->buffer_bytes;
  pgstr->num_chars = pgstr->buffer_bytes;
  pgstr->flags = encoding | PGSTR_STORAGE_NOFREE;

  e = mkhandle(str,PG_TYPE_PGSTRING,-1,pgstr);
  errorcheck;

  return success;
}

/* Use a statically allocated pgstring structure to wrap the given ASCII string
 * in a pgstring structure temporarily. This is not re-entrant! If you need
 * a string that lasts a while, get a handle using pgstring_wrap()
 */
const struct pgstring *pgstring_tmpwrap(const char *cstring) {
  static struct pgstring str;

  str.buffer = (u8*) cstring;
  str.buffer_bytes = strlen(cstring);
  str.num_bytes = str.buffer_bytes;
  str.num_chars = str.buffer_bytes;
  str.flags = PGSTR_ENCODE_ASCII | PGSTR_STORAGE_NOFREE;

  return &str;
}

/* Print a string in UTF-8 encoding, return the number of characters output. */
int pgstring_print(const struct pgstring *str) {
  struct pgstring *tmpstr;
  struct pgstr_iterator p = PGSTR_I_NULL;
  u32 ch;

  /* 8-byte character buffer for UTF-8 encoding */
  if (iserror(pgstring_new(&tmpstr,PGSTR_ENCODE_UTF8,8,NULL)))
    return 0;

  while (ch = pgstring_decode(str,&p)) {
    struct pgstr_iterator q = PGSTR_I_NULL;
    pgstring_encode_meta(tmpstr, &q, ch, NULL);
    fwrite(tmpstr->buffer, q.offset,1,stdout);
  }
  
  pgstring_delete(tmpstr);
  return 0;
}

/* Create an exact duplicate of an existing pgstring */
g_error pgstring_dup(struct pgstring **dest, struct pgstring *src) {
  

  return success;
}

/* Convert one pgstring to a new encoding in a new pgstring */
g_error pgstring_convert(struct pgstring **dest, int encoding, struct pgstring *src) {
  return success;
}

/* An implementation of strcmp() for pgstrings */
int pgstring_cmp(const struct pgstring *a, const struct pgstring *b) {
  struct pgstr_iterator ia = PGSTR_I_NULL, ib = PGSTR_I_NULL;
  u32 ca,cb;

  do {
    ca = pgstring_decode(a,&ia);
    cb = pgstring_decode(b,&ib);
    
    if (ca<cb) return -1;
    if (ca>cb) return 1;
  } while (ca && cb);
  
  return 0;
}

/* Decode a pgstring one character at a time. p is a pointer to
 * a (u8 *) that keeps track of the position in the string.
 * p should start out NULL. At the end of the string, this should return 0.
 */
u32 pgstring_decode(const struct pgstring *str, struct pgstr_iterator *p) {
  pgstr_boundscheck(str,p);
  if (p->invalid)
    return 0;

  return pgstr_getformat(str)->decode(str,p).ch;
}

/* Seek a pgstr_iterator to some position in the string. If the position can't be
 * found, the iterator will be set to NULL
 */
void pgstring_seek(const struct pgstring *str, struct pgstr_iterator *p, s32 char_num) {
  pgstr_getformat(str)->seek(str,p,char_num);
  pgstr_boundscheck(str,p);
}

/* like pgstring_seek, but use units of bytes instead of characters
 */
void pgstring_seek_bytes(const struct pgstring *str, struct pgstr_iterator *p, s32 byte_num) {
  p->offset += byte_num;
  pgstr_boundscheck(str,p);
}

/* Encode a character to the string, with metadata
 */
void pgstring_encode_meta(struct pgstring *str, struct pgstr_iterator *p, u32 ch, void *metadata) {
  struct pgstr_char c;
  c.ch = ch;
  c.metadata = metadata;
  pgstr_boundscheck(str,p);
  pgstr_getformat(str)->encode(str,p,c);
}

/* Decode a character from the string, with metadata
 */
u32 pgstring_decode_meta(struct pgstring *str, struct pgstr_iterator *p, void **metadatap) {
  struct pgstr_char c;
  pgstr_boundscheck(str,p);
  if (p->invalid) {
    c.ch = 0;
    c.metadata = NULL;
  }
  else 
    c = pgstr_getformat(str)->decode(str,p);
  if (metadatap)
    *metadatap = c.metadata;
  return c.ch;
}

/* Measure how many bytes a character would take to encode
 */
u32 pgstring_encoded_length(struct pgstring *str, u32 ch) {
  struct pgstr_char c;
  c.ch = ch;
  return pgstr_getformat(str)->encoded_length(c);
}

/* Copy a block of characters to another location within the string.
 * This may not work as you expect on formats that use variable length encodings.
 */
void pgstring_chrcpy(struct pgstring *str,u32 dest_chr, u32  src_chr, u32 num_chars) {
  struct pgstr_iterator dest = PGSTR_I_NULL,src = PGSTR_I_NULL;

  /* FIXME: This is cheesy, need to support UTF-8 and textbox encodings.
   *        as-is this is just enough for the terminal.
   */

  pgstring_seek(str,&src,src_chr);
  pgstring_seek(str,&dest,dest_chr);
  num_chars *= pgstring_encoded_length(str,' ');
  if (dest.offset + num_chars > str->num_bytes)
    num_chars = str->num_bytes - num_chars - dest.offset;
  memmove(str->buffer + dest.offset,str->buffer + src.offset,num_chars);
}

/* Comparison function for pgstring iterators. Returns:
 *  <0 if a<b
 *   0 if a==b
 *  >0 if a>b
 */
int pgstring_iteratorcmp(struct pgstr_iterator *a, struct pgstr_iterator *b) {
  /* FIXME: Update this for textbuffers */
  if (a->offset == b->offset)
    return 0;
  else if (a->offset < b->offset)
    return -1;
  return 1;
}

/******************************************************** Insertion/deletion */

/* Insert a character before the insertion point p, resizing the string as necessary.
 * After this operation, the insertion point will point after the newly inserted character.
 */
g_error pgstring_insert_char(struct pgstring *str, struct pgstr_iterator *p, u32 ch, void *metadata) {
  u32 len = pgstring_encoded_length(str,ch);
  g_error e;

  /* Make sure we have enough space */
  e = pgstring_resize(str, str->num_bytes + len);
  errorcheck;

  /* Make room for the inserted text */
  memmove(str->buffer + p->offset + len, str->buffer + p->offset, 
	  str->num_bytes - p->offset);

  pgstring_encode_meta(str,p,ch,metadata);
 
  str->num_chars++;
  str->num_bytes += len;

  return success;
} 

/* Insert one string inside another string before the insertion point p, resizing as necessary */
g_error pgstring_insert_string(struct pgstring *str, struct pgstr_iterator *p, 
			       struct pgstring *substring) {
  g_error e;
  struct pgstr_iterator sub_i = PGSTR_I_NULL;
  u32 ch;
  void *meta;
  
  while ((ch = pgstring_decode_meta(substring, &sub_i, &meta))) {
    e = pgstring_insert_char(str,p,ch,meta);
    errorcheck;
  }

  return success;
}

/* Delete the character pointed to by the insertion point p. The insertion point will
 * now point to the character after the one deleted, or it will be NULL if that was the
 * last character.
 */
g_error pgstring_delete_char(struct pgstring *str, struct pgstr_iterator *p) {
  struct pgstr_iterator p2 = *p;
  u32 len;
  g_error e;

  /* Measure the character we're about to delete */
  pgstring_seek(str,&p2,1);
  len = p2.offset - p->offset;

  e = pgstring_resize(str, str->num_bytes - len);
  errorcheck;
  memmove(str->buffer + p->offset, str->buffer + p->offset + len, 
	  str->num_bytes - (p->offset + len));

  str->num_chars--;
  str->num_bytes -= len;
  pgstr_boundscheck(str, p);

  return success;
}

/* Make sure that the string's buffer can hold at least 'size' bytes, resizing the buffer
 * as necessary.
 */
g_error pgstring_resize(struct pgstring *str, u32 size) {
  u32 new_size;

  /* Too small? */
  if (str->buffer_bytes < size) {
    new_size = size + BUFFER_GROW_MARGIN;
  }
  /* Too big? */
  else if (str->buffer_bytes > size + BUFFER_MAX_EMPTY) {
    new_size = size + BUFFER_SHRINK_MARGIN;
  }
  /* Just right? */
  else
    return success;

  DBG("Resizing buffer from %d to %d bytes, %d bytes / %d chars used\n",
      str->buffer_bytes, new_size, str->num_bytes, str->num_chars);

  str->buffer_bytes = new_size;
  return g_realloc((void**)&str->buffer, new_size);
}

/******************************************************** ASCII string encoding */

u32 pgstr_ascii_length(struct pgstring *str) {
  return str->num_bytes;
}

struct pgstr_char pgstr_ascii_decode(const struct pgstring *str, struct pgstr_iterator *p) {
  struct pgstr_char c;
  c.ch = str->buffer[p->offset++];
  c.metadata = NULL;
   return c;
}

u32 pgstr_ascii_encoded_length(struct pgstr_char ch) {
  return 1;
}

void pgstr_ascii_encode(struct pgstring *str, struct pgstr_iterator *p, struct pgstr_char ch) {
  str->buffer[p->offset++] = ch.ch;
}

void pgstr_ascii_seek(const struct pgstring *str, struct pgstr_iterator *p, s32 char_num) {
  p->offset += char_num;
}

static const struct pgstr_format pgstrf_ascii = {
  length: pgstr_ascii_length,
  decode: pgstr_ascii_decode,
  encoded_length: pgstr_ascii_encoded_length,
  encode: pgstr_ascii_encode,
  seek: pgstr_ascii_seek,
};


/******************************************************** UTF-8 string encoding
 *
 * For a description of the UTF-8 standard, see:
 * http://www.cl.cam.ac.uk/~mgk25/unicode.html
 */

u32 pgstr_utf8_length(struct pgstring *str) {
  int i = 0;
  struct pgstr_iterator p = PGSTR_I_NULL;

  while (pgstring_decode(str,&p))
    i++;

  return i;
}

u32 pgstr_utf8_encoded_length(struct pgstr_char ch) {
  if (ch.ch <= 0x7F)
    return 1;
  if (ch.ch <= 0x7FF)
    return 2;
  if (ch.ch <= 0xFFFF)
    return 3;
  if (ch.ch <= 0x1FFFFF)
    return 4;
  if (ch.ch <= 0x3FFFFFF)
    return 5;
  return 6;
}

struct pgstr_char pgstr_utf8_decode(const struct pgstring *str, struct pgstr_iterator *p) {
  struct pgstr_char c;
  u32 ch = 0;
  u8 b;
  int length,i;
  c.metadata = NULL;

  /* The first character determines the sequence's length */
  ch = str->buffer[p->offset++];

  if (!ch) {
    /* We shouldn't have an actual null character, flag it as undefined */
    c.ch = PGCHAR_UNDEF;
    return c;
  }

  if (!(ch & 0x80)) {
    /* 1-byte code, return it as-is */
    c.ch = ch;
    return c;
  }
  else if ((ch & 0xC0) == 0x80) {
    c.ch = PGCHAR_UNDEF;
    return c;
  }
  else if (!(ch & 0x20)) {
    length = 2;
    ch &= 0x1F;
  }
  else if (!(ch & 0x10)) {
    length = 3;
    ch &= 0x0F;
  }
  else if (!(ch & 0x08)) {
    length = 4;
    ch &= 0x07;
  }
  else if (!(ch & 0x04)) {
    length = 5;
    ch &= 0x03;
  }
  else if (!(ch & 0x02)) {
    length = 6;
    ch &= 0x01;
  }
  else {
    /* Invalid code */
    c.ch = PGCHAR_UNDEF;
    return c;
  }

  /* Decode each byte of the sequence */
  for (i=1;i<length;i++) {
    /* End of string checks */
    if (p->offset >= str->num_bytes) {
      c.ch = 0;
      return c;
    }
    b = str->buffer[p->offset++];       /* Get the next byte */
    if (!b) {
      c.ch = PGCHAR_UNDEF;
      return c;
    }

    if ((b & 0xC0) != 0x80) {
      p->offset--;
      c.ch = PGCHAR_UNDEF;
      return c;
    }
    ch <<= 6;
    ch |= b & 0x3F;
  }  
  c.ch = ch;
  
  /* Make sure it is a unique representation */
  if (pgstr_utf8_encoded_length(c) != length)
    c.ch = PGCHAR_UNDEF;

  return c;
}

void pgstr_utf8_encode(struct pgstring *str, struct pgstr_iterator *p, struct pgstr_char ch) {
  int shift, length;
  u8 byte;

  length = pgstr_utf8_encoded_length(ch);

  /* One character, encode it verbatim */
  if (length==1) {
    str->buffer[p->offset++] = ch.ch;
    return;
  }

  /* Calculate the content of the first byte, given the length of our string */
  byte = 0xFF << (8 - length);
  shift = 6 * (length - 1);

  /* The first byte will be as calculated above, the rest will all have 10 (binary)
   * followed by 6 bits of data.
   */
  while (length) {
    str->buffer[p->offset++] = byte | ((ch.ch >> shift) & 0x3F); 
    shift -= 6;
    length--;
    byte = 0x80;
  }
}

void pgstr_utf8_seek(const struct pgstring *str, struct pgstr_iterator *p, s32 char_num) {
  /* FIXME: handle negative char_num */

  while (char_num) {
    if (!pgstring_decode(str,p))
      return;
    char_num--;
  }
}

static const struct pgstr_format pgstrf_utf8 = {
  length: pgstr_utf8_length,
  decode: pgstr_utf8_decode,
  encoded_length: pgstr_utf8_encoded_length,
  encode: pgstr_utf8_encode,
  seek: pgstr_utf8_seek,
};


/******************************************************** Terminal 16-bit encoding */

u32 pgstr_term16_length(struct pgstring *str) {
  return str->num_bytes >> 1;
}

struct pgstr_char pgstr_term16_decode(const struct pgstring *str, struct pgstr_iterator *p) {
  struct pgstr_char c;
  c.ch = str->buffer[p->offset++];
  c.metadata = (void*) (u32) str->buffer[p->offset++];
  return c;
}

u32 pgstr_term16_encoded_length(struct pgstr_char ch) {
  return 2;
}

void pgstr_term16_encode(struct pgstring *str, struct pgstr_iterator *p, struct pgstr_char ch) {
  str->buffer[p->offset++] = ch.ch;
  str->buffer[p->offset++] = (u8) (u32) ch.metadata;
}

void pgstr_term16_seek(const struct pgstring *str, struct pgstr_iterator *p, s32 char_num) {
  p->offset += char_num<<1;
}

static const struct pgstr_format pgstrf_term16 = {
  length: pgstr_term16_length,
  decode: pgstr_term16_decode,
  encoded_length: pgstr_term16_encoded_length,
  encode: pgstr_term16_encode,
  seek: pgstr_term16_seek,
};


/******************************************************** Terminal 32-bit encoding */

u32 pgstr_term32_length(struct pgstring *str) {
  return str->num_bytes >> 2;
}

struct pgstr_char pgstr_term32_decode(const struct pgstring *str, struct pgstr_iterator *p) {
  struct pgstr_char c;

  /* Advance in 32-bit chunks using the native byte 
   * order (different than how term16 works!) 
   */
  c.ch = *((u32*)(str->buffer + p->offset));
  p->offset += 4;

  /* Use the high 8 bits as metadata */
  c.metadata = (void*) (c.ch >> 24);
  c.ch &= 0x00FFFFFF;

  return c;
}

u32 pgstr_term32_encoded_length(struct pgstr_char ch) {
  return 4;
}

void pgstr_term32_encode(struct pgstring *str, struct pgstr_iterator *p, struct pgstr_char ch) {
  *((u32*)(str->buffer + p->offset)) = (ch.ch & 0xFFFFFF) | ((((u32)ch.metadata) & 0xFF) << 24);
  p->offset += 4;
}

void pgstr_term32_seek(const struct pgstring *str, struct pgstr_iterator *p, s32 char_num) {
  p->offset += char_num<<2;
}

static const struct pgstr_format pgstrf_term32 = {
  length: pgstr_term32_length,
  decode: pgstr_term32_decode,
  encoded_length: pgstr_term32_encoded_length,
  encode: pgstr_term32_encode,
  seek: pgstr_term32_seek,
};

/* The End */
