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


# List of UI module names and descriptions, sorted by priority.
# The most preferred UIs should go first.
catalog = [
    ("Tk",     "Tkinter based graphical front-end"),
    ("Curses", "Curses based full-screen text front-end"),
    ("Text",   "Command line front-end, with optional colorizing"),
    ("None",   "No front-end"),
    ("Help",   "List the available UI modules"),
    ("Auto",   "Automatically choose a UI module"),
    ]


def find(name):
    """Given a UI module name, this tries loading it. The given name is case-insensitive,
       this function will munge its case to match our naming convention.
       """
    name = name[0].upper() + name[1:].lower()
    try:
        return __import__(name, globals(), locals())
    except ImportError:
        raise PGBuild.Errors.UserError("Unknown UI module '%s'" % name)
        
### The End ###
        
    
