""" PGBuild.UI

Contains modules to implement PGBuild user interfaces using different
frameworks. This includes a base that all UIs subclass, and various
command line based interfaces.
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
    """List the names of all UI modules"""
    modules = []
    for file in os.listdir(__path__[0]):
        if file[-3:] == ".py" and file != "__init__.py":
            modules.append(file[:-3])
    return modules

def getPrioritizedModules():
    """List the working UI modules, highest priority first"""
    guiList = []
    for name in PGBuild.UI.getNames():
        try:
            guiList.append(PGBuild.UI.find(name))
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
        raise PGBuild.Errors.UserError("Unknown UI module '%s'" % name)
        
### The End ###
        
    
