/*
   This file is part of PocketBee.
  
   PocketBee is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.
  
   PocketBee is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
  
   You should have received a copy of the GNU General Public License
   along with PocketBee; see the file COPYING.  If not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   $Id: gen-pgserver-conf.c,v 1.1 2001/11/06 08:12:43 bauermeister Exp $

   $Log: gen-pgserver-conf.c,v $
   Revision 1.1  2001/11/06 08:12:43  bauermeister
   Added etc directory

*/

#include <stdio.h>
#include <pgserver/autoconf.h>

/*****************************************************************************/

void gen_inputdrivers(void)
{
  int nb =
#define DRV(name, dummy) +1
#include <inputdrivers.inc>
    ;

  if(nb) {
    printf("input =");
#undef DRV
#define DRV(name, dummy) printf(" %s", name);
#include <inputdrivers.inc>
    printf("\n");
  }
  return;
}

/*****************************************************************************/

int main(int argc, char** argv)
{
  int i;

  for(i=1; i<argc; ++i) {
    if(argv[i][0]=='-') {
      switch(argv[i][1]) {
      case 'i':
	gen_inputdrivers();
	exit(0);
      default:
      }
    }
  }

  fprintf(stderr, "Usage: %s [OPTION]\n", argv[0]);
  fprintf(stderr, "Outputs one line of pgserver config file.\n\n");
  fprintf(stderr, "  -i\tprints 'input = <list of input drivers>'\n");
  exit(1);
}

/*****************************************************************************/

/*
   Local Variables:
   c-file-style: "smartdata"
   End:
*/
