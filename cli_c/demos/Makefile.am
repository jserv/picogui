# $Id: Makefile.am,v 1.10 2001/09/18 21:52:10 micahjd Exp $

noinst_PROGRAMS = \
	abstest 		\
	canvastst 		\
	demo 			\
	fieldtest 		\
	imgview 		\
	menutest 		\
	omnibar 		\
	omnibar-helio 		\
	pgfx_canvas_immediate 	\
	pgfx_canvas_persistent 	\
	themedemo 		\
	vidpopup                \
	offscreen               \
	scrsaver	        \
	dialogdemo              \
	animbutton              \
	galaxy                  \
	bouncyball              \
	flyingtux               \
	textdemo

noinst_SCRIPTS = Demo.app

EXTRA_DIST = \
	data/dustpuppy.pnm	\
	data/dustpuppy_mask.pnm	\
	data/tux.pnm		\
	data/tux_mask.pnm 

LDADD    = ../src/libpgui.la
INCLUDES = -I$(top_srcdir)/include

abstest_SOURCES   = abstest.c
canvastst_SOURCES = canvastst.c
demo_SOURCES      = demo.c
fieldtest_SOURCES = fieldtest.c
imgview_SOURCES   = imgview.c
menutest_SOURCES  = menutest.c
omnibar_SOURCES   = omnibar.c
omnibar_helio_SOURCES = omnibar-helio.c
pgfx_canvas_immediate_SOURCES  = pgfx_canvas_immediate.c
pgfx_canvas_persistent_SOURCES = pgfx_canvas_persistent.c
themedemo_SOURCES = themedemo.c
vidpopup_SOURCES  = vidpopup.c
dialogdemo_SOURCES = dialogdemo.c

Demo.app:
	ln -fs fieldtest "Field Test.app"
	ln -fs demo "Demo.app"
	ln -fs menutest "Menu Test.app"
	ln -fs dialogdemo "Dialog Demo.app"
	ln -fs canvastst "Canvas Test.app"	
	ln -fs vidpopup "Set Video Mode.app"
	ln -fs ../../apps/pterm/pterm "Terminal (pterm).app"
	ln -fs ../../apps/blackout/blackout "Blackout.app"
	ln -fs ../../apps/battleship/battleship "Battleship.app"

clean-local: 
	find -type l -exec rm {} \;
