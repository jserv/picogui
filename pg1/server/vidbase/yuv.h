/* $Id$
 *
 * yuv.h - Some general-purpose YUV stuff
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
 *  Pascal Bauermeister <pascal.bauermeister@smartdata.ch>
 *
 */

#ifndef __VIDEO__YUV_H__
#define __VIDEO__YUV_H__

static void rgb_to_ycbcr(unsigned long r,
                         unsigned long g, unsigned long b, int *r_y,
                         int *r_cb, int *r_cr)
{
  register long y;
  register long cb;
  register long cr;
  register long r1;

  /*
   * Convert using formula on page 42 of Video Demystified.
   */
  r1 = (r * 66) + (g * 129) + (b * 25);

  if ((r1 % 256) > 128) {
    y = 1;
  }
  else {
    y = 0;
  }

  y += ((r1 >> 8) + 16);
  r1 = ((-((long) r * 38)) - ((long) g * 74) + ((long) b * 112));

  if ((r1 % 256) > 128) {
    cb = 1;
  }
  else if ((r1 % 256) < (-128)) {
    cb = -1;
  }
  else {
    cb = 0;
  }

  cb += ((r1 >> 8) + 128);
  r1 = (((long) r * 112) - ((long) g * 94) - ((long) b * 18));

  if ((r1 % 256) > 128) {
    cr = 1;
  }
  else if ((r1 % 256) < (-128)) {
    cr = -1;
  }
  else {
    cr = 0;
  }
  
  cr += (r1 >> 8) + 128;
#if 0
  /*
   * Bound check.
   */

  if (y < 0) {
    y = 0;
  }
  
  if (y > 255) {
    y = 255;
  }
  
  if (cb < 0) {
    cb = 0;
  }

  if (cb > 255) {
    cb = 255;
  }

  if (cr < 0) {
    cr = 0;
  }
  
  if (cr > 255) {
    cr = 255;
  }

  /*
   * Scale to accepted values.
   */
  if ((y % 4) > 2) {
    *r_y = (y / 4) + 1;
  }
  else {
    *r_y = y / 4;
  }
  
  if ((*r_y) > 0x3F) {
    *r_y = 0x3F;
  }
  
  if ((cb % 16) > 8) {
      *r_cb = (cb / 16) + 1;
  }
  else {
    *r_cb = cb / 16;
  }
  
  if ((*r_cb) > 0xF) {
    *r_cb = 0x0F;
  }
  
  if ((cr % 16) > 8) {
    *r_cr = (cr / 16) + 1;
  }
  else {
    *r_cr = cr / 16;
  }

  if ((*r_cr) > 0xF) {
    *r_cr = 0x0F;
  }
#endif

  *r_y = y;
  *r_cb = cb;
  *r_cr = cr;

  return;
}

#endif /* __VIDEO__YUV_H__ */
