""" PGBuild.GUI.auto

The 'auto' GUI, that automatically picks a GUI module in order of preference
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

import PGBuild.GUI

description = "Automatically choose a GUI"
priority = 0

# Sort the GUIs by priority
guiList = []
for name in PGBuild.GUI.getNames():
    guiList.append(PGBuild.GUI.find(name))
def prioritySort(a,b):
    return cmp(a.priority, b.priority)

# Transmogrify our interface into the highest priority GUI
Interface = guiList[-1].Interface
        
### The End ###
        
    
