/* $Id: EmbeddedPGserver.h,v 1.3 2002/11/26 19:18:07 micahjd Exp $
 *
 * EmbeddedPGserver.h - Interface for a simple wrapper around libpgserver
 *
 * Copyright (C) 2002 Micah Dowty and David Trowbridge
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
 */

#ifndef _H_EMBEDDEDPGSERVER
#define _H_EMBEDDEDPGSERVER

extern "C" {
#include <pgserver/g_error.h>
}
#include "SimpleException.h"

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

class PicoGUIException : public SimpleException {
 public:
  PicoGUIException(g_error e);
  virtual void show(void);
 private:
  g_error e;
};

#endif /* _H_EMBEDDEDPGSERVER */
