/* A class to wrap libpgserver */

#ifndef _H_EMBEDDEDPGSERVER
#define _H_EMBEDDEDPGSERVER

extern "C" {
#include <pgserver/g_error.h>
}

class EmbeddedPGserver {
 public:
  EmbeddedPGserver(int argc, char **argv);
  ~EmbeddedPGserver();

  bool mainloopIsRunning(void);
  void mainloopIteration(void);

 private:
  void mainloopStart(void);
  const char *getParam(const char *section, const char *param, const char *def);
  int getParam(const char *section, const char *param, int def);
  void setParam(const char *section, const char *param, const char *value);
};

class PicoGUIException {
 public:
  PicoGUIException(g_error e);
  void show(void);
 private:
  g_error e;
};

#endif /* _H_EMBEDDEDPGSERVER */
