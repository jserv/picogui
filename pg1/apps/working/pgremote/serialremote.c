/* $Id$
 * 
 * serialremote.c - remote input driver for PicoGUI (mouse only)
 *                  relays events directly from a microsoft-compatible
 *                  serial mouse to the pgserver. Serial port can be specified
 *                  on the command line.
 *
 * THIS DOES NOT WORK YET!
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micahjd@users.sourceforge.net>
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

#include <picogui.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>

int main(int argc, char **argv) {
   int fd;
   char data[3];
   int buttons,dx,dy;
   
   fd = open((argc>1) ? argv[0] : "/dev/ttyS0",O_RDONLY);
   if (fd<=0) {
      perror("Opening mouse");
      return -1;
   }
   
   while (read(fd,data,sizeof(data))>0) {
      /* Decode mouse data (from gpm) */
      buttons= ((data[0] & 0x20) >> 3) | ((data[0] & 0x10) >> 4);
      dx=      (signed char)(((data[0] & 0x03) << 6) | (data[1] & 0x3F));
      dy=      (signed char)(((data[0] & 0x0C) << 4) | (data[2] & 0x3F));
       
 
      printf("%d %d %d\n",buttons,dx,dy);
   }
      
   return 0;
}
   
/* The End */
