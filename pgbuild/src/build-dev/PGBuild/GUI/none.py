""" PGBuild.GUI.none

The 'none' GUI, that provides only command-line-driven functionality
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

import PGBuild.CommandLine.Output

description = "Command line interface only"
priority = 10

class Interface:
    def __init__(self, config, progressClass=PGBuild.CommandLine.Output.Progress):
        self.config = config

        # Set up a progress reporter object at the specified verbosity
        self.verbosity = int(config.eval("invocation/option[@name='verbosity']/text()"))
        self.progress = progressClass(self.verbosity)

    def run(self):
        """Examine the provided configuration and take the specified actions"""
        
        import PGBuild.Site
        t = self.progress.task("Debuggative cruft")
        p = self.config.packages.findPackage('picogui')
        t.report('package', p)
        v = p.findVersion()
        t.report('version', v)
        t.report('site', v.findMirror(t).absoluteURI)

        treeDumpFile = self.config.eval("invocation/option[@name='treeDumpFile']/text()")
        if treeDumpFile:
            f = open(treeDumpFile, "w")
            f.write(self.config.toprettyxml())
            f.close()
        
### The End ###
        
    
