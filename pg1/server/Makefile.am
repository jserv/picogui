SUBDIRS = \
	include \
	widget appmgr debug gcore net \
	vidbase theme video os vidwrap \
	input font formats \
	profiles \
	.

DIST_SUBDIRS = configscript $(SUBDIRS)

INCLUDES = -Iinclude -I$(srcdir)/include $(DRIVER_CFLAGS)

bin_PROGRAMS = pgserver

pgserver_SOURCES = pgmain.c

pgserver_LDADD = \
	libpgserver.la	\
	$(DRIVER_LIBS)	

lib_LTLIBRARIES = libpgserver.la

libpgserver_la_SOURCES =

libpgserver_la_LIBADD = \
	net/libnet.la		\
	theme/libtheme.la	\
	debug/libdebug.la	\
	input/libinput.la	\
	video/libvideo.la	\
	gcore/libgcore.la	\
	appmgr/libappmgr.la	\
	widget/libwidget.la 	\
	vidwrap/libvidwrap.la	\
	vidbase/libvidbase.la	\
	formats/libformats.la	\
	font/libfont.la		\
	os/libos.la		\
	$(DRIVER_LIBS)		

EXTRA_DIST = \
	script/fontdef.pl 		\
	script/pnm_embed.pl		\
	script/lserrors.pl		\
	config.in			\
	select.in			\
	profile.defaults 		\
	macros/aclocal-include.m4 	\
	macros/pguiconfig.m4

CONFIG_STATUS_DEPENDENCIES = $(CONFIG_FILE) $(srcdir)/select.in

.PHONY: config menuconfig

# Note that this did read $(SHELL) instead of bash, but since Configure really does
# require the bash shell this is better.
config:
	bash $(top_srcdir)/configscript/Configure \
		$(top_srcdir)/config.in $(CONFIG_FILE)
	@mv -f $(CONFIG_FILE) $(CONFIG_FILE).old
	@rm -f .config.h && mv .config $(CONFIG_FILE)
	@echo
	@echo "type make to build the program"

menuconfig: configscript/lxdialog/lxdialog
	bash $(top_srcdir)/configscript/Menuconfig \
		$(top_srcdir)/config.in $(CONFIG_FILE)
	@rm -f .config.h
	@test -f .config && \
		mv $(CONFIG_FILE) $(CONFIG_FILE).old && mv .config $(CONFIG_FILE)
	@echo
	@echo "type make to build the program"

configscript/lxdialog/lxdialog:
	@(cd configscript && $(MAKE) $(AM_MAKEFLAGS) all)

distclean-local:
	(cd configscript && $(MAKE) $(AM_MAKEFLAGS) distclean)

