#!/usr/bin/env python
""" build.py

Command-line entry point for the PGBuild system, responsible only for setting
paths and transferring control to the PGBuild.Main module. This file should
be as simple as possible, since it can't be automatically updated, but it
should also contain all details specific to this directory layout.
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
    
    # Store the path this script is in as the PGBuild root path
    rootPath = os.path.abspath(os.path.dirname(sys.argv[0]))

    # Set the other important paths
    binPath       = os.path.join(rootPath, 'bin')
    localConfPath = os.path.join(rootPath, 'conf')
    packagePath   = os.path.join(rootPath, 'src')

    # Set the name of the two packages PGBuild requires for bootstrapping
    buildPackage  = "build-dev"
    confPackage   = "conf-dev"

    # Get paths for the bootstrap packages
    buildPackagePath = os.path.join(packagePath, buildPackage)
    confPackagePath  = os.path.join(packagePath, confPackage)

# Use the Bootstrap class to locate our build package and call its Main
boot = Bootstrap()
sys.path.append(boot.buildPackagePath)
import PGBuild.Main
PGBuild.Main.main(boot, sys.argv)

### The End ###
