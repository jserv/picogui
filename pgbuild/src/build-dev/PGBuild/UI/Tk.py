""" PGBuild.UI.Tk

A Tkinter-based frontend for PGBuild
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

import PGBuild.UI.None
import PGBuild.Errors
try:
    import tkMessageBox
except ImportError:
    raise PGBuild.Errors.EnvironmentError("Tkinter doesn't seem to be installed")

class Interface(PGBuild.UI.None.Interface):
    def run(self):
        tkMessageBox.showerror(PGBuild.name,
                               "The tk UI is not implemented yet")

### The End ###
        
    
