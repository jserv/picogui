/* $Id: pgmain.c,v 1.9 2001/07/10 11:46:53 micahjd Exp $
 *
 * pgmain.c - Processes command line, initializes and shuts down
 *            subsystems, and invokes the net subsystem for the
 *            main loop.
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000,2001 Micah Dowty <micahjd@users.sourceforge.net>
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

#include <string.h>   /* For strdup() */

#ifdef WINDOWS
#include <process.h>
#else
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
extern char **environ;
#endif

volatile u8 proceed = 1;
volatile u8 in_shutdown = 0, in_init = 1;
int use_sessionmgmt = 0;
extern long memref;
struct dtstack *dts;

#ifdef UCLINUX
extern char *optarg;
extern int optind;
#endif /* UCLINUX */

#ifndef WINDOWS
pid_t my_pid;
void sigterm_handler(int x);
#endif

/* For storing theme files to load later */
struct themefilenode {
  char *name;
  handle h;
  struct themefilenode *next;
};

struct themefilenode *themefiles;

/********** And it all starts here... **********/
int main(int argc, char **argv) {
#ifdef WINDOWS
  /* Fake it */
  int optind = 1;
#endif
#ifdef CONFIG_VIDEOTEST
  int videotest_mode,videotest_on = 0;
#endif
   
#ifndef WINDOWS
  my_pid = getpid();
#endif

  /* Initialize pointer tables here if it can't be done at compile-time */
#ifdef RUNTIME_FUNCPTR
   widgettab_init();
   drivertab_init();
   rqhtab_init();
#endif
   
  /*************************************** Command-line processing */

#ifdef DEBUG_INIT
   printf("Init: processing command line\n");
#endif

  /* Read in global and user-specific config files */
  {
    char *s,*home;

    configfile_parse("/etc/pgserver.conf");

    home = getenv("HOME");
    if (home && !iserror(prerror(g_malloc((void**)&s,strlen(home)+20)))) {
      strcpy(s,home);
      strcat(s,"/.pgserverrc");
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
    handle h;
    int vidw,vidh,vidd,vidf;
    char *str;
    g_error (*viddriver)(struct vidlib *v) = NULL;
    
#ifndef WINDOWS    /* Command line processing is broke in windoze */

    while (1) {

      c = getopt(argc,argv,"hlnv:m:i:t:c:-:");
      if (c==-1)
	break;
      
      switch (c) {

      case 'n':        /* Ignore config data */
	configfile_free();
	break;

      case '-':        /* config option */
	{
	  char *section, *key, *value;

	  if (key = strchr(optarg,'.')) {
	    *key = 0;
	    key++;
	    section = optarg;
	  }
	  else {
	    key = optarg;
	    section = "pgserver";
	  }

	  if (value = strchr(key,'=')) {
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
	
	printf("\n           Fonts:");
	{
	  struct fontstyle_node *p = fontstyles;
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

	puts("\nOptional widgets:"
#ifdef CONFIG_WIDGET_TERMINAL
	     " Terminal"
#endif
#ifdef CONFIG_WIDGET_CANVAS
	     " Canvas  "
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
	if (tail)
	  tail->next = p;
	else
	  themefiles = tail = p;
	tail = p;
	break;

      default:        /* Need help */
#ifndef CONFIG_TEXT
	puts("Commandline error");
#else
	puts("PicoGUI server (pgui.sourceforge.net)\n\n"
#ifdef DEBUG_ANY
	     "DEBUG MODE ON\n\n"
#endif
	     "usage: pgserver [-hln] [-c configfile] [-v driver] [-m WxHxD]\n"
	     "                [--section.key=value] [--key=value] [--key]\n"
	     "                [-i driver] [-t theme] [session manager...]\n\n"
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
	     "  client disconencts.");
#endif
	exit(1);
      }
      
    }

#ifdef DEBUG_INIT
     printf("Init: loading video drivers\n");
#endif

     
#endif /* WINDOWS */

     /* Load alternate messages into the error table */
     if (iserror(prerror(errorload(get_param_str("pgserver",
						 "messagefile",
						 NULL)))))
    return 1;
    
#ifdef CONFIG_VIDEOTEST
     /* Process test mode config options */

     if (str = get_param_str("pgserver","videotest",NULL)) {
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
       char *themes;
       char *tok;

       if (themes = get_param_str("pgserver","themes",NULL)) {
	 themes = strdup(themes);

	 while (tok = strtok(themes," \t")) {

	   if (iserror(prerror(g_malloc((void**)&p,
					sizeof(struct themefilenode)))))
	     return 1;
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
       char *inputs,*str;
       char *tok;

       if (inputs = get_param_str("pgserver","input",NULL)) {
	 str = inputs = strdup(inputs);

	 while (tok = strtok(str," \t")) {
	   if (iserror(prerror(
			       load_inlib(find_inputdriver(tok),NULL)
			       ))) 
	     return 1;

	   str = NULL;
	 }
	 free(inputs);
       }
     } 

    /* Process video driver config options */
    vidw = get_param_int("pgserver","width",0);
    vidh = get_param_int("pgserver","height",0);
    vidd = get_param_int("pgserver","depth",0);
    vidf = get_param_int("pgserver","vidflags",0);
    sscanf(get_param_str("pgserver","mode",""),"%dx%dx%d",&vidw,&vidh,&vidd);

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
    if (str = get_param_str("pgserver","video",NULL)) {
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

    p = themefiles;
    while (p) {

#ifdef DEBUG_INIT
      printf("Init: loading theme '%s'\n",p->name);
#endif
      
      /* Load */
      if ((fd = open(p->name,O_RDONLY))<=0) {
	perror(p->name);
	return 1;
      }
      fstat(fd,&st);
      if (iserror(prerror(g_malloc((void**)&themebuf,st.st_size)))) return 1;
      read(fd,themebuf,st.st_size);
      close(fd);
      if (iserror(prerror(theme_load(&p->h,-1,themebuf,st.st_size)))) return 1;
      g_free(themebuf);

      p = p->next;
    }

#endif /* WINDOWS */

  }

  /*************************************** More Initialization */

#ifdef DEBUG_INIT
   printf("Init: signal handler and subprocess\n");
#endif

#ifndef WINDOWS
  /* Signal handler (it's usually good to have a way to exit!) */
  if (signal(SIGTERM,&sigterm_handler)==SIG_ERR) {
     prerror(mkerror(PG_ERRT_INTERNAL,54));
     exit(1);
  }
  if (signal(SIGINT,&sigterm_handler)==SIG_ERR) {
     prerror(mkerror(PG_ERRT_INTERNAL,54));
     exit(1);
  }
#endif
   
#ifdef CONFIG_VIDEOTEST   /* Video test mode */
    if (videotest_on==1)
       videotest_run(videotest_mode);
    if (videotest_on==2) {
       videotest_benchmark();
       proceed = 0;       /* Don't bother with running a server :) */
    }
       
  /* initial update */
  if (!videotest_on)    /* If we have a test pattern, leave that up */
#endif   
     update(NULL,1);

  /* Now that the socket is listening, run the session manager */

  if (optind<argc && argv[optind]) {
    use_sessionmgmt = 1;

#ifdef WINDOWS
    if (_spawnvp(_P_NOWAIT,argv[optind],argv+optind)<=0) {
      prerror(mkerror(PG_ERRT_BADPARAM,55));
      exit(1);
    }
#else
# ifdef UCLINUX
    if (!vfork()) {
# else
    if (!fork()) {
# endif
      execvp(argv[optind],argv+optind);
      prerror(mkerror(PG_ERRT_BADPARAM,55));
      kill(my_pid,SIGTERM);
      exit(1);
    }
#endif

  }

#ifndef WINDOWS  
  /* We still might have a session manager from the config file */
  if (get_param_str("pgserver","session",NULL)) {
    use_sessionmgmt = 1;
    
# ifdef UCLINUX
    if (!vfork()) {
# else
    if (!fork()) {
# endif
      char *sargv[4];
      sargv[0] = "sh";
      sargv[1] = "-c";
      sargv[2] = get_param_str("pgserver","session",NULL);
      sargv[3] = 0;
      execve("/bin/sh",sargv,environ);
      prerror(mkerror(PG_ERRT_BADPARAM,55));
      kill(my_pid,SIGTERM);
      exit(1);
    }
  }    
#endif

  in_init = 0;
     
  /*************************************** Main loop */

#ifdef DEBUG_INIT
  printf("Initialization done");
  guru("Initialization done!\n\n(This message brought to you\nby DEBUG_INIT)");
#endif
     
  while (proceed)
    net_iteration();

  /*************************************** cleanup time */

  in_shutdown = 1;          /* Disables most individual cleanups in favor
			     * of this bulk extinction of memory */

  timer_release();
  cleanup_inlib();
  handle_cleanup(-1,-1);
  hotspot_free();
  dts_free();
  net_release();
  appmgr_free();
  grop_kill_zombies();
  if (vid)
    VID(close) ();
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
  if (memref!=0) prerror(mkerror(PG_ERRT_MEMORY,56));
  exit(0);
}

#ifndef WINDOWS
void sigterm_handler(int x) {
  proceed = 0;
}
#endif

void request_quit(void) {
#ifdef WINDOWS
  proceed = 0;
#else
  kill(my_pid,SIGTERM);
#endif
}
   
/* This is called whenever video is reloaded at a higher color depth
 * to reload all themes passed on the command line */
g_error reload_initial_themes(void) {
   struct themefilenode *p;
   g_error e;
   unsigned char *themebuf;
   int fd;
   struct stat st;

   /* Don't need to reload them if we're not even done initting yet.
    * It would be bad to reload themes before the appmgr or layout engine
    * is up! */
   if (in_init) return sucess;
   
   for (p=themefiles;p;p=p->next) {
      
      /* Kill the previous load */
      handle_free(-1,p->h);
      
      /* Load theme from file */
      if ((fd = open(p->name,O_RDONLY))<=0)
	 continue;     /* Maybe next time we will beink more sucessful, da? */
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
   return sucess;
}

/* The End */









