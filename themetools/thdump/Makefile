# $Id: Makefile,v 1.3 2000/10/10 00:49:05 micahjd Exp $                                                $
#
# Theme dump Makefile
#
# PicoGUI small and efficient client/server GUI
# Copyright (C) 2000 Micah Dowty <micahjd@users.sourceforge.net>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
# 
# Contributors:
#
#
#
#

######## Add new source and header files here

BINNAME = thdump

SRC := symtab.c main.c

HDR := ../include/picogui/*.h thdump.h
CONSTANTS := ../include/picogui/constants.h

CFLAGS := -I../include

######## Flags and Tools

CFLAGS := $(CFLAGS) -O3 -g -c
LDFLAGS :=

GCC := gcc
MKSYMTAB := perl mksymtab.pl

### Rules

# Some extension substitution
OBJ := $(SRC:.c=.o)

all: $(BINNAME)

clean:
	rm -f $(OBJ) $(BINNAME) symtab.c

$(BINNAME) : $(OBJ) $(HDR)
	$(GCC) -o $(BINNAME) $(OBJ) $(LDFLAGS)

%.o : %.c $(HDR)
	$(GCC) -o $@ $< $(CFLAGS)

# Auto-generated symtab

symtab.c : $(CONSTANTS) mksymtab.pl
	$(MKSYMTAB) < $(CONSTANTS) > symtab.c

### The End ###





















