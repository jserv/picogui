/* $Id: pgmain.c,v 1.32 2001/04/11 02:28:59 micahjd Exp $
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
   
  {  /* Restrict the scope of these vars so they go away after
	initialization is done with them */

    int c,fd;
#ifndef WINDOWS
    struct stat st;
#endif
    unsigned char *themebuf;
    struct themefilenode *tail = NULL,*p;
    handle h;

    /* Default video mode: 0x0x0 (driver chooses) */
    int vidw=0,vidh=0,vidd=0,vidf=0;

    /* Drivers */
    g_error (*viddriver)(struct vidlib *v) = NULL;
    
#ifndef WINDOWS    /* Command line processing is broke in windoze */

    while (1) {

      c = getopt(argc,argv,"mfrbhlx:y:d:v:i:t:s:");
      if (c==-1)
	break;
      
      switch (c) {

      case 'f':        /* Fullscreen */
	vidf |= PG_VID_FULLSCREEN;
	break;
	 
      case 'b':        /* Double-buffering */
        vidf |= PG_VID_DOUBLEBUFFER;
	break;
	 
#ifdef CONFIG_ROTATE
      case 'r':        /* Rotate */
	vidf |= PG_VID_ROTATE90;
	break;
#endif
	 
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

      case 'x':        /* Width */
       	vidw = atoi(optarg);
	break;

      case 'y':        /* Height */
	vidh = atoi(optarg);
	break;

      case 'd':        /* Depth */
       	vidd = atoi(optarg);
	break;

      case 'v':        /* Video */
	if (!(viddriver = find_videodriver(optarg))) {
	  prerror(mkerror(PG_ERRT_BADPARAM,77));
	  exit(1);
	}
	break;

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

#ifdef CONFIG_VIDEOTEST /* Video test mode */
      case 's':
	videotest_on = 1;
	videotest_mode = atoi(optarg);
	if (!videotest_mode) {
	   videotest_help();
	   exit(1);
	}
	break;

      case 'm':
	videotest_on = 2;
        break;
#endif
	 
      default:        /* Need help */
#ifndef CONFIG_TEXT
	puts("Commandline error");
#else
	puts("PicoGUI server (pgui.sourceforge.net)\n\n"
#ifdef DEBUG_ANY
	     "DEBUG MODE ON\n\n"
#endif
	     "usage: pgserver [-fbhl] [-x width] [-y height] [-d depth] [-v driver]\n"
	     "                [-i driver] [-t theme] [session manager...]\n\n"
	     "  f : Fullscreen mode (if the driver supports it)\n"
	     "  b : double-buffering (if the driver supports it)\n"
	     "  h : This help message\n"
	     "  l : List installed drivers and fonts\n"
#ifdef CONFIG_ROTATE
	     "  r : Begin with screen rotated\n"
#endif
#ifdef CONFIG_VIDEOTEST
	     "  m : enter benchmark mode\n"
#endif
	     "\n"
	     "  x width   : default screen width\n"
	     "  y height  : default screen height\n"
	     "  d depth   : default bits per pixel\n"
	     "  v driver  : default video driver (see -l)\n"
	     "  i driver  : load an input driver, can use more than one (see -l)\n"
	     "  t theme   : load a compiled theme file, can use more than one\n"
#ifdef CONFIG_VIDEOTEST
	     "  s modenum : enter video test mode. modenum = 'help' to list modes\n"
#endif
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
     
    if (viddriver) {
      /* Force a specific driver */

      if (iserror(prerror(
			  load_vidlib(viddriver,vidw,vidh,vidd,vidf)
			  ))) exit(1);  
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

#ifdef CONFIG_VIDEOTEST   /* Video test mode */
    if (videotest_on==1)
       videotest_run(videotest_mode);
    if (videotest_on==2)
       videotest_benchmark();
#endif
     
     
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

  /* initial update */
#ifdef CONFIG_VIDEOTEST
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
  dts_free();
  net_release();
  appmgr_free();
  if (vid)
    VID(close) ();

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









