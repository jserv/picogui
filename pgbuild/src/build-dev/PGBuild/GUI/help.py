""" PGBuild.GUI.help

The 'help' GUI, that just lists the available GUIs and exits
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

import PGBuild.GUI.none
import sys

description = "List the available GUIs and exit"
priority = 0

class Interface(PGBuild.GUI.none.Interface):
    def run(self):
        text = "Available GUI modules:\n\n"
        for name in PGBuild.GUI.getNames():
            text += "%10s: %s\n" % (name, PGBuild.GUI.find(name).description)
        self.progress.message(text[:-1])
        
### The End ###
        
    
