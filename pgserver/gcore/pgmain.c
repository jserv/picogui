/* $Id: pgmain.c,v 1.25 2001/02/14 05:13:18 micahjd Exp $
 *
 * pgmain.c - Processes command line, initializes and shuts down
 *            subsystems, and invokes the net subsystem for the
 *            main loop.
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micahjd@users.sourceforge.net>
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

volatile int proceed = 1;
volatile int in_shutdown = 0;
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
  struct themefilenode *next;
};

/********** And it all starts here... **********/
int main(int argc, char **argv) {
#ifdef WINDOWS
  /* Fake it */
  int optind = 1;
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
    struct themefilenode *head = NULL,*tail = NULL,*p;
    handle h;

    /* Default video mode: 0x0x0 (driver chooses) */
    int vidw=0,vidh=0,vidd=0,vidf=0;

    /* Drivers */
    g_error (*viddriver)(struct vidlib *v) = NULL;
    
#ifndef WINDOWS    /* Command line processing is broke in windoze */

    while (1) {

      c = getopt(argc,argv,"fbhlx:y:d:v:i:t:");
      if (c==-1)
	break;
      
      switch (c) {

      case 'f':        /* Fullscreen */
	vidf |= PG_VID_FULLSCREEN;
	break;
	 
      case 'b':        /* Double-buffering */
        vidf |= PG_VID_DOUBLEBUFFER;
	break;

#ifndef TINY_MESSAGES
      case 'l':        /* List */

	puts("\nVideo drivers:");
	{
	  struct vidinfo *p = videodrivers;
	  while (p->name) {
	    printf("  %s",p->name);
	    p++;
	  }
	}

	puts("\n\nInput drivers:");
	{
	  struct inputinfo *p = inputdrivers;
	  while (p->name) {
	    printf("  %s",p->name);
	    p++;
	  }
	}
	
	puts("\n\nFonts:");
	{
	  struct fontstyle_node *p = fontstyles;
	  puts("  Name              Size Normal Bold Italic BoldItalic Fixed Default\n");
	  while (p) {
	    printf("  %-18s%4d   %c     %c     %c        %c        %c      %c\n",
		   p->name,p->size,
		   p->normal ? '*' : ' ',
		   p->bold ? '*' : ' ',
		   p->italic ? '*' : ' ',
		   p->bolditalic ? '*' : ' ',
		   (p->flags & PG_FSTYLE_FIXED) ? '*' : ' ',
		   (p->flags & PG_FSTYLE_DEFAULT) ? '*' : ' ');

	    p = p->next;
	  }
	}

	puts("");
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
	   so for now just store the filenames */
	if (iserror(prerror(g_malloc((void**)&p,
				     sizeof(struct themefilenode))))) return 1;
	p->name = strdup(optarg);
	p->next = NULL;
	if (tail)
	  tail->next = p;
	else
	  head = tail = p;
	tail = p;
	break;

      case '?':        /* Need help */
      case 'h':
#ifdef TINY_MESSAGES
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
	     "  l : List installed drivers and fonts\n\n"
	     "  x width   : default screen width\n"
	     "  y height  : default screen height\n"
	     "  d depth   : default bits per pixel\n"
	     "  v driver  : default video driver (see -l)\n"
	     "  i driver  : load an input driver, can use more than one (see -l)\n"
	     "  t theme   : load a compiled theme file, can use more than one\n"
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

    /* Load theme files and free linked list memory */

#ifdef DEBUG_INIT
   printf("Init: loading themes\n");
#endif

    p = head;
    while (p) {

      /* Load */
      if ((fd = open(p->name,O_RDONLY))<=0) {
	perror(p->name);
	return 1;
      }
      fstat(fd,&st);
      if (iserror(prerror(g_malloc((void**)&themebuf,st.st_size)))) return 1;
      read(fd,themebuf,st.st_size);
      close(fd);
      if (iserror(prerror(theme_load(&h,-1,themebuf,st.st_size)))) return 1;
      g_free(themebuf);

      /* Free memory */
      tail = p;
      p = p->next;
      free(tail->name);  /* Must use the normal free() here because the pointer was
			    not generated with g_malloc, but instead strdup() */
      g_free(tail);
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

  /*************************************** Main loop */

#ifdef DEBUG_INIT
  printf("Initialization done");
  guru("Initialization done!\n\n(This message brought to you\nby DEBUG_INIT)");
#endif
     
  while (proceed)
    net_iteration();

  /*************************************** cleanup time */
  in_shutdown = 1;
  timer_release();
  cleanup_inlib();
  handle_cleanup(-1,-1);
  dts_free();
  net_release();
  appmgr_free();
  if (vid)
    (*vid->close)();
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

/* The End */









