/* Small demo for integrating picogui support into an OpenGL app.
 * This demo will evolve as pgserver's embedding support does.
 *
 * The sample app used here is Lesson 37 from the NeHe OpenGL tutorials.
 * nehe.gamedev.net
 *
 * This requires a pgserver with:
 *   - libpgserver support
 *   - the sdlgl video driver
 *   - the sdlinput driver
 *   - the ftgl font engine
 *
 * -- Micah Dowty <micahjd@users.sourceforge.net>
 */

#include <pgserver/common.h>      /* data types and exceptions, must come first */
#include <pgserver/init.h>        /* pgserver_init, pgserver_shutdown */
#include <pgserver/requests.h>    /* request_exec, struct request_data and friends */
#include <pgserver/os.h>          /* os_show_error */
#include <pgserver/handle.h>      /* handle type */
#include <pgserver/configfile.h>  /* set_param_str */
#include <GL/gl.h>                /* OpenGL of course! */
#include <stdio.h>                /* fopen() and friends */

 
/* Our main program, wrapped with exception handling */ 
g_error protected_main(int argc, char **argv) {
  g_error e;
  struct request_data r;
  struct pgrequest req;
  struct pgreqd_handlestruct hs;
  u32 file_size;
  void *file_data;
  FILE *f;
  handle wt, wt_instance;

  /* Here we're adding our own config parameters to pgserver's
   * configuration database. (Below we tell it to init without
   * loading /etc/pgserver.conf and ~/.pgserverrc)
   * Since the config database isn't pgserver-specific, it could
   * be used for general configuration of the host app.
   * See pgserver/configfile.h for the interface.
   */
   
  /* Force the sdlgl video driver.
   * This driver handles OpenGL initialization, shutdown,
   * and page flipping by itself. It will default to 640x480,
   * but you can use standard picogui config vars to change that
   * or to enable fullscreen mode.
   * If you just want to render to an existing OpenGL context,
   * you can use the "glcontext" driver here instead
   */
  e = set_param_str("pgserver", "video", "sdlgl");
  errorcheck;

  /* Load a slightly hacked up lucid theme */
  e = set_param_str("pgserver", "themes", "gl_lucid.th");
  errorcheck;

  /* Use the Freetype2 OpenGL font engine */
  e = set_param_str("pgserver", "font_engine", "ftgl");
  errorcheck;

  /* Standard picogui fonts */
  e = set_param_str("font-ftgl", "path", "../../../fonts");
  errorcheck;

  /* No need for sprite dragging */
  e = set_param_str("pgserver", "dragsolid", "1");
  errorcheck;

  /* Custom title */
  e = set_param_str("video-sdlgl", "caption", "PicoGUI server embedding test");
  errorcheck;

  /* Redraw frames continuously instead of just when something changes */
  e = set_param_str("opengl", "continuous", "1");
  errorcheck;

  /* Define a configuration without using the usual config files.
   * Command line processing is optional as well, though it's useful
   * to have in this demo.
   * See pgserver/init.h for more info.
   */
  e = pgserver_init(PGINIT_NO_CONFIGFILE,argc,argv);
  errorcheck;

  /* OpenGL init for our scene */
  Initialize(640, 480);

  /* Now we'll do a small test of pgserver, loading and displaying a widget
   * template. Note that normally this shouldn't need to call request_exec
   * directly- either a different process or thread should connect to the
   * pgserver, or support for request_exec should be added to cli_c so we
   * can use normal API functions instead of packing everything ourselves.
   */
   
  /* Read in a compiled widget template */
  f = fopen("test.wt","rb");
  fseek(f,0,SEEK_END);
  file_size = ftell(f);
  rewind(f);
  e = g_malloc((void**)&file_data, file_size);
  errorcheck;
  fread(file_data,1,file_size,f);
  fclose(f);

  /* Load the compiled WT */
  memset(&r,0,sizeof(r));
  r.in.req = &req;
  req.type = htons(PGREQ_MKTEMPLATE);
  req.size = htonl(file_size);
  r.in.data = file_data;
  e = request_exec(&r);
  errorcheck;
  wt = r.out.ret;
  if (r.out.free_response_data)
    g_free(r.out.response_data);
  g_free(file_data);

  /* Instantiate the compiled WT */
  hs.h = htonl(wt);
  memset(&r,0,sizeof(r));
  r.in.req = &req;
  req.type = htons(PGREQ_DUP);
  req.size = htonl(sizeof(hs));
  r.in.data = &hs;
  e = request_exec(&r);
  errorcheck;
  wt_instance = r.out.ret;
  if (r.out.free_response_data)
    g_free(r.out.response_data);

  /* Update, necessary for this to appear on screen */
  memset(&r,0,sizeof(r));
  r.in.req = &req;
  req.type = htons(PGREQ_UPDATE);
  e = request_exec(&r);
  errorcheck;
  if (r.out.free_response_data)
    g_free(r.out.response_data);

  /* Run our main loop until something signals it to stop...
   * This could be anything that calls pgserver_mainloop_stop, including
   * interrupts from drivers or the user.
   */
  pgserver_mainloop_start();
  while (pgserver_mainloop_is_running()) {
    /* This is our custom scene that we'll render beneath the GUI.
     * If the theme has no 'holes' for this to show through, you can see it
     * by using CTRL-ALT-Q to move the camera then CTRL-ALT-G to disable the grid.
     */
    Draw();

    /* PicoGUI's main loop, where it processes network, input, and rendering */
    e = pgserver_mainloop_iteration();
    errorcheck;
  }

  /* Shut down all subsystems and perform memory leak detection */
  e = pgserver_shutdown();
  errorcheck;

  return success;
}


/* Exception handling wrapper for main */
int main(int argc, char **argv) {
  g_error e;
   
  /* pgserver has an exception handling mechanism based on the g_error
   * data type and the 'errorcheck' macro. See pgserver/g_error.h
   * for info on this system.
   */
  e = protected_main(argc,argv);
  if (iserror(e)) {

    /* This will print a possibly OS-specific error message. Right now this just
     * dumps it to stderr, but in a future Win32 port for example it may make a dialog.
     */
    os_show_error(e);
    return 1;
  }

  return 0;
}

/* The End */
