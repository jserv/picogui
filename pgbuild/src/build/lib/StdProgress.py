#!/usr/bin/env python
"""
 StdProgress.py - A progress reporter using stdout
"""
# 
# PicoGUI Build System
# Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
# 
#  This library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public
#  License as published by the Free Software Foundation; either
#  version 2.1 of the License, or (at your option) any later version.
#  
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Lesser General Public License for more details.
#  
#  You should have received a copy of the GNU Lesser General Public
#  License along with this library; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
# 

import os
import sys

class Colorizer:
    """Detect whether VT100-style color escape codes are supported,
       and if so, provide an interface for outputting them.
       """

    # Map color/attribute names to ECMA-48 SGR attributes
    attrMap = {
        'reset': 0,
        'bold': 1,
        'red': 32,
        'green': 32,
        'brown': 33,
        'blue': 34,
        'magenta': 35,
        'cyan': 36,
        'white': 37,
        }
    
    def __init__(self):
        self.supported = 0
        try:
            term = os.getenv('TERM')
            if term in ('vt100', 'vt102', 'xterm', 'linux', 'Eterm'):
                self.supported = 1
        except:
            pass

    def set(self, attrs):
        if self.supported:
            def lookupColor(name):
                return self.attrMap[name]
            sys.stdout.write("\x1b[%sm" % (",".join(map(str,map(lookupColor,attrs)))))

    def write(self, str, color=None):
        if color:
            self.set(color)
        sys.stdout.write(str)
        if color:
            self.set(('reset',))
        

class StdProgress:
    """Progress reporter based on stdout and Colorizer"""

    def __init__(self):
        self.color = Colorizer()
    
    def report(self, verb, noun):
        self.color.write("%15s " % verb)
        self.color.write(":", ('bold',))
        self.color.write(" %s\n" % noun)

    def task(self, name):
        self.color.write(" - ", ('bold',))
        self.color.write("%s...\n" % name, ('bold', 'cyan'))
        return StdProgress()
    
if __name__ == '__main__':
    p = StdProgress()
    task = p.task('Frobbing sprockets in group A')
    task.report('frobbed', 'super sprocket')
    task.report('frobbed', '/dev/sprocket')
    task.report('frobbed', 'more sprockets')
    task = p.task('Cleaning up')
    task.report('scrubbed', 'super sprocket')
    task.report('scrubbed', '/dev/sprocket')
    task.report('scrubbed', 'more sprockets')
    
