
"""scons.Node.Alias

Alias nodes.

This creates a hash of global Aliases (dummy targets).

"""

#
# Copyright (c) 2001, 2002, 2003 Steven Knight
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
# KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
# WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
# LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
# OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
# WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#

__revision__ = "src/engine/SCons/Node/Alias.py 0.D014 2003/05/21 13:50:45 software"

import UserDict

import SCons.Errors
import SCons.Node
import SCons.Util

class AliasNameSpace(UserDict.UserDict):
    def Alias(self, name):
        if self.has_key(name):
            raise SCons.Errors.UserError
        self[name] = SCons.Node.Alias.Alias(name)
        return self[name]

    def lookup(self, name):
        try:
            return self[name]
        except KeyError:
            return None

class Alias(SCons.Node.Node):
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
        
default_ans = AliasNameSpace()

SCons.Node.arg2nodes_lookups.append(default_ans.lookup)
