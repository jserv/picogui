SUBDIRS = \
	include \
	widget appmgr debug gcore net \
	vidbase theme video os vidwrap \
	input font formats etc \
	profiles \
	.

DIST_SUBDIRS = configscript $(SUBDIRS)

INCLUDES = -Iinclude -I$(srcdir)/include $(DRIVER_CFLAGS)

bin_PROGRAMS = pgserver

pgserver_SOURCES = pgmain.c

pgserver_LDADD = \
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
	@LIB_OS@		\
	$(DRIVER_LIBS)		\
	@RM_LIBS@


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

config:
	$(SHELL) $(top_srcdir)/configscript/Configure \
		$(top_srcdir)/config.in $(CONFIG_FILE)
	@mv -f $(CONFIG_FILE) $(CONFIG_FILE).old
	@rm -f .config.h && mv .config $(CONFIG_FILE)
	@echo
	@echo "type make to build the program"

menuconfig: configscript/lxdialog/lxdialog
	$(SHELL) $(top_srcdir)/configscript/Menuconfig \
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


if ENABLE_BEE

dist-hook: @PACKAGE@.spec
	cp -a $(srcdir)/PicoGUIServer.mdl $(distdir)
	cp -a @PACKAGE@.spec $(distdir)/@PACKAGE@.spec

@RPM_MAKE_FRAG@

else

dist-hook:
	@echo "nothing to add"

endif
