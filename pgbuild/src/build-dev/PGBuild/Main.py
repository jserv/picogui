""" PGBuild.Main

Frontend for PGBuild- provides command line processing and functions for
carrying out high-level tasks. This module's main() is invoked by build.py
as soon as it creates a Bootstrap object with vital path and package names.
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

from optik import OptionParser
import PGBuild
import PGBuild.Config

def processCommandLine(argv):
    """Process command line options using Optik"""
    parser = OptionParser(version=PGBuild.version)
    parser.parse_args(argv)

def main(bootstrap, argv):
    """The entry point called by build.py"""
    try:
        processCommandLine(argv)
        PGBuild.Config.default.boot(bootstrap)
    finally:
        PGBuild.Config.default.commit()

### The End ###
        
    
