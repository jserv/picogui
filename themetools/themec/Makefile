# $Id: Makefile,v 1.1 2000/09/24 06:39:59 micahjd Exp $
#
# Theme compiler makefile
#
# PicoGUI small and efficient client/server GUI
# Copyright (C) 2000 Micah Dowty <micah@homesoftware.com>
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

BINNAME = themec

SRC := lex.yy.c y.tab.c
LEXSRC := pgtheme.l
YACCSRC := pgtheme.y

HDR := y.tab.h

CFLAGS := -I../include

######## Flags and Tools

CFLAGS := $(CFLAGS) -O3 -g -c
LDFLAGS := -lfl

GCC := gcc
LEX := flex
YACC := yacc -d

### Rules

# Some extension substitution
OBJ := $(SRC:.c=.o)

all: $(BINNAME)

clean:
	rm -f $(OBJ) $(BINNAME) lex.yy.c y.tab.c y.tab.h

$(BINNAME) : $(OBJ) $(HDR)
	$(GCC) -o $(BINNAME) $(OBJ) $(LDFLAGS)

%.o : %.c $(HDR)
	$(GCC) -o $@ $< $(CFLAGS)

# Lex and yacc

lex.yy.c : $(LEXSRC) 
	$(LEX) $(LEXSRC)

y.tab.c : $(YACCSRC)
	$(YACC) $(YACCSRC)

y.tab.h : y.tab.c

### The End ###





















