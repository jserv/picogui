INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include

noinst_LIBRARIES = libwidget.a

libwidget_a_SOURCES = \
    bitmap.c \
    indicator.c \
    label.c \
    toolbar.c \
    scroll.c \
    widget.c \
    menubar.c \
    button.c \
    popup.c \
    box.c \
    field.c \
    background.c \
    menuitem.c \
    checkbox.c \
    flatbutton.c \
    listitem.c \
    radiobutton.c \
    textbox_main.c \
    textbox_document.c \
    list.c \
    $(WIDGET)

EXTRA_libwidget_a_SOURCES = \
    terminal.c \
    canvas.c \
    panel_nobar.c \
    panel.c

libwidget_a_LIBADD       = $(WIDGET:%.c=%.o)
libwidget_a_DEPENDENCIES = $(WIDGET:%.c=%.o)


