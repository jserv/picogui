INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include

noinst_LIBRARIES = libwidget.a

libwidget_a_SOURCES = \
    indicator.c \
    label.c \
    toolbar.c \
    scroll.c \
    widget.c \
    button.c \
    popup.c \
    box.c \
    background.c \
    menuitem.c \
    checkbox.c \
    flatbutton.c \
    listitem.c \
    radiobutton.c \
    panelbar.c \
    simplemenu.c \
    dialogbox.c \
    messagedialog.c \
    scrollbox.c \
    $(WIDGET)

EXTRA_libwidget_a_SOURCES = \
    terminal_frontend.c \
    terminal_textgrid.c \
    terminal_vt102.c \
    canvas.c \
    panel_nobar.c \
    textbox_frontend.c \
    textbox_document.c \
    textbox_paragraph.c \
    pgstr_textbuffers.c \
    field.c \
    panel.c

libwidget_a_LIBADD       = $(WIDGET:%.c=%.o)
libwidget_a_DEPENDENCIES = $(WIDGET:%.c=%.o)


