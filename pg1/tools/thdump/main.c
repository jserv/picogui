/* $Id$
 *
 * main.c - theme dump program for debugging and testing the
 *          theme tools
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

#include "thdump.h"

void constval(unsigned long x,char *prefix);

int main(int argc, char **argv) {
  FILE *thf;
  struct pgtheme_header hdr;
  struct pgtheme_thobj  obj;
  struct pgtheme_prop   prop;
  int o,p;
  unsigned long obj_save;

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

  fread(&hdr,sizeof(hdr),1,thf)<sizeof(hdr);
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
	
  /* Objects */
  for (o=0;o<ntohs(hdr.num_thobj);o++) {

    fread(&obj,sizeof(obj),1,thf);
    printf("\n- Object (#%d) ID = [%d",o,ntohs(obj.id));
    constval(ntohs(obj.id),"PGTH_O_");
    printf("], num_prop = %d\n",ntohs(obj.num_prop));
    obj_save = ftell(thf);
    
    /* Properties */
    fseek(thf,ntohl(obj.proplist),SEEK_SET);
    for (p=0;p<ntohs(obj.num_prop);p++) {
      
      fread(&prop,sizeof(prop),1,thf);
      printf("  -  [%d",ntohs(prop.id));
      constval(ntohs(prop.id),"PGTH_P_");
      printf("] = [0x%08X %d",
	     ntohl(prop.data),ntohl(prop.data));
      constval(ntohl(prop.data),NULL);
      printf("] via [%d",ntohs(prop.loader));
      constval(ntohs(prop.loader),"PGTH_LOAD_");
      printf("]\n");
    }

    fseek(thf,obj_save,SEEK_SET);
  }
  
  return 0;
}

/* Prints any constants with the value x */
void constval(unsigned long x,char *prefix) {
  struct symnode *p = symboltab;
  while (p->name) {
    if (p->value==x && ((!prefix) || (!strncmp(p->name,prefix,strlen(prefix)))))
      printf(" %s",p->name);
    p++;
  }
}

/* The End */
