CC = @CC_FOR_BUILD@

CFLAGS  = -O2
LDFLAGS =
LIBS =

noinst_PROGRAMS  = lxdialog

lxdialog_SOURCES = \
	checklist.c \
	colors.h \
	dialog.h \
	inputbox.c \
	lxdialog.c \
	menubox.c \
	msgbox.c \
	textbox.c \
	util.c \
	yesno.c

lxdialog_LDADD = -l$(CURSES)

EXTRA_DIST = BIG.FAT.WARNING
