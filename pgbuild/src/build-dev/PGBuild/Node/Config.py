""" PGBuild.Node.Config

Implements PGBuild's configuration tree, formed from multiple XML
documents with one extra restriction placed on them: Any elements
with the same parent, name, and attributes, are treated as equal.
They will be merged, with child nodes appended.

The configuration document is based on PGBuild.Node.XML, so its
elements are both SCons nodes and XML DOM elements. The configuration
document is created by 'mounting' XML files. A read-only mount will
have no further effect after its contents are read and merged with
the config document. A read-write mount will be used to save a
modified configuration document.
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

import PGBuild.Node.XML

class Element(PGBuild.Node.XML.Element):
    

### The End ###
        
    
