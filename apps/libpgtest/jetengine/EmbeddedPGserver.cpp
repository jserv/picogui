/* A class to wrap libpgserver */

extern "C" {
#include <pgserver/common.h>
#include <pgserver/init.h>
#include <pgserver/os.h>
#include <pgserver/configfile.h>
}

#include "EmbeddedPGserver.h"


EmbeddedPGserver::EmbeddedPGserver(int argc, char **argv) {
  g_error e;

  /* Set defaults */
  setParam("pgserver", "video", "sdlgl");
  setParam("pgserver", "font_engine", "ftgl");
  setParam("font-ftgl", "path", "../../../fonts");
  setParam("pgserver", "dragsolid", "1");
  setParam("video-sdlgl", "caption", "JetEngine");
  setParam("opengl", "continuous", "1");

  /* Load without a config file */
  e = pgserver_init(PGINIT_NO_CONFIGFILE, argc, argv);
  if (iserror(e))
    throw PicoGUIException(e);
  
  mainloopStart();
}  

EmbeddedPGserver::~EmbeddedPGserver() {
  g_error e;

  e = pgserver_shutdown();
  if (iserror(e))
    throw PicoGUIException(e);
}  

const char *EmbeddedPGserver::getParam(const char *section, const char *param, const char *def) {
  return get_param_str(section,param,def);
}

int EmbeddedPGserver::getParam(const char *section, const char *param, int def) {
  return get_param_int(section,param,def);
}

void EmbeddedPGserver::setParam(const char *section, const char *param, const char *value) {
  g_error e;

  e = set_param_str(section,param,value);
  if (iserror(e))
    throw PicoGUIException(e);  
}

void EmbeddedPGserver::mainloopStart(void) {
  pgserver_mainloop_start();
}
 
bool EmbeddedPGserver::mainloopIsRunning(void) {
  return pgserver_mainloop_is_running();
}

void EmbeddedPGserver::mainloopIteration(void) {
  g_error e;
  
  e = pgserver_mainloop_iteration();
  if (iserror(e))
    throw PicoGUIException(e);  
}

PicoGUIException::PicoGUIException(g_error e_) {
  e = e_;
}

void PicoGUIException::show(void) {
  os_show_error(e);
}
