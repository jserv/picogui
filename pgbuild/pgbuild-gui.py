#!/usr/bin/env python
#
# Yet another entry point, but automatically prepends --gui to all the given
# command line options, causing a graphical interface to be opened. This is for
# the convenience of point-and-click operating systems like Windows and MacOS.
#
import pgbuild
pgbuild.main(['--gui=auto'])
