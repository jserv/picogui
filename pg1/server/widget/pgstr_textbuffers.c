/* $Id$
 *
 * pgstr_textbuffers.c - An encoding for pgstrings that uses an array of small buffers,
 *                       providing fast insertion/deletion, Unicode support, and the
 *                       ability to store formatting metadata.
 *
 *   This encoding works by creating many small (several kilobytes) strings in
 *   UTF-8 encoding, and allowing the small buffers to grow and shrink as text is
 *   inserted and deleted. When the buffer grows or shrinks beyond allowable size
 *   limits, adjacent buffers are split or combined. The individual buffers
 *   are pgstrings themselves, in UTF-8 format. Metadata is stored using a NULL
 *   character followed by character values representing a pointer. (in UTF-8)
 *
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
#include <pgserver/pgstring.h>



/************************************************************ Public pgstr_format interface */

u32 pgstr_textbuffers_length(struct pgstring *str) {
  return 0;
}

struct pgstr_char pgstr_textbuffers_decode(const struct pgstring *str, struct pgstr_iterator *p) {
  struct pgstr_char c;
  return c;
}

u32 pgstr_textbuffers_encoded_length(struct pgstr_char ch) {
  return 0;
}

void pgstr_textbuffers_encode(struct pgstring *str, struct pgstr_iterator *p, struct pgstr_char ch) {
}

void pgstr_textbuffers_seek(const struct pgstring *str, struct pgstr_iterator *p, s32 char_num, int whence) {
}

const struct pgstr_format pgstrf_textbuffers = {
  pgstr_textbuffers_length,
  pgstr_textbuffers_decode,
  pgstr_textbuffers_encoded_length,
  pgstr_textbuffers_encode,
  pgstr_textbuffers_seek,
};

/* The End */

