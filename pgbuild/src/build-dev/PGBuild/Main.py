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

    progress.message("This is a normal message")
    progress.message("This is an unimportant message",2)
    progress.message("This is an important message",0)

    t = progress.task("Unimportant task",1)
    t.message("This is a normal message")
    t.message("This is an unimportant message",2)
    t.message("This is an important message",0)

    treeDumpFile = config.eval("invocation/option[@name='treeDumpFile']/text()")
    if treeDumpFile:
        f = open(treeDumpFile, "w")
        f.write(config.toprettyxml())
        f.close()


def main(bootstrap, argv):
    """The entry point called by build.py. Most of the work is done by run(),
       this just handles:

         - Initializing the config tree
         - Initializing the Progress object
         - Exception catching
       """
    import PGBuild.CommandLine.Options
    import PGBuild.CommandLine.Output
    import PGBuild.Config
    config = PGBuild.Config.Tree()
    try:
        # Load the options passed to use by build.py into the <bootstrap> section
        config.boot(bootstrap)

        # Parse command line options into the <invocation> section
        PGBuild.CommandLine.Options.parse(config, argv)

        # Set up a progress reporter object at the specified verbosity
        verbosity = int(config.eval("invocation/option[@name='verbosity']/text()"))
        progress = PGBuild.CommandLine.Output.Progress(verbosity)

        # Do everything else :)
        run(config, progress)
    finally:
        config.commit()

### The End ###
        
    
