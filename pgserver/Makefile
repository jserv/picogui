# $Id: Makefile,v 1.7 2000/04/24 02:38:36 micahjd Exp $
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

######## Add new source and header files for the core in this section

BINNAME = pgserver

SRC := widget/bitmap.c widget/indicator.c widget/label.c \
       widget/panel.c widget/scroll.c widget/widget.c gcore/grop.c \
       gcore/g_malloc.c gcore/div.c gcore/font.c \
       gcore/handle.c gcore/g_error.c appmgr/global.c widget/button.c

HDR := widget/widget.h gcore/divtree.h gcore/g_malloc.h \
       gcore/font.h gcore/handle.h gcore/g_error.h gcore/video.h \
       appmgr/appmgr.h gcore/input.h

CFLAGS := -Iwidget -Igcore -Iappmgr

######## Flags and Tools

CFLAGS := $(CFLAGS) -O2 -g -c
LDFLAGS := 

# These are default, can be overridden later
GCC := gcc
STRIP := strip

######## Load options from .config and add source files for modules

### Config parsing

include .config

# Target platform
ifeq ($(strip $(PLATFORM)),windows)
	GCC := i386-mingw32-$(GCC)
	STRIP := i386-mingw32-$(STRIP)
	BINNAME := $(BINNAME).exe
	LDFLAGS := $(LDFLAGS) -lwsock32
endif

# Set debugging variables
ifeq ($(strip $(DEBUG)),efence)
	# ElectricFence debugging
	LDFLAGS := $(LDFLAGS) -lefence
	CFLAGS := $(CFLAGS) -DDEBUG # These are helpful
endif
ifeq ($(strip $(DEBUG)),gprof)
	# Profiling with gprof - no debug statements during profiling!
	CFLAGS := $(CFLAGS) -pg
	LDFLAGS := $(LDFLAGS) -pg
endif
ifeq ($(strip $(DEBUG)),gcov)
	# Profiling with gcov (debug statements on, why not?)
	CFLAGS := $(CFLAGS) -DDEBUG -fprofile-arcs -ftest-coverage
endif
ifeq ($(strip $(DEBUG)),src)
	# Only debug statements
	CFLAGS := $(CFLAGS) -DDEBUG
endif

# Select fonts
ifeq ($(strip $(FONTSET)),all)
	FONTS := font/*.fi
endif
ifeq ($(strip $(FONTSET)),minimal)
	FONTS := font/helvetica.fi
endif
ifeq ($(strip $(FONTSET)),fixed)
	FONTS := font/helvetica.fi font/console.fi
endif
ifeq ($(strip $(FONTSET)),standard)
	FONTS := font/helvetica.fi font/console.fi font/times.fi font/tiny.fi
endif

# Break apart the hardware string
VIDLIB := $(word 1, $(strip $(VID)))
VIDMODE := $(word 2, $(strip $(VID)))

# Make these available for the source files
CFLAGS := $(CFLAGS) \
	  -DHWRLIB=\"$(strip $(VIDLIB))\" \
	  -DHWRMODE=\"$(strip $(VIDMODE))\" \
	  -DHWR=\"$(strip $(VIDLIB)):$(strip $(VIDMODE))\"

# Set any necessary flags peculiar to a hardware library
ifeq ($(strip $(VIDLIB)),sdl)
	# Libraries and includes for SDL
	CFLAGS := $(CFLAGS) $(shell sdl-config --cflags)
	LDFLAGS := $(LDFLAGS) $(shell sdl-config --libs)
endif

# Hardware-specific source files
SRC := $(SRC) video/$(VIDLIB)/hardware.c
CFLAGS := $(CFLAGS) -Ivideo/$(VIDLIB)/$(VIDMODE)
HDR := $(HDR) video/$(VIDLIB)/$(VIDMODE)/mode.h

# Network-specific source files
SRC := $(SRC) net/$(NET)/request.c net/$(NET)/mainloop.c net/$(NET)/eventq.c
CFLAGS := $(CFLAGS) -Inet/$(NET)
HDR := $(HDR) net/$(NET)/request.h

# Input-specific source files
SRC := $(SRC) input/$(INPUT)/input.c

OBJ := $(SRC:.c=.o) font/fontdata.o

### Rules

all : $(BINNAME)

clean:
	rm -f $(BINNAME) *.da *.bb *.bbg debug/* $(OBJ) core 

.config :
	script/config.pl

config :
	script/config.pl

$(BINNAME): $(OBJ) $(HDR)
	$(GCC) -o $(BINNAME) $(OBJ) $(LDFLAGS)
ifeq ($(strip $(STRIPBIN)),yes)
	$(STRIP) $(BINNAME)
endif

# Oodles of scripts to convert the .fdf and .fi files into
# an object file
font/fontdata.o : font/*.fdf font/*.fi .config script/fontdef.pl
	script/fontdef.pl $(FONTS) > fonts.tmp.c
	$(GCC) -o font/fontdata.o fonts.tmp.c $(CFLAGS)
	rm -f fonts.tmp.c

%.o : %.c $(HDR) .config
	$(GCC) -o $@ $< $(CFLAGS)

### The End ###






















