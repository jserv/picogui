/* $Id: posix_commandline.c 3978 2003-05-23 10:19:38Z micah $
 *
 * posix_commandline.c - Process pgserver's command line, adding the
 *                       resulting data to the config database
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 * 
 * Contributors:
 * 
 * 
 */

#include <pgserver/common.h>
#include <pgserver/configfile.h>
#include <picogui/version.h>
#include <io.h>            /* For getopt() */
#include <string.h>

/* These headers are needed for commandline_list() */
#include <pgserver/video.h>
#include <pgserver/input.h>
#include <pgserver/appmgr.h>
#include <pgserver/font.h>
#ifdef CONFIG_FONTENGINE_BDF
#include <pgserver/font_bdf.h>
#endif

#include "getopt.c"

g_error commandline_help(void);
g_error commandline_list(void);


/******************************************************** Public functions **/

/* Parse a command line, and add it to the config file database.
 * If the command line was invalid or the user requested help, a help string
 * will be printed and an error will be returned.
 */
g_error commandline_parse(int argc, char **argv) {    
  int c;
  g_error e;

  while (1) {    
    c = getopt(argc,argv,"hlnv:m:i:t:c:-:s:");
    if (c==-1)
      break;
    
    switch (c) {
      
    case 'n':        /* Ignore config data */
      configfile_free();
      break;
      
    case '-':        /* config option */
      {
	char *section, *key, *value;
	
	/* Treat --help as an exception */
	if (!strcmp(optarg,"help"))
	  return commandline_help();
	
	if ((key = strchr(optarg,'.'))) {
	  *key = 0;
	  key++;
	  section = optarg;
	}
	else {
	  /* Default section */
	  key = optarg;
	  section = "pgserver";
	}

	if ((value = strchr(key,'='))) {
	  *value = 0;
	  value++;
	}
	else
	  /* Default value */
	  value = "1";
	
	e = set_param_str(section,key,value);
	errorcheck;
      } 
      break;
	
    case 'v':        /* Video driver */
      e = set_param_str("pgserver","video",optarg);
      errorcheck;
      break;
      
    case 'm':        /* Video mode */
      e = set_param_str("pgserver","mode",optarg);
      errorcheck;
      break;
      
    case 's':        /* Session manager */
      e = set_param_str("pgserver","session",optarg);
      errorcheck;
      break;

    case 'c':        /* Config file */
      e = configfile_parse(optarg);
      errorcheck;
      break;

    case 'l':        /* List */
      e = commandline_list();
      errorcheck;
      break;
      
    case 'i':        /* Input */
      e = append_param_str("pgserver","input"," ",optarg);
      errorcheck;
      break;

    case 't':        /* Theme */
      e = append_param_str("pgserver","themes"," ",optarg);
      errorcheck;
      break;
	
    default:        /* Need help */
      return commandline_help();
    }
  }
    
  if (optind < argc)  /* extra options */
    return commandline_help();
 
  return success;
}


/******************************************************** Internal utilities **/

g_error commandline_help(void) {
#ifdef CONFIG_TEXT
  puts("\n"
       "PicoGUI Server (http://picogui.org)\n"
       "Version " PGSERVER_VERSION_STRING
#ifdef DEBUG_ANY
       " debug"
#endif
       "\n\n"
       "usage: pgserver [-hln] [-c configfile] [-v driver] [-m WxHxD]\n"
       "                [--section.key=value] [--key=value] [--key]\n"
       "                [-i driver] [-t theme] [-s \"session manager\"]\n"
       "\n"
       "  h : This help message\n"
       "  l : List installed drivers and fonts\n"
       "  n : Ignore existing configuration data\n"
       "\n"
       "  c conf    : Load a configuration file\n"
       "  v driver  : Set the video driver (see -l)\n"
       "  m WxHxD   : Set the video mode resolution and color depth\n"
       "  i driver  : Load an input driver, can use more than one (see -l)\n"
       "  t theme   : Load a compiled theme file, can use more than one\n"
       "\n"
       "  Configuration options may be specified with section, key, and value.\n"
       "  If the section is omitted, 'pgserver' is assumed. If the value is\n"
       "  missing, '1' is used.\n"
       "\n"
       "  If specified, a session manager process will be run after server\n"
       "  initialization is done, and the server will quit after the last\n"
       "  client disconencts.\n");
#endif /* CONFIG_TEXT */
  
  return mkerror(PG_ERRT_BADPARAM, 144);   /* Badly formed command line */
}


/* Invoked by the -l option */
g_error commandline_list(void) {
#ifdef CONFIG_TEXT
  printf("\n   Video drivers:");
  {
    struct vidinfo *p = videodrivers;
    while (p->name) {
      printf(" %s",p->name);
      p++;
    }
  }

  printf("\n\n   Input drivers:");
  {
    struct inputinfo *p = inputdrivers;
    while (p->name) {
      printf(" %s",p->name);
      p++;
    }
  }
	
  printf("\n\n    Font engines:");
  {
    struct fontengine *p = fontengine_list;
    while (p->name) {
      printf(" %s",p->name);
      p++;
    }
  }

#ifdef CONFIG_FONTENGINE_BDF
  printf("\n\n       BDF fonts:");
  {
    struct bdf_fontstyle_node *p = bdf_fontstyles;
    while (p) {
      printf(" %s%d[",p->name,p->size);
      if (p->normal)
	printf("n");
      if (p->bold)
	printf("b");
      if (p->italic)
	printf("i");
      if (p->bolditalic)
	printf("I");
      if (p->flags & PG_FSTYLE_FIXED)
	printf("f");
      if (p->flags & PG_FSTYLE_DEFAULT)
	printf("d");
      printf("]");
      p = p->next;
    }
  }
#endif

  puts("\n\n         Widgets:"
#ifdef CONFIG_WIDGET_BACKGROUND
       " background"
#endif
#ifdef CONFIG_WIDGET_BOX
       " box"
#endif
#ifdef CONFIG_WIDGET_BUTTON
       " button"
#endif
#ifdef CONFIG_WIDGET_CHECKBOX
       " checkbox"
#endif
#ifdef CONFIG_WIDGET_FLATBUTTON
       " flatbutton"
#endif
#ifdef CONFIG_WIDGET_LABEL
       " label"
#endif
#ifdef CONFIG_WIDGET_LISTITEM
       " listitem"
#endif
#ifdef CONFIG_WIDGET_MENUITEM
       " menuitem"
#endif
#ifdef CONFIG_WIDGET_RADIOBUTTON
       " radiobutton"
#endif
#ifdef CONFIG_WIDGET_SUBMENUITEM
       " submenuitem"
#endif
#ifdef CONFIG_WIDGET_CANVAS
       " canvas"
#endif
#ifdef CONFIG_WIDGET_DIALOGBOX
       " dialogbox"
#endif
#ifdef CONFIG_WIDGET_MESSAGEDIALOG
       " messagedialog"
#endif
#ifdef CONFIG_WIDGET_INDICATOR
       " indicator"
#endif
#ifdef CONFIG_WIDGET_MANAGEDWINDOW
       " managedwindow"
#endif
#ifdef CONFIG_WIDGET_PANELBAR
       " panelbar"
#endif
#ifdef CONFIG_WIDGET_PANEL
       " panel"
#endif
#ifdef CONFIG_WIDGET_POPUP
       " popup"
#endif
#ifdef CONFIG_WIDGET_SCROLL
       " scroll"
#endif
#ifdef CONFIG_WIDGET_SCROLLBOX
       " scrollbox"
#endif
#ifdef CONFIG_WIDGET_SIMPLEMENU
       " simplemenu"
#endif
#ifdef CONFIG_WIDGET_TERMINAL
       " terminal"
#endif
#ifdef CONFIG_WIDGET_TEXTBOX
       " textbox"
#endif
#ifdef CONFIG_WIDGET_FIELD
       " field"
#endif
#ifdef CONFIG_WIDGET_TEXTEDIT
       " textedit"
#endif
#ifdef CONFIG_WIDGET_TOOLBAR
       " toolbar"
#endif
       );

  printf("\n    App managers:");
  {
    struct appmgr **p = appmgr_modules;
    while (*p) {
      printf(" %s",(*p)->name);
      p++;
    }
  }

  printf("\n\n  Bitmap formats:");
  {
    struct bitformat *p = bitmap_formats;
    char name[5] = {0,0,0,0,0};
    while (p->name[0]) {
      memcpy(name,p->name,4);
      printf(" %s[",name);
      if (p->detect)
	printf("d");
      if (p->load)
	printf("l");
      if (p->save)
	printf("s");
      printf("]");
      p++;
    }
  }
	   
  puts("\n");
#endif /* CONFIG_TEXT */
	
  return mkerror(PG_ERRT_BADPARAM, 144);   /* Badly formed command line */
}


/* The End */
