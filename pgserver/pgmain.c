/* $Id: pgmain.c,v 1.41 2002/10/12 14:46:34 micahjd Exp $
 *
 * pgmain.c - Processes command line, initializes and shuts down
 *            subsystems, and invokes the net subsystem for the
 *            main loop.
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2002 Micah Dowty <micahjd@users.sourceforge.net>
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
#include <pgserver/appmgr.h>
#include <pgserver/handle.h>
#include <pgserver/video.h>
#include <pgserver/input.h>
#include <pgserver/widget.h>
#include <pgserver/configfile.h>
#include <pgserver/timer.h>
#include <pgserver/hotspot.h>

#ifdef CONFIG_FONTENGINE_BDF
#include <pgserver/font_bdf.h>
#endif

#include <stdlib.h>
#include <string.h>   /* For strdup() */

#ifdef WINDOWS
#include <process.h>
#else
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
extern char **environ;
#endif

int mainloop_proceed = 1;
int in_init = 1;
int use_sessionmgmt = 0;           /* Using session manager, exit after last client */
int use_tpcal = 0;                 /* Run tpcal before running the session manager */
int sessionmgr_secondary = 0;      /* Need to run session manager after tpcal */
int sessionmgr_start = 0;          /* Start the session manager at the next iteration */
int server_returnval = 0;          /* This is the return value pgserver will exit with */

extern s32 memref;
struct dtstack *dts;

#ifdef UCLINUX
extern char *optarg;
extern int optind;
#endif /* UCLINUX */

/* List of themes managed by pgserver */
struct themefilenode *themefiles;

/* Fork off a process specified in a config variable, 
 * returns nonzero if a process was specified.
 */
int run_config_process(const char *name) {
#ifndef WINDOWS  
  const char *cmd;
  int my_pid = getpid();

  cmd = get_param_str("pgserver",name,NULL);

  if (!cmd)
    return 0;

# ifdef UCLINUX
  if (!vfork())
# else
    if (!fork())
# endif
      {
	char *sargv[4];
	sargv[0] = "sh";
	sargv[1] = "-c";
	sargv[2] = (char *) cmd;
	sargv[3] = 0;
	execve("/bin/sh",sargv,environ);
	prerror(mkerror(PG_ERRT_BADPARAM,55));
	kill(my_pid,SIGTERM);
	exit(1);
      }
   
  return 1;
#endif
}

void commandline_help(void) {
#ifndef CONFIG_TEXT
  puts("Commandline error");
#else
  puts("\n"
       "PicoGUI server (http://picogui.org)\n"
       "\n"
#ifdef DEBUG_ANY
       "DEBUG MODE ON\n"
       "\n"
#endif
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
#endif
  exit(1);
}

/********** And it all starts here... **********/
int main(int argc, char **argv) {
#ifdef WINDOWS
  /* Fake it */
  int optind = 1;
#endif
#ifdef CONFIG_VIDEOTEST
  int videotest_mode,videotest_on = 0;
#endif
   
  /* Initialize pointer tables here if it can't be done at compile-time */
#ifdef RUNTIME_FUNCPTR
  widgettab_init();
  drivertab_init();
  rqhtab_init();
#endif
   
  /*************************************** Command-line processing */

#ifdef DEBUG_INIT
  printf("Init: signal handler\n");
#endif
   
  /* Get signals.c to init signal handlers */
  signals_install();
   
#ifdef DEBUG_INIT
  printf("Init: processing command line\n");
#endif

  /* Read in global and user-specific config files */
  {
    const char filename[]="/.pgserverrc";
    char *s,*home;
    size_t len;

    configfile_parse("/etc/pgserver.conf");

    home = getenv("HOME");
    len=strlen(home);
    if (home && !iserror(prerror(g_malloc((void**)&s,len+sizeof(filename))))) {
      memcpy(s,home,len);
      memcpy(s+len,filename,sizeof(filename));
      configfile_parse(s);
      g_free(s);
    }
  }   
  
  {  /* Restrict the scope of these vars so they go away after
	initialization is done with them */
    
    int c,fd;
#ifndef WINDOWS
    struct stat st;
#endif
    unsigned char *themebuf;
    struct themefilenode *tail = NULL,*p;
    int vidw,vidh,vidd,vidf;
    const char *str;
    g_error (*viddriver)(struct vidlib *v) = NULL;
    
#ifndef WINDOWS    /* Command line processing is broke in windoze */

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
	    commandline_help();

	  if ((key = strchr(optarg,'.'))) {
	    *key = 0;
	    key++;
	    section = optarg;
	  }
	  else {
	    key = optarg;
	    section = "pgserver";
	  }

	  if ((value = strchr(key,'='))) {
	    *value = 0;
	    value++;
	  }
	  else
	    value = "1";
	  
	  set_param_str(section,key,value);
	}
	 
	break;
	
      case 'v':        /* Video driver */
	set_param_str("pgserver","video",optarg);
	break;

      case 'm':        /* Video mode */
	set_param_str("pgserver","mode",optarg);
	break;

      case 's':        /* Session manager */
	set_param_str("pgserver","session",optarg);
	break;

      case 'c':        /* Config file */
	if (iserror(prerror(configfile_parse(optarg))))
	  return 1;
	break;

#ifdef CONFIG_TEXT
      case 'l':        /* List */

	printf("\n   Video drivers:");
	{
	  struct vidinfo *p = videodrivers;
	  while (p->name) {
	    printf(" %s",p->name);
	    p++;
	  }
	}

	printf("\n   Input drivers:");
	{
	  struct inputinfo *p = inputdrivers;
	  while (p->name) {
	    printf(" %s",p->name);
	    p++;
	  }
	}
	
	printf("\n    Font engines:");
	{
	  struct fontengine *p = fontengine_list;
	  while (p->name) {
	    printf(" %s",p->name);
	    p++;
	  }
	}

#ifdef CONFIG_FONTENGINE_BDF
	printf("\n       BDF fonts:");
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

	puts("\nOptional widgets:"
#ifdef CONFIG_WIDGET_TERMINAL
	     " terminal"
#endif
#ifdef CONFIG_WIDGET_CANVAS
	     " canvas"
#endif
#ifdef CONFIG_WIDGET_TEXTBOX
	     " textbox"
#endif
#ifdef CONFIG_WIDGET_TEXTEDIT
	     " tetedit"
#endif
	     );

	printf("  Bitmap formats:");
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
	exit(1);
#endif

      case 'i':        /* Input */
	if (iserror(prerror(
			    load_inlib(find_inputdriver(optarg),NULL)
			    ))) exit(1);
	break;

      case 't':        /* Theme */
	/* Themes have to be loaded later in the initialization process,
	 * so for now just store the filenames.
	 */
	if (iserror(prerror(g_malloc((void**)&p,
				     sizeof(struct themefilenode))))) return 1;
	p->name = optarg;  /* Optarg points inside argv so it
			    * will stick around for a while */
	p->next = NULL;
	p->h = 0;
	if (tail)
	  tail->next = p;
	else
	  themefiles = tail = p;
	tail = p;
	break;
	
      default:        /* Need help */
	commandline_help();
      }
    }
    
    if (optind < argc)  /* extra options */
      commandline_help();
    
#endif /* WINDOWS */


#ifdef DEBUG_INIT
    printf("Init: loading video drivers\n");
#endif     

    /* Load alternate messages into the error table */
    if (iserror(prerror(errorload(get_param_str("pgserver",
						"messagefile",
						NULL)))))
      return 1;
    
#ifdef CONFIG_VIDEOTEST
    /* Process test mode config options */

    if ((str = get_param_str("pgserver","videotest",NULL))) {
      videotest_on = 1;
      videotest_mode = atoi(str);
      if (!videotest_mode) {
	videotest_help();
	exit(1);
      }
    }

    if (get_param_int("pgserver","benchmark",0)) {
      videotest_on = 2;
    }
#endif

    /* Transcribe the list of themes from config option to linked list.
     * This makes it easier to load themes from the command line also,
     * and this is necessary so handles to the loaded themes can be stored
     * for reloading later 
     *
     * For some reason that wierd GNU manpage for strtok() says I shouldn't
     * use it, but in this case there's no reason why not to.
     */
    {
      const char *constthemes;
      char *themes;
      char *tok;

      if ((constthemes = get_param_str("pgserver","themes",NULL))) {
	themes = strdup(constthemes);

	while ((tok = strtok(themes," \t"))) {

	  if (iserror(prerror(g_malloc((void**)&p,
				       sizeof(struct themefilenode)))))
	    return 1;
	  p->h = 0;
	  p->name = tok;
	  p->next = NULL;
	  if (tail)
	    tail->next = p;
	  else
	    themefiles = tail = p;
	  tail = p;
	   
	  themes = NULL;
	}
      }
    } 

    /* Use strtok again to load input drivers */
    {
      const char *constinputs;
      char *inputs,*str;
      char *tok;

      if ((constinputs = get_param_str("pgserver","input",NULL))) {
	str = inputs = strdup(constinputs);

	while ((tok = strtok(str," \t"))) {
	  if (iserror(prerror(
			      load_inlib(find_inputdriver(tok),NULL)
			      ))) 
	    return 1;

	  str = NULL;
	}
	free(inputs);
      }
    } 

    /* Input filters should be initialized before video drivers,
     * since some video drivers may need to set up their own input
     * filters. (sdlgl, particularly.)
     * Since the input filters are pretty simple, this shouldn't hurt anything.
     */
#ifdef DEBUG_INIT
    printf("Init: infilter\n");
#endif
    if (iserror(prerror(infilter_init())))  return 1;

    /* Before loading the video driver, load the palette */
#ifdef CONFIG_PAL8_CUSTOM
    if (iserror(prerror(load_custom_palette(get_param_str("pgserver","palette",NULL)))))
      return 1;
#endif

    /* Process video driver config options */
    vidw = get_param_int("pgserver","width",0);
    vidh = get_param_int("pgserver","height",0);
    vidd = get_param_int("pgserver","depth",0);
    vidf = get_param_int("pgserver","vidflags",0);
#if defined(CONFIG_XCOPILOT) || defined(CONFIG_SOFT_CHIPSLICE)
    sscanf(get_param_str("pgserver","mode",NULL),"%dx%dx%d",&vidw,&vidh,&vidd);
#else
    sscanf(get_param_str("pgserver","mode",""),"%dx%dx%d",&vidw,&vidh,&vidd);
#endif

    /* Add rotation flags */
    switch (get_param_int("pgserver","rotate",0)) {
    case 90:
      vidf |= PG_VID_ROTATE90;
      break;
    case 180:
      vidf |= PG_VID_ROTATE180;
      break;
    case 270:
      vidf |= PG_VID_ROTATE270;
      break;
    }

    /* Force a specific video driver? */
    if ((str = get_param_str("pgserver","video",NULL))) {
      if (!(viddriver = find_videodriver(str))) {
	prerror(mkerror(PG_ERRT_BADPARAM,77));
	exit(1);
      }
      if (iserror(prerror(
			  load_vidlib(viddriver,vidw,vidh,vidd,vidf)
			  )))
	exit(1);  
    }
    else {
      /* Try to detect a driver (see driverinfo.c) */
      struct vidinfo *p = videodrivers;

      while (p->name) {
	if (!iserror(
		     load_vidlib(p->regfunc,vidw,vidh,vidd,vidf)
		     ))
	  /* Yay, found one that works */
	  break;
	p++;
      }
      if (!p->name) {
	/* Oh well... */
	prerror(mkerror(PG_ERRT_IO,78));
	exit(1);
      }
    }

    /* Subsystem initialization and error check */

#ifdef DEBUG_INIT
    printf("Init: font\n");
#endif
    if (iserror(prerror(font_init())))   return 1;
#ifdef DEBUG_INIT
    printf("Init: divtree\n");
#endif
    if (iserror(prerror(dts_new())))     return 1;
#ifdef DEBUG_INIT
    printf("Init: net\n");
#endif
    if (iserror(prerror(net_init())))    return 1;
#ifdef DEBUG_INIT
    printf("Init: appmgr\n");
#endif
    if (iserror(prerror(appmgr_init()))) return 1;
#ifdef DEBUG_INIT
    printf("Init: timer\n");
#endif
    if (iserror(prerror(timer_init())))  return 1;

#ifndef WINDOWS   /* This is also broke for windoze */

    /* Load theme files, keep the list around so they can be
     * reloaded if necessary */

#ifdef DEBUG_INIT
    printf("Init: loading themes\n");
#endif

    /* If we aren't loading any themes, set the default nav hotkeys */
    if (!themefiles)
      reload_hotkeys();

    /* Load us some themes */
    if (iserror(prerror(load_themefile_list(themefiles))))
      return 1;

#endif /* WINDOWS */

  }

  /*************************************** More Initialization */

#ifdef CONFIG_VIDEOTEST   /* Video test mode */
  if (videotest_on==1)
    videotest_run(videotest_mode);
  if (videotest_on==2) {
    videotest_benchmark();
    mainloop_proceed = 0;   /* Don't bother with running a server :) */
  }
       
  /* initial update */
  if (!videotest_on)    /* If we have a test pattern, leave that up */
#endif   
    update(NULL,1);

  /* Need to calibrate touchscreen? */
#ifdef CONFIG_TOUCHSCREEN
  if (prerror(touchscreen_init(&use_tpcal)))
    return 1;
#endif

  /* Start the first child process, either tpcal or the session manager */
  if (use_tpcal) {
    if (!run_config_process("tpcal")) {
      if (run_config_process("session"))
	use_sessionmgmt = 1;
    }
    else
      sessionmgr_secondary = 1;
  }
  else {
    if (run_config_process("session"))
      use_sessionmgmt = 1;    
  }

  /* Done! */
  in_init = 0;
  /*************************************** Main loop */

#ifdef DEBUG_INIT
  printf("Initialization done");
  guru("Initialization done!\n\n(This message brought to you\nby DEBUG_INIT)");
#endif

  /* warn all the drivers (esp. the eventbroker) that we are ready */
  drivermessage (PGDM_READY, 0, NULL);

  while (mainloop_proceed) {
    net_iteration();

    if (sessionmgr_start) {
      if (run_config_process("session"))
	use_sessionmgmt = 1;    
      sessionmgr_start = 0;
    }
  }

  /*************************************** cleanup time */

  timer_release();
  handle_cleanup(-1,-1);
  hotspot_free();
  dts_free();
  net_release();
  appmgr_free();
  grop_kill_zombies();
  if (vid) {
    if (vid->display && ((struct stdbitmap *)vid->display)->rend &&
	vid->bitmap_getsize==def_bitmap_getsize)
      g_free(((struct stdbitmap *)vid->display)->rend);
    VID(close) ();
  }
  cleanup_inlib();   /* Cleanup inlib after video drivers, since video drivers may
		      * delete inlibs they've loaded automatically.
		      */
  configfile_free();
  errorload(NULL);

  {  /* Free the list of loaded theme files */
    struct themefilenode *p,*condemn;
    p = themefiles;
    while (p) {
      condemn = p;
      p = p->next;
      g_free(condemn);
    }
  }
  
  /* Check for memory leaks and, finally, exit */
  if (memref!=0)
    memoryleak_trace();
  exit(server_returnval);
}

void request_quit(void) {
#ifdef WINDOWS
  mainloop_proceed = 0;
#else
  kill(getpid(),SIGTERM);
#endif
}
   
/* This is called whenever video is reloaded at a higher color depth
 * to reload all themes passed on the command line */
g_error reload_initial_themes(void) {
  /* If we're still initializing, don't need to do this */
  if (in_init)
    return;

  return load_themefile_list(themefiles);
}

/* This loads a list of theme files into pgserver */
g_error load_themefile_list(struct themefilenode *list) {
  struct themefilenode *p;
  g_error e;
  unsigned char *themebuf;
  int fd;
  struct stat st;
  const char *themedir;

  /* See if we have a theme directory... */
  themedir = get_param_str("pgserver","themedir",NULL);

  for (p=list;p;p=p->next) {
    char *filename;
    char pathbuffer[1024];

    /* Kill the previous load */
    if (p->h)
      handle_free(-1,p->h);
      
    if (themedir) {
      pathbuffer[sizeof(pathbuffer)-1] = 0;
      snprintf(pathbuffer,sizeof(pathbuffer)-1,"%s/%s",themedir,p->name);
      filename = pathbuffer;
    }
    else
      filename = p->name;

    /* Load theme from file */
    if ((fd = open(filename,O_RDONLY))<=0)
      return mkerror(PG_ERRT_IO,109);       /* Can't find a theme file */
    fstat(fd,&st);
    e = g_malloc((void**)&themebuf,st.st_size);
    errorcheck;
    read(fd,themebuf,st.st_size);
    close(fd);
    e = theme_load(&p->h,-1,themebuf,st.st_size);
    errorcheck;
      
    /* FIXME: Theme not loaded in the correct order */
      
    g_free(themebuf);
  }
  return success;
}

/* The End */









