/* Small demo for integrating picogui support into an OpenGL app.
 * This demo will evolve as pgserver's embedding support does.
 * Right now it's all rather hackish, and just intended to be a proof-of-concept.
 *
 * To compile this, enable the 'libpgserver' option in pgserver's menuconfig,
 * then link this with -lpgserver
 *
 * -- Micah Dowty <micahjd@users.sourceforge.net>
 */

#include <stdio.h>                /* fopen() and friends */
#include <pgserver/common.h>      /* data types and exceptions, must come first */
#include <pgserver/init.h>        /* pgserver_init, pgserver_shutdown */
#include <pgserver/requests.h>    /* request_exec, struct request_data and friends */
#include <pgserver/os.h>          /* os_show_error */
#include <pgserver/handle.h>      /* handle type */
#include <pgserver/configfile.h>  /* set_param_str */
#include <pgserver/pgnet.h>       /* net_iteration */
#include <GL/gl.h>                /* OpenGL of course! */

/* Necessary for the hack in the main loop below */
extern int mainloop_runflag;


/* The OpenGL scene we draw below the GUI */
void scene(void) {
  static float xr,yr,zr;
  static float light1_diffuse[] = {0.5,0.8,1,1};
  static float light1_ambient[] = {0.2,0.2,0.2,0};
  static float light1_position[] = {-10,5,-5,0};

  /* pgserver sets up its own camera matrix at the beginning of
   * its frame, so it's okay if we wipe out whatever matrix is set.
   */
  glLoadIdentity();
  glClearColor(0,0,0,0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glLightfv(GL_LIGHT1, GL_DIFFUSE, light1_diffuse);
  glLightfv(GL_LIGHT1, GL_POSITION, light1_position);
  glLightfv(GL_LIGHT1, GL_AMBIENT, light1_ambient);

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glDisable(GL_BLEND);
  glDisable(GL_TEXTURE_2D);
  glEnable(GL_LIGHT1);
  glEnable(GL_LIGHTING);
  glColor3f(1,1,1);

  glTranslatef(0.0f,0.0f,-5.0f);
  glRotatef(xr,1.0f,0.0f,0.0f);
  glRotatef(yr,0.0f,1.0f,0.0f);
  glRotatef(zr,0.0f,0.0f,1.0f);
  xr += 0.1;
  yr += 0.2;
  zr += 0.05;

  glBegin(GL_QUADS);  /* Cube */
  
  glNormal3f(0,0,-1);
  glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f,  1.0f);
  glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f,  1.0f);
  glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f,  1.0f);
  glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f,  1.0f);

  glNormal3f(0,0,1);
  glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f, -1.0f);
  glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f,  1.0f, -1.0f);
  glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f,  1.0f, -1.0f);
  glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, -1.0f, -1.0f);

  glNormal3f(0,-1,0);
  glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f, -1.0f);
  glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f,  1.0f,  1.0f);
  glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f,  1.0f,  1.0f);
  glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f, -1.0f);

  glNormal3f(0,1,0);
  glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f, -1.0f, -1.0f);
  glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f, -1.0f, -1.0f);
  glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, -1.0f,  1.0f);
  glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f,  1.0f);

  glNormal3f(1,0,0);
  glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f, -1.0f);
  glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f, -1.0f);
  glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f,  1.0f,  1.0f);
  glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, -1.0f,  1.0f);

  glNormal3f(-1,0,0);
  glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f, -1.0f);
  glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f,  1.0f);
  glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f,  1.0f,  1.0f);
  glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f, -1.0f);

  glEnd();

  /* pgserver normally runs with depth test and lighting off.
   * It will set blending and texturing as needed.
   */
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);
}

 
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
   
  /* Force the sdlgl video driver */
  e = set_param_str("pgserver", "video", "sdlgl");
  errorcheck;

  /* Load a slightly hacked up lucid theme */
  e = set_param_str("pgserver", "themes", "gl_lucid.th");
  errorcheck;

  /* Use the Freetype2 OpenGL font engine */
  e = set_param_str("pgserver", "font_engine", "ftgl");
  errorcheck;

  /* Standard picogui fonts */
  e = set_param_str("font-ftgl", "path", "../../fonts");
  errorcheck;

  /* No need for sprite dragging */
  e = set_param_str("pgserver", "dragsolid", "1");
  errorcheck;

  /* Custom title */
  e = set_param_str("video-sdlgl", "caption", "PicoGUI server embedding test");
  errorcheck;

  /* Redraw frames continuously instead of just when something changes */
  e = set_param_str("video-sdlgl", "continuous", "1");
  errorcheck;

  /* Define a configuration without using the usual config files.
   * Command line processing is optional as well, though it's useful
   * to have in this demo.
   * See pgserver/init.h for more info.
   */
  e = pgserver_init(PGINIT_NO_CONFIGFILE,argc,argv);
  errorcheck;

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

  /* This is a hack.. pgserver's standard main loop uses this runflag to
   * terminate cleanly when it gets an interrupt, it's closed by the window
   * manager, etc.
   * This whole main loop should be cleaned up when pgserver's main loop
   * is reorganized.
   */
  mainloop_runflag = 1;
  while (mainloop_runflag) {
    /* This is our custom scene that we'll render beneath the GUI.
     * If the theme has no 'holes' for this to show through, you can see it
     * by using CTRL-ALT-Q to move the camera then CTRL-ALT-G to disable the grid.
     */
    scene();

    /* The guts of pgserver's main loop, processing network and input events.
     * This also happens to redraw the frame in the sdlgl driver.
     * Necessary for now to recieve input. It's likely a real opengl game would
     * have its own input driver code, and would just insert events using requests.
     */
    net_iteration();
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
    os_show_error(e);
    return 1;
  }
  return 0;
}

/* The End */
