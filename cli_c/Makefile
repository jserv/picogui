# $Id: Makefile,v 1.2 2000/09/09 07:14:46 micahjd Exp $
#
# C client library (libpgui) makefile
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

LIBNAME = libpgui

# The source files to link into the library
SRC := 

# Demo program files (each .c is a self-contained demo proggie)
DEMOSRC := 

HDR := include/picogui/*.h

CFLAGS := -Iinclude

######## Flags and Tools

DEMOCFLAGS := $(CFLAGS) -O3
CFLAGS := $(CFLAGS) -O3 -g -c
LDFLAGS := -rpath /usr/local/lib

LIBTOOL := libtool
GCC := gcc

### Rules

# Some extension substitution
OBJ := $(SRC:.c=.o)
LOBJ := $(SRC:.c=.lo)
DEMOBIN := $(DEMOSRC:.c=)

instructions:
	@echo "Try one of the following to compile libpgui:"
	@echo ""
	@echo "	make clean     : Get rid of object files and binaries"
	@echo "	make library   : Build static and shared lib into the .libs directory"
	@echo "	make install   : * Not implemented yet *"
	@echo "	make uninstall : * Not implemented yet *"
	@echo "	make demos     : Compile the demo programs"
	@echo ""

clean:
	rm -f $(OBJ) $(LOBJ) $(DEMOBIN) *.la
	rm -Rf .libs src/.libs

library : $(LIBNAME).la

$(LIBNAME).la : $(LOBJ) $(HDR)
	$(LIBTOOL) $(GCC) $(LDFLAGS) -o $(LIBNAME).la $(LOBJ)

# Library object files
%.lo : %.c $(HDR)
	$(LIBTOOL) $(GCC) -o $@ $(CFLAGS) $<

demos : $(DEMOBIN)

# Demo object files (each one to a seperate binary)
% : %.c $(HDR) $(LIBNAME).la
	$(GCC) -o $@ $(DEMOCFLAGS) $< .libs/$(LIBNAME).a

### The End ###





















