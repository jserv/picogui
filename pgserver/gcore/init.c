/* $Id: init.c,v 1.5 2002/11/04 00:24:38 micahjd Exp $
 *
 * init.c - High level pgserver initialization and shutdown
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

  DBG("operating system init");
  e = os_init();
  errorcheck;

  if (!(flags & PGINIT_NO_CONFIGFILE)) {
    DBG("configuration files");
    e = configfile_parse_default();
    errorcheck;
  }

#ifdef CONFIG_OS_POSIX
  if (!(flags & PGINIT_NO_COMMANDLINE)) {
    DBG("command line");
    e = commandline_parse(argc,argv);
    errorcheck;
  }
#endif
 
  DBG("error message table");
  e = errorload(get_param_str("pgserver","messagefile",NULL));
  errorcheck;

  DBG("input drivers");
  e = input_init();
  errorcheck;

  DBG("input filters");
  e = infilter_init();
  errorcheck;

  DBG("fonts");
  e = font_init();
  errorcheck;

  DBG("video");
  e = video_init();
  errorcheck;

  DBG("divtree");
  e = dts_new();
  errorcheck;

  DBG("network");
  e = net_init();
  errorcheck;

  DBG("globals");
  e = globals_init();
  errorcheck;

  DBG("app manager");
  e = appmgr_init();
  errorcheck;

  DBG("timers");
  timer_init();

  DBG("initial themes");
  e = reload_initial_themes();
  errorcheck;

#ifdef CONFIG_TOUCHSCREEN
  /* This will queue up the touchscreen calibrator 
   * app as a child process if needed 
   */
  DBG("touchscreen calibration");
  e = touchscreen_init(void);
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

  DBG("Initialization done");
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
    e = mainloop_run();
    errorcheck;
  }

  e = pgserver_shutdown();
  errorcheck;
  
  return success;
}

/* The End */

