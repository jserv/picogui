INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include

noinst_LTLIBRARIES = libwidget.la

libwidget_la_SOURCES = \
    widget.c \
    widget_table.c \
    $(WIDGET)

EXTRA_libwidget_la_SOURCES = \
    terminal_frontend.c \
    terminal_textgrid.c \
    terminal_vt102.c \
    canvas.c \
    textbox_frontend.c \
    textbox_document.c \
    textbox_paragraph.c \
    pgstr_textbuffers.c \
    field.c \
    panel.c \
    textedit_frontend.c \
    textedit_logical.c \
    textedit_llist.c \
    managedwindow.c \
    submenuitem.c \
    indicator.c \
    label.c \
    toolbar.c \
    scroll.c \
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
    tabpage.c

libwidget_la_LIBADD       = $(WIDGET:%.c=%.lo)
libwidget_la_DEPENDENCIES = $(WIDGET:%.c=%.lo)
