/* $Id$
 *
 * pgstring.h - String data type to handle various encodings
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

#ifndef __PGSTRING_H
#define __PGSTRING_H

#include <pgserver/handle.h>       /* Needed for the "handle" data type */


/***************************************************** pgstring Data Type **/

/*
 * This set of constants defines the encodings that strings can have internal to pgserver.
 * Externally, strings are now always expressed in either ASCII or UTF-8. I don't think we'll
 * need to allow external access to all these encodings, because UTF-8 should handle all our
 * Unicode needs, and these other encodings are done for speed or metadata storage.
 */

#define PGSTR_ENCODE_ASCII       0 /* Plain old ASCII 1 byte per character encoding */
#define PGSTR_ENCODE_UTF8        1 /* UTF-8 unicode encoding (variable bytes per character) */
#define PGSTR_ENCODE_TERM16      2 /* Terminal encoding: 1 byte character plus 1 byte formatting */
#define PGSTR_ENCODE_TERM32      3 /* Terminal encoding: 3 byte character, 1 byte formatting */
#define PGSTR_ENCODE_TEXTBUFFERS 4 /* Fast insertion/deletion, and stores formatting info */

#define PGSTR_STORAGE_NOFREE  0x10000 /* Buffer is static, or managed elsewhere. Don't free it. */

#define PGSTR_ENCODE_MASK    0x0000FFFF   /* Mask of encoding bits used for actual encoding */
#define PGSTR_STORAGE_MASK   0x00FF0000   /* Mask of encoding bits used for storage */

/* Constants for seeking, same meaning as in fseek */
#define PGSEEK_SET   0   /* Seek from beginning */
#define PGSEEK_CUR   1   /* Seek from cursor    */
#define PGSEEK_END   2   /* Seek from the end   */

struct pgstring {
  u8 *buffer;
  u32 buffer_bytes;           /* Total buffer length in bytes */
  u32 num_bytes;              /* Number of bytes used in the buffer */
  u32 num_chars;              /* Number of characters used in the buffer */
  u32 flags;                  /* PGSTR_* constants (above) */
};

struct pgstr_iterator {
  s32 offset;                 /* Byte offset within a buffer */
  void *buffer;               /* Format-specific buffer pointer */
  unsigned int invalid:1;     /* Nonzero if this is outside the string */
};

#define PGCHAR_UNDEF        0xFFFFFFFF      /* Undefined character (bad encoding) */

/* One character in the string, with optional metadata. This is the 
 * common format used for exchanging data between string formats.
 */
struct pgstr_char {
  u32 ch;
  void *metadata;
};

struct pgstr_format {
  u32 (*length)(struct pgstring *str);
  struct pgstr_char (*decode)(const struct pgstring *str, struct pgstr_iterator *p);
  u32 (*encoded_length)(struct pgstr_char ch);
  void (*encode)(struct pgstring *str, struct pgstr_iterator *p, struct pgstr_char ch);
  void (*seek)(const struct pgstring *str, struct pgstr_iterator *p, s32 char_num, int whence);
  void (*del)(struct pgstring *str);
};


/******************************************************** Public Methods **/

/* Create a new empty string of the given encoding and length (in bytes, not characters)
 * If the given data is non-NULL, copy and use that as the initial contents of the string.
 * The data doesn't have to be null-terminated.
 */
g_error pgstring_new(struct pgstring **str, int encoding, int length, const u8 *data);

void pgstring_delete(struct pgstring *str);

/* Wrap an existing C string in a pgstring structure. The encoding should only be something
 * compatible with C strings, like UTF-8 or ASCII. This is primarily used to make handles to
 * pgserver's internal strings. This function already assumes use of the 
 * PGSTR_STORAGE_NOFREE flag. The resulting handle is owned by pgserver.
 */
g_error pgstring_wrap(handle *str, int encoding, const char *cstring);

/* Use a statically allocated pgstring structure to wrap the given UTF-8/ASCII string
 * in a pgstring structure temporarily. This is not re-entrant! If you need
 * a string that lasts a while, get a handle using pgstring_wrap()
 */
const struct pgstring *pgstring_tmpwrap(const char *cstring);

/* Print a string in UTF-8 encoding, return the number of characters output. */
int pgstring_print(const struct pgstring *str);

/* Create an exact duplicate of an existing pgstring */
g_error pgstring_dup(struct pgstring **dest, const struct pgstring *src);

/* Convert one pgstring to a new encoding in a new pgstring */
g_error pgstring_convert(struct pgstring **dest, int encoding, const struct pgstring *src);

/* An implementation of strcmp() for pgstrings */
int pgstring_cmp(const struct pgstring *a, const struct pgstring *b);

/* Decode a pgstring one character at a time. p is a pointer to
 * a (u8 *) that keeps track of the position in the string.
 * p should start out NULL. At the end of the string, this should return 0.
 * Improperly encoded characters return a -1.
 */
u32 pgstring_decode(const struct pgstring *str, struct pgstr_iterator *p);

/* Seek a pgstr_iterator to some relative position in the string.
 * The semantics are similar to fseek(), except that the iterator is allowed to go
 * past the edge of the string. When the iterator is past the string edge,
 * pgstring_eof should be nonzero.
 */
void pgstring_seek(const struct pgstring *str, struct pgstr_iterator *p, s32 char_num, int whence);

/* like pgstring_seek, but use units of bytes instead of characters
 */
void pgstring_seek_bytes(const struct pgstring *str, struct pgstr_iterator *p, s32 byte_num);

/* Encode/decode, with optional metadata
 */
void pgstring_encode_meta(struct pgstring *str, struct pgstr_iterator *p, u32 ch, void *metadata);
u32 pgstring_decode_meta(const struct pgstring *str, struct pgstr_iterator *p, void **metadatap);

/* Measure how many bytes a character would take to encode
 */
u32 pgstring_encoded_length(struct pgstring *str, u32 ch);

/* Copy a block of characters to another location within the string.
 * The destination string is resized to accompany this copy if necessary.
 * If num_chars is -1, copy as many characters as possible.
 */
g_error pgstring_copy(struct pgstring *dest, struct pgstring *src, struct pgstr_iterator *dest_i,
		      struct pgstr_iterator *src_i, s32 num_chars);

/* Like copy, but the source characters are deleted */
g_error pgstring_move(struct pgstring *dest, struct pgstring *src, struct pgstr_iterator *dest_i,
		      struct pgstr_iterator *src_i, s32 num_chars);

/* Insert a character before the insertion point p, resizing the string as necessary.
 * After this operation, the insertion point will point after the newly inserted character.
 */
g_error pgstring_insert_char(struct pgstring *str, struct pgstr_iterator *p, u32 ch, void *metadata);

/* Insert one string inside another string before the insertion point p, resizing as necessary */
g_error pgstring_insert_string(struct pgstring *str, struct pgstr_iterator *p, 
			       const struct pgstring *substring);

/* Delete the character pointed to by the insertion point p. The insertion point will
 * now point to the character after the one deleted, or it will be NULL if that was the
 * last character.
 */
g_error pgstring_delete_char(struct pgstring *str, struct pgstr_iterator *p);

/* Make sure that the string's buffer can hold at least 'size' bytes, resizing the buffer
 * as necessary.
 */
g_error pgstring_resize(struct pgstring *str, u32 size);

/* Comparison function for pgstring iterators. Returns:
 *  <0 if a<b
 *   0 if a==b
 *  >0 if a>b
 */
int pgstring_iteratorcmp(struct pgstring *str, struct pgstr_iterator *a,
			 struct pgstr_iterator *b);

/* Return 0 if the cursor is still inside the string,
 * If the cursor is before the beginning of the string return
 * a negative number equal to the number of characters before,
 * likewise return a positive number indicating the number
 * of characters after if the cursor is after the end of the string.
 */
int pgstring_eof(struct pgstring *str, struct pgstr_iterator *p);


#endif /* __PGSTRING_H */
/* The End */
