## Process this file with automake to produce Makefile.in

EXTRA_DIST = pixmaps/* makefile.msc makeinc.msc \
	fe-text/fe-text.c fe-text/myglib.h \
	fe-text/README fe-text/fe-text.h fe-text/glib.c

if DO_PICOGUI
picogui_fe = fe-picogui
endif

if DO_TEXT
text_fe = fe-text
endif

if DO_GTK
gtk_fe = fe-gtk
endif

SUBDIRS = common $(gtk_fe) $(text_fe) $(picogui_fe)
