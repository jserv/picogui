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


def run(config, progress):
    """Examine the provided configuration and take the specified actions"""

    for dumpFileNode in config.xpath("invocation/option[@name='treeDumpFile']/text()"):
        f = open(dumpFileNode.data, "w")
        f.write(config.toprettyxml())
        f.close()

def main(bootstrap, argv):
    """The entry point called by build.py. Initializes the config tree
       and handles exceptions, letting run() do most of the work.
       """
    import PGBuild.CommandLine.Options
    import PGBuild.CommandLine.Output
    import PGBuild.Config
    config = PGBuild.Config.Tree()
    progress = PGBuild.CommandLine.Output.Progress()
    try:
        config.boot(bootstrap)
        PGBuild.CommandLine.Options.parse(config, argv)
        run(config, progress)
    finally:
        config.commit()

### The End ###
        
    
