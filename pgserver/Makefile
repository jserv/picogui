# $Id: Makefile,v 1.40 2000/12/29 21:36:18 micahjd Exp $
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

BINNAME = pgserver

SRC := \
    widget/bitmap.c \
    widget/indicator.c \
    widget/label.c \
    widget/toolbar.c \
    widget/scroll.c \
    widget/widget.c \
    widget/button.c \
    widget/panel.c \
    widget/popup.c \
    widget/box.c \
    widget/field.c \
    widget/background.c \
    widget/menuitem.c \
    widget/terminal.c \
    \
    gcore/grop.c \
    gcore/g_malloc.c \
    gcore/div.c \
    gcore/font.c \
    gcore/handle.c \
    gcore/g_error.c \
    gcore/timer.c \
    gcore/errortext.c \
    gcore/video.c \
    gcore/input.c \
    gcore/driverinfo.c \
    gcore/pgmain.c \
    \
    appmgr/global.c \
    \
    net/request.c \
    net/dispatch.c \
    net/eventq.c \
    \
    video/svgagl.c \
    video/ez328_chipslice_lcd.c \
    video/sdl.c \
    video/svgafb.c \
    video/sdlfb.c \
    \
    input/sdlinput.c \
    input/svgainput.c \
    \
    vidbase/defaultvbl.c \
    vidbase/linear8.c \
    \
    theme/memtheme.c \
    theme/fillstyle.c \


HDR := include/pgserver/*.h include/picogui/*.h

CFLAGS := -Iinclude

######## Flags and Tools

CFLAGS := $(CFLAGS) -g -c
LDFLAGS :=

# These are default, can be overridden later
GCC := gcc
STRIP := strip

######## Load options from Makefile.cfg

### Config parsing

include Makefile.cfg

CFLAGS := $(CFLAGS) -Iinclude/$(PLATFORM)

# Target platform
ifeq ($(strip $(PLATFORM)),windows)
	GCC := i386-mingw32-$(GCC)
	STRIP := i386-mingw32-$(STRIP)
	BINNAME := $(BINNAME).exe
	LDFLAGS := $(LDFLAGS) -lwsock32
endif
ifeq ($(strip $(PLATFORM)),uclinux)
        GCC := m68k-pic-coff-gcc
        CFLAGS := $(CFLAGS) -fPIC -DUCLINUX
        BINNAME := uclinux-$(BINNAME)
	SRC := $(SRC) os/$(PLATFORM)/platform.c
endif

# Extra compiler flags needed by drivers

ifneq ($(findstring DRIVER_SDL,$(DRV)),)
        # Use sdl-config
	SDLCONFIG := sdl-config
	ifeq ($(strip $(PLATFORM)),windows)
		SDLCONFIG := i386-mingw32-sdl-config
	endif

        # Libraries and includes for SDL
	CFLAGS := $(CFLAGS) $(shell $(SDLCONFIG) --cflags) 
	LDFLAGS := $(LDFLAGS) $(shell $(SDLCONFIG) --libs)
endif

ifneq ($(findstring DRIVER_SVGAGL,$(DRV)),)
	LDFLAGS := $(LDFLAGS) -lvga -lvgagl
endif

ifneq ($(findstring DRIVER_SVGAFB,$(DRV)),)
        LDFLAGS := $(LDFLAGS) -lvga
endif

# Generate a list of object files
OBJ := $(SRC:.c=.o) font/fontdata.o

### Rules

all : $(BINNAME)

clean:
	rm -f $(BINNAME) $(BINNAME).coff .da *.bb *.bbg debug/* $(OBJ) core 

$(BINNAME): $(OBJ) $(HDR)
	$(GCC) -o $(BINNAME) $(OBJ) $(LDFLAGS)
ifeq ($(strip $(STRIPBIN)),yes)
	$(STRIP) $(BINNAME)
endif

# Oodles of scripts to convert the .fdf and .fi files into
# an object file
font/fontdata.o : font/*.fdf font/*.fi Makefile.cfg script/fontdef.pl
	chmod a+x script/*.pl
	script/fontdef.pl $(FONTS) > fonts.tmp.c
	$(GCC) -o font/fontdata.o fonts.tmp.c $(CFLAGS)
	rm -f fonts.tmp.c

%.o : %.c $(HDR) Makefile.cfg
	$(GCC) -o $@ $< $(CFLAGS) $(DRV)

Makefile.cfg:
	@echo "***** No Makefile.cfg found. Installing sample..."
	@echo "***** If you would like to configure PicoGUI,"
	@echo "***** edit Makefile.cfg"
	cp Makefile.cfg.sample Makefile.cfg

info:
	@echo "SRC      : [${SRC}]"
	@echo "HEADER   : [${HDR}]"
	@echo "CFLAGS   : [$(CFLAGS)]"
	@echo "DRV      : [$(DRV)]"	
	@echo "LDFLAGS  : [$(LDFLAGS)]"
	@echo "PLATFORM : [${PLATFORM}]"

### The End ###
