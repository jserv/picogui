#!/usr/bin/env python
""" pgbuild.py

Bootstrap pathnames and packagenames for PGBuild, can function as an entry
point identically to 'build' for operating systems that don't support "#!" lines
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

import sys, os

class Bootstrap:
    """PGBuild bootstrap object, holds pathnames and other
       settings necessary to bootstrap the rest of the system.
       """
    def __init__(self):
        self.paths = {}
        self.packages = {}

        # Store the path this script is in as the PGBuild root path
        self.paths['root'] = os.path.abspath(os.path.dirname(sys.argv[0]))

        # Set the other important paths
        self.paths['bin']       = os.path.join(self.paths['root'], 'bin')
        self.paths['localConf'] = os.path.join(self.paths['root'], 'conf')
        self.paths['packages']  = os.path.join(self.paths['root'], 'src')
        
        # Set the name of the two packages PGBuild requires for bootstrapping
        self.packages['build'] = "build-dev"
        self.packages['conf']  = "conf-dev"

def main(extraArgs=[]):
    # Use the Bootstrap class to locate our build package and call its Main
    boot = Bootstrap()
    sys.path.insert(0, os.path.join(boot.paths['packages'], boot.packages['build']))
    import PGBuild.Main
    PGBuild.Main.main(boot, [sys.argv[0]] + extraArgs + sys.argv[1:])

if __name__ == '__main__':
    main()

### The End ###
