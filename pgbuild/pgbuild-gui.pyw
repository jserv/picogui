#!/usr/bin/env python
"""
Yet another entry point, but automatically prepends --ui=auto to all the given
command line options, causing a graphical interface module to be automatically
detected and loaded. This is for the convenience of pointy-clicky operating
systems. You know who you are.
"""
import pgbuild, sys
pgbuild.main(sys.argv[0], ['--ui=auto'] + sys.argv[1:])
