""" PGBuild.GUI

Contains modules to implement PGBuild user interfaces using different
frameworks. This includes the 'none' GUI, that provides the standard
command line interface.
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

import PGBuild.Errors
import os, glob

def getNames():
    """List the names of all GUI modules"""
    modules = []
    for file in os.listdir(__path__[0]):
        if file[-3:] == ".py" and file != "__init__.py":
            modules.append(file[:-3])
    return modules

def getPrioritizedModules():
    """List the working GUI modules, highest priority first"""
    guiList = []
    for name in PGBuild.GUI.getNames():
        try:
            guiList.append(PGBuild.GUI.find(name))
        except:
            pass
    def prioritySort(a,b):
        return cmp(b.priority, a.priority)
    guiList.sort(prioritySort)
    return guiList

def find(name):
    try:
        return __import__(name, globals(), locals())
    except ImportError:
        raise PGBuild.Errors.UserError("Unknown GUI module '%s'" % name)
        
### The End ###
        
    
