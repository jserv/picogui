/* $Id$
 *
 * widget_table.c - Table defining all the installed widgets
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
 *    
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 * Contributors:
 * 
 * 
 * 
 */

#include <pgserver/common.h>
#include <pgserver/widget.h>


/* The macros here are just shortcuts for defining the list
 * of implementations for each widget, and the subclass number.
 * STATICWIDGET_TABLE assumes there is no trigger handler,
 * and HYBRIDWIDGET_TABLE is used to override the initialization
 * of an existing widget without fully subclassing it.
 */

/* Table of widgets */
struct widgetdef widgettab[] = {
   
#ifdef CONFIG_WIDGET_TOOLBAR
DEF_STATICWIDGET_TABLE(0,toolbar)
#else
DEF_ERRORWIDGET_TABLE(mkerror(PG_ERRT_BADPARAM,121))
#endif

#ifdef CONFIG_WIDGET_LABEL
DEF_HYBRIDWIDGET_TABLE(label,button)
#else
DEF_ERRORWIDGET_TABLE(mkerror(PG_ERRT_BADPARAM,122))
#endif

#ifdef CONFIG_WIDGET_SCROLL
DEF_WIDGET_TABLE(0,scroll)
#else
DEF_ERRORWIDGET_TABLE(mkerror(PG_ERRT_BADPARAM,123))
#endif

#ifdef CONFIG_WIDGET_INDICATOR
DEF_STATICWIDGET_TABLE(0,indicator)
#else
DEF_ERRORWIDGET_TABLE(mkerror(PG_ERRT_BADPARAM,124))
#endif

#ifdef CONFIG_WIDGET_MANAGEDWINDOW
DEF_WIDGET_TABLE(0,managedwindow)
#else
DEF_ERRORWIDGET_TABLE(mkerror(PG_ERRT_BADPARAM,120))
#endif

#ifdef CONFIG_WIDGET_BUTTON
DEF_WIDGET_TABLE(0,button)
#else
DEF_ERRORWIDGET_TABLE(mkerror(PG_ERRT_BADPARAM,125))
#endif

#ifdef CONFIG_WIDGET_PANEL
DEF_STATICWIDGET_TABLE(0,panel)
#else
DEF_ERRORWIDGET_TABLE(mkerror(PG_ERRT_BADPARAM,94))
#endif

#ifdef CONFIG_WIDGET_POPUP
DEF_WIDGET_TABLE(0,popup)
#else
DEF_ERRORWIDGET_TABLE(mkerror(PG_ERRT_BADPARAM,126))
#endif

#ifdef CONFIG_WIDGET_BOX
DEF_STATICWIDGET_TABLE(0,box)
#else
DEF_ERRORWIDGET_TABLE(mkerror(PG_ERRT_BADPARAM,127))
#endif

#ifdef CONFIG_WIDGET_FIELD
DEF_HYBRIDWIDGET_TABLE(field,textbox)
#else
DEF_ERRORWIDGET_TABLE(mkerror(PG_ERRT_BADPARAM,128))
#endif

#ifdef CONFIG_WIDGET_BACKGROUND
DEF_WIDGET_TABLE(0,background)
#else
DEF_ERRORWIDGET_TABLE(mkerror(PG_ERRT_BADPARAM,129))
#endif

#ifdef CONFIG_WIDGET_MENUITEM
DEF_HYBRIDWIDGET_TABLE(menuitem,button)
#else
DEF_ERRORWIDGET_TABLE(mkerror(PG_ERRT_BADPARAM,130))
#endif

#ifdef CONFIG_WIDGET_TERMINAL
DEF_WIDGET_TABLE(0,terminal)
#else
DEF_ERRORWIDGET_TABLE(mkerror(PG_ERRT_BADPARAM,102))
#endif

#ifdef CONFIG_WIDGET_CANVAS
DEF_WIDGET_TABLE(0,canvas)
#else
DEF_ERRORWIDGET_TABLE(mkerror(PG_ERRT_BADPARAM,103))
#endif

#ifdef CONFIG_WIDGET_CHECKBOX
DEF_HYBRIDWIDGET_TABLE(checkbox,button)
#else
DEF_ERRORWIDGET_TABLE(mkerror(PG_ERRT_BADPARAM,131))
#endif

#ifdef CONFIG_WIDGET_FLATBUTTON
DEF_HYBRIDWIDGET_TABLE(flatbutton,button)
#else
DEF_ERRORWIDGET_TABLE(mkerror(PG_ERRT_BADPARAM,132))
#endif

#ifdef CONFIG_WIDGET_LISTITEM
DEF_HYBRIDWIDGET_TABLE(listitem,button)
#else
DEF_ERRORWIDGET_TABLE(mkerror(PG_ERRT_BADPARAM,133))
#endif

#ifdef CONFIG_WIDGET_SUBMENUITEM
DEF_HYBRIDWIDGET_TABLE(submenuitem,button)
#else
DEF_ERRORWIDGET_TABLE(mkerror(PG_ERRT_BADPARAM,134))
#endif

#ifdef CONFIG_WIDGET_RADIOBUTTON
DEF_HYBRIDWIDGET_TABLE(radiobutton,button)
#else
DEF_ERRORWIDGET_TABLE(mkerror(PG_ERRT_BADPARAM,135))
#endif

#ifdef CONFIG_WIDGET_TEXTBOX
DEF_WIDGET_TABLE(0,textbox)
#else
DEF_ERRORWIDGET_TABLE(mkerror(PG_ERRT_BADPARAM,98))
#endif
				     
#ifdef CONFIG_WIDGET_PANELBAR
DEF_WIDGET_TABLE(0,panelbar)
#else
DEF_ERRORWIDGET_TABLE(mkerror(PG_ERRT_BADPARAM,137))
#endif

/* Subclasses popup */
#ifdef CONFIG_WIDGET_SIMPLEMENU
DEF_WIDGET_TABLE(1,simplemenu)
#else
DEF_ERRORWIDGET_TABLE(mkerror(PG_ERRT_BADPARAM,138))
#endif

/* Subclasses popup */
#ifdef CONFIG_WIDGET_DIALOGBOX
DEF_WIDGET_TABLE(1,dialogbox)
#else
DEF_ERRORWIDGET_TABLE(mkerror(PG_ERRT_BADPARAM,139))
#endif

/* Subclasses dialogbox, popup */
#ifdef CONFIG_WIDGET_MESSAGEDIALOG
DEF_STATICWIDGET_TABLE(2,messagedialog)
#else
DEF_ERRORWIDGET_TABLE(mkerror(PG_ERRT_BADPARAM,140))
#endif

#ifdef CONFIG_WIDGET_SCROLLBOX
DEF_WIDGET_TABLE(0,scrollbox)
#else
DEF_ERRORWIDGET_TABLE(mkerror(PG_ERRT_BADPARAM,136))
#endif

#ifdef CONFIG_WIDGET_TEXTEDIT
DEF_WIDGET_TABLE(0,textedit)
#else
DEF_ERRORWIDGET_TABLE(mkerror(PG_ERRT_BADPARAM,108))
#endif

#ifdef CONFIG_WIDGET_TABPAGE
  /* Custom table entry, since so far this is the only widget
   * to define the post_attach method.
   */
  {
    /*subclass_num:  */1,
         /*install:  */tabpage_install,
          /*remove:  */tabpage_remove,
         /*trigger:  */NULL,
             /*set:  */tabpage_set,
             /*get:  */tabpage_get,
          /*resize:  */tabpage_resize,
     /*post_attach:  */tabpage_post_attach
  },
#else
DEF_ERRORWIDGET_TABLE(mkerror(PG_ERRT_BADPARAM,147))
#endif

};

/* The End */








