# $Id: Makefile,v 1.20 2001/03/30 08:29:05 micahjd Exp $
#
# C client library (libpgui) makefile
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

LIBNAME = libpgui

# Library installation path
RPATH := /usr/lib

# Header installation path
HPATH := /usr/include

# The source files to link into the library
SRC := src/netcore.c src/api.c

# Demo program files (each .c is a self-contained demo proggie)
DEMOSRC := demos/demo.c demos/themedemo.c demos/msgdialog.c \
           demos/imgview.c demos/menutest.c demos/omnibar.c \
           demos/fieldtest.c demos/calc.c demos/canvastst.c \
	   demos/abstest.c demos/vidpopup.c

HDR := include/*.h include/picogui/*.h

CFLAGS := -Iinclude

#LDFLAGS := -lefence

######## Flags and Tools

DEMOCFLAGS := $(CFLAGS) -O3 -g
CFLAGS := $(CFLAGS) -O3 -g -c
LDFLAGS := $(LDFLAGS) -rpath $(RPATH)

LIBTOOL := libtool
GCC := gcc

######## Rules

# Some extension substitution
OBJ := $(SRC:.c=.o)
LOBJ := $(SRC:.c=.lo)
DEMOBIN := $(DEMOSRC:.c=)

instructions:
	@echo "Try one of the following to compile libpgui:"
	@echo ""
	@echo "	make clean     : Get rid of object files and binaries"
	@echo "	make library   : Build static and shared lib into the .libs directory"
	@echo "	make install   : Install libraries and headers"
	@echo "	make uninstall : Uninstall libraries and headers"
	@echo "	make demos     : Compile the demo programs"
	@echo ""

clean:
	rm -f $(OBJ) $(LOBJ) $(DEMOBIN) *.la
	rm -Rf .libs src/.libs

library : $(LIBNAME).la

install : library
	cp -Rf -d include/* $(HPATH)
	$(LIBTOOL) --mode=install install -c $(LIBNAME).la $(RPATH)/$(LIBNAME).la

uninstall :
	rm -Rf $(HPATH)/picogui.h $(HPATH)/picogui
	$(LIBTOOL) --mode=uninstall rm $(RPATH)/$(LIBNAME).la

$(LIBNAME).la : $(LOBJ) $(HDR)
	$(LIBTOOL) $(GCC) $(LDFLAGS) -o $(LIBNAME).la $(LOBJ)

# Library object files
%.lo : %.c $(HDR)
	$(LIBTOOL) $(GCC) -o $@ $(CFLAGS) $<

# Build demos and symlink some .app thingies
demos : $(DEMOBIN)
	ln -fs fieldtest "demos/Field Test.app"
	ln -fs demo "demos/Demo.app"
	ln -fs msgdialog "demos/Message Dialog Demo.app"
	ln -fs menutest "demos/Menu Test.app"
	ln -fs calc "demos/Calculator.app"
	ln -fs canvastst "demos/Canvas Test.app"	
	ln -fs vidpopup "demos/Set Video Mode.app"
	ln -fs ../../apps/pterm/pterm "demos/Terminal (pterm).app"
	ln -fs ../../apps/blackout/blackout "demos/Blackout.app"
	ln -fs ../../apps/battleship/battleship "demos/Battleship.app"

# Demo object files (each one to a seperate binary)
% : %.c $(HDR) $(LIBNAME).la
	$(GCC) -o $@ $(DEMOCFLAGS) $< .libs/$(LIBNAME).a

### The End ###





















