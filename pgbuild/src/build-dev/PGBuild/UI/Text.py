""" PGBuild.UI.Text

The 'text' UI, that provides progress reporting and other UI functionality
specific to running in a command line environment.
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
_svn_id = "$Id$"

import PGBuild.UI.None
import sys, os

class Colorizer(object):
    """Detect whether VT100-style color escape codes are supported,
       and if so, provide an interface for outputting them.
       """

    # Map color/attribute names to ECMA-48 SGR attributes
    attrMap = {
        'reset': 0,
        'bold': 1,
        'red': 31,
        'green': 32,
        'brown': 33,
        'blue': 34,
        'magenta': 35,
        'cyan': 36,
        'white': 37,
        }
    
    def __init__(self):
        self.supported = 0
        try:
            self.term = os.getenv('TERM')
            if self.term in ('vt100', 'vt102', 'xterm', 'linux', 'Eterm'):
                self.supported = 1
        except:
            pass

    def set(self, attrs):
        if self.supported:
            def lookupColor(name):
                return self.attrMap[name]
            sys.stdout.write("\x1b[%sm" % (",".join(map(str,map(lookupColor,attrs)))))

    def write(self, str, color=None):
        if color:
            self.set(color)
        sys.stdout.write(str)
        if color:
            self.set(('reset',))

    def setTitle(self, title):
        if self.term in ('xterm', 'Eterm'):
            sys.stdout.write("\x1b]2;%s\x07" % title)

    def cleanup(self):
        self.setTitle(self.term)
        

class Progress(PGBuild.UI.None.Progress):
    """Progress reporter for command line use, with optional color"""

    def _init(self):
        # Set up our Colorizer object
        if self.parent:
            self.color = self.parent.color
        else:
            self.color = Colorizer()

    def _showTaskHeading(self):
        self.color.write(" ")
        self.color.write("-" * self.indentLevel, ('bold',))
        self.color.write(" %s..." % self.taskName, ('bold', 'cyan'))
        self.color.write("\n")
        self.color.setTitle("%s - %s" % (PGBuild.name, self.taskName))
        
    def _report(self, verb, noun):
        self.color.write("%10s " % verb)
        self.color.write(":", ('bold',))
        self.color.write(" %s\n" % noun)

    def _textBlock(self, text, color=None, bullet="*"):
        """Output a block of text in the given color"""
        for line in text.split("\n"):
            self.color.write(" %s " % bullet, ('bold',))
            self.color.write(line, color)
            self.color.write("\n")
            bullet = ' '

    def _warning(self, text):
        self._prefixedTextBlock(text, "Warning:", ('bold', 'brown'))
            
    def _error(self, text):
        self._prefixedTextBlock(text, "Error:", ('bold', 'red'))

    def _prefixedTextBlock(self, text, prefix, color):
        if len(text.split("\n")) > 1:
            separator = "\n"
        else:
            separator = " "
        self._textBlock(prefix + separator + text, color)

    def _message(self, text):
        self._textBlock(text)

    def cleanup(self):
        self.color.cleanup()


class Interface(PGBuild.UI.None.Interface):
    progressClass = Progress

    def cleanup(self, ctx):
        try:
            ctx.progress.cleanup()
        except AttributeError:
            pass

### The End ###
        
    
