/* $Id: main.c,v 1.1 2000/10/07 19:03:48 micahjd Exp $
 *
 * main.c - theme dump program for debugging and testing the
 *          theme tools
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micah@homesoftware.com>
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

#include "themec.h"

int main(int argc, char **argv) {
  FILE *thf;
  struct pgtheme_header hdr;

  /* Open the file */

  thf = stdin;
  if (argc==2) {
    thf = fopen(argv[1],"r");
    if (!thf) {
      perror("Error opening file");
      return 2;
    }
  }

  /* Header */

  fread(&hdr,sizeof(hdr),1,thf);
  printf("\n- Header\n"
	 "  magic       = %c%c%c%c (%02X-%02X-%02X-%02X)\n"
         "  file_len    = %d\n"
	 "  file_sum32  = 0x%08X\n"
	 "  file_ver    = %d\n"
	 "  num_tags    = %d\n"
	 "  num_thobj   = %d\n"
	 "  num_totprop = %d\n",
	 hdr.magic[0],hdr.magic[1],hdr.magic[2],hdr.magic[3],
	 hdr.magic[0],hdr.magic[1],hdr.magic[2],hdr.magic[3],
	 ntohl(hdr.file_len),ntohl(hdr.file_sum32),
	 ntohs(hdr.file_ver),
	 ntohs(hdr.num_tags),
	 ntohs(hdr.num_thobj),
	 ntohs(hdr.num_totprop));
	 
  return 0;
}

/* The End */
