SUBDIRS = \
	configscript include \
	widget appmgr debug gcore net \
	vidbase theme video os vidwrap \
	input font formats \
	.

INCLUDES = -Iinclude -I$(srcdir)/include $(DRIVER_CFLAGS)

bin_PROGRAMS = pgserver

pgserver_SOURCES = pgmain.c

pgserver_LDADD = \
	@LIB_OS@		\
	gcore/libgcore.a	\
	widget/libwidget.a 	\
	appmgr/libappmgr.a	\
	net/libnet.a		\
	theme/libtheme.a	\
	debug/libdebug.a	\
	video/libvideo.a	\
	vidwrap/libvidwrap.a	\
	font/libfont.a		\
	vidbase/libvidbase.a	\
	input/libinput.a	\
	formats/libformats.a	\
	$(DRIVER_LIBS)

EXTRA_DIST = \
	script/cnvfont.1bpp.pl 		\
	script/fontdef.pl 		\
	script/grabfonts.pl 		\
	script/pnm_embed.pl		\
	config.in			\
	select.in			\
	config.defaults 		\
	macros/aclocal-include.m4 	\
	macros/pguiconfig.m4

CONFIG_STATUS_DEPENDENCIES = $(CONFIG_FILE) $(srcdir)/select.in

.PHONY: config menuconfig

config:
	sh $(top_srcdir)/configscript/Configure \
		$(top_srcdir)/config.in $(CONFIG_FILE)
	@mv -f $(CONFIG_FILE) $(CONFIG_FILE).old
	@rm -f .config.h && mv .config $(CONFIG_FILE)
	@echo
	@echo "type make to build the program"

menuconfig: configscript/lxdialog/lxdialog
	sh $(top_srcdir)/configscript/Menuconfig \
		$(top_srcdir)/config.in $(CONFIG_FILE)
	@rm -f .config.h
	@test -f .config && \
		mv $(CONFIG_FILE) $(CONFIG_FILE).old && mv .config $(CONFIG_FILE)
	@echo
	@echo "type make to build the program"

configscript/lxdialog/lxdialog:
	@(cd configscript/lxdialog && $(MAKE) $(AM_MAKEFLAGS) all)

