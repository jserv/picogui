""" PGBuild.Config

Implements PGBuild's configuration tree. The config tree's nodes are
an instance of SCons.Node, so the config tree integrates nicely
with SCons' dependency analysis engine. Persistence is achieved by
'mounting' XML documents into the config tree. These mounts can
be read-write or read-only, and multiple XML documents can be mounted
at the same place in the config tree, for a cascading effect.
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

import SCons.Node


class Config(SCons.Node.Node):
    """The node corresponding to an XML element in exactly one
       source XML file, either read-only or read-write.
       """

    def __init__(self, name):
        SCons.Node.Node.__init__(self)
        self.name = name

    def __str__(self):
        return self.name

    def build(self):
        """A "builder" for aliases."""
        pass

    def current(self, calc):
        """If all of our children were up-to-date, then this
        Alias was up-to-date, too."""
        # Allow the children to calculate their signatures.
        calc.bsig(self)
        state = 0
        for kid in self.children(None):
            s = kid.get_state()
            if s and (not state or s > state):
                state = s
        if state == 0 or state == SCons.Node.up_to_date:
            return 1
        else:
            return 0

    def sconsign(self):
        """An Alias is not recorded in .sconsign files"""
        pass

    def is_under(self, dir):
        # Make Alias nodes get built regardless of 
        # what directory scons was run from. Alias nodes
        # are outside the filesystem:
        return 1

    def get_contents(self):
        """The contents of an alias is the concatenation
        of all the contents of its sources"""
        contents = ""
        for kid in self.children(None):
            contents = contents + kid.get_contents()
        return contents
    

### The End ###
        
    
