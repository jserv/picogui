/* $Id$
 *
 * init.c - High level pgserver initialization and shutdown
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
 * 
 */

#include <pgserver/common.h>
#include <pgserver/init.h>
#include <pgserver/configfile.h>
#include <pgserver/video.h>
#include <pgserver/input.h>
#include <pgserver/divtree.h>
#include <pgserver/appmgr.h>
#include <pgserver/timer.h>
#include <pgserver/hotspot.h>
#include <pgserver/widget.h>
#include <pgserver/handle.h>

#ifdef DEBUG_INIT
#define DEBUG_FILE
#endif
#include <pgserver/debug.h>

/******************************************************** Public functions **/

/* Initialize all pgserver subsystems.
 * Normally this will initialize everything, if you want to change
 * that, pass it some of the above flags. If you want to pass extra
 * config parameters, you can use any of the configfile functions
 * before calling this.
 */
g_error pgserver_init(int flags, int argc, char **argv) {
  g_error e;
  int n;

  DBG("operating system init\n");
  e = os_init();
  errorcheck;

  if (!(flags & PGINIT_NO_CONFIGFILE)) {
    DBG("configuration files\n");
    e = configfile_parse_default();
    errorcheck;
  }

#ifdef CONFIG_OS_POSIX
  if (!(flags & PGINIT_NO_COMMANDLINE)) {
    DBG("command line\n");
    e = commandline_parse(argc,argv);
    errorcheck;
  }
#endif
 
  DBG("error message table\n");
  e = errorload(get_param_str("pgserver","messagefile",NULL));
  errorcheck;

  DBG("input drivers\n");
  e = input_init();
  errorcheck;

  DBG("input filters\n");
  e = infilter_init();
  errorcheck;

  DBG("video\n");
  e = video_init();
  errorcheck;

  DBG("fonts\n");
  e = font_init();
  errorcheck;

  DBG("app manager\n");
  e = appmgr_init();
  errorcheck;

  DBG("network\n");
  e = net_init();
  errorcheck;

  DBG("globals\n");
  e = globals_init();
  errorcheck;

  DBG("timers\n");
  timer_init();

  DBG("initial themes\n");
  e = reload_initial_themes();
  errorcheck;

  DBG("Initial mouse cursor\n");
  cursor_retheme();

#ifdef CONFIG_TOUCHSCREEN
  /* This will queue up the touchscreen calibrator 
   * app as a child process if needed 
   */
  DBG("touchscreen calibration\n");
  e = touchscreen_init();
  errorcheck;
#endif
  
  /* Set up for running the session manager */
  e = childqueue_push(get_param_str("pgserver","session",NULL));
  errorcheck;

  /* The first update, should show the background from the themes
   * we loaded, if any, on the video driver we just loaded.
   */
  update(NULL,1);

  /* Give the drivers a final warning that pgserver is ready to start */
  drivermessage (PGDM_READY, 0, NULL);

  /* Start the first child process */
  childqueue_pop();

  DBG("Initialization done\n");
  return success;
}

/* Shut down all pgserver subsystems */
g_error pgserver_shutdown(void) {
  /* Clean up after each subsystem */
  theme_shutdown();
  handle_cleanup(-1,-1);
  hotspot_free();
  dts_free();
  net_release();
  appmgr_free();
  font_shutdown();
  grop_kill_zombies();
  video_shutdown();
  cleanup_inlib();
  configfile_free();
  errorload(NULL);
  os_shutdown();
  childqueue_shutdown();

  /* Check for memory leaks */
  return memoryleak_trace();
}

g_error pgserver_main(int flags, int argc, char **argv) {
  g_error e;
  int n;

  e = pgserver_init(flags,argc,argv);
  errorcheck;

#ifdef CONFIG_VIDEOTEST
  /* Run a video test if the user asked for that */
  if (get_param_str("pgserver","videotest",NULL)) {
    n = get_param_int("pgserver","videotest",0);
    if (n)
      videotest_run(n);
    else {
      videotest_help();
      return pgserver_shutdown();
    }
  }

  if (get_param_int("pgserver","benchmark",0)) {
    /* Run the benchmarks instead of usual main loop */
    videotest_benchmark();
  }
  else 
#endif /* CONFIG_VIDEOTEST */
  {
    /* Normal main loop */
    e = pgserver_mainloop();
    errorcheck;
  }

  e = pgserver_shutdown();
  errorcheck;
  
  return success;
}

/* The End */

