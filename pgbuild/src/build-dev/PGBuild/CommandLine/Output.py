""" PGBuild.CommandLine.Output

Utilities for outputting meaningful progress information on the command line
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

import os
import sys

class Colorizer:
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
            term = os.getenv('TERM')
            if term in ('vt100', 'vt102', 'xterm', 'linux', 'Eterm'):
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
        

class Progress:
    """Progress reporter based on Colorizer
       This supports dividing the program's execution into a hierarchy of tasks,
       and reporting progress within each of those tasks.

       Verbosity levels:
         1.0 is always the neutral verbosity level. -v command line switches, for example,
         add 1 to the verbosity level. The -q switch will set it to zero.

       Unimportance levels:
         Tasks and progress reports have unimportance levels- if the task/report's unimportance
         is greater than the verbosity, nothing will be output.

         The unimportance level of a task is added to all of its children's unimportance levels.
         This means that a trivial task can be given an unimportance of 1 for example, so that
         its children will have an unimportance of 2 and will only be output if there was a
         -v command line switch. However, if a critical error with an unimportance of -5 occurs
         in the trivial task, the task heading and that error will both be output.
    """

    def __init__(self, verbosityLevel=1, parent=None, taskName=None):
        self.parent = parent
        self.verbosityLevel = verbosityLevel
        self.taskName = taskName
        self.taskHeadingPrinted = 0
        if parent:
            self.color = parent.color
            self.indentLevel = parent.indentLevel + 1
            self.root = parent.root
        else:
            self.color = Colorizer()
            self.indentLevel = 0
            self.root = self
            self.lastTask = self

    def showTaskHeading(self):
        """We need to print a task heading if we're in a task we've never been in
           before, or if the current task has changed.
           This can also be called manually if a task is starting that will take
           some time before generating any progress reports.
           """
        if self.root.lastTask != self or not self.taskHeadingPrinted:
            if self.parent and not self.parent.taskHeadingPrinted:
                self.parent.showTaskHeading()            
            if self.taskName:
                self.color.write(" ")
                self.color.write("-" * self.indentLevel, ('bold',))
                self.color.write(" %s..." % self.taskName, ('bold', 'cyan'))
                self.color.write("\n")
            self.taskHeadingPrinted = 1
            self.root.lastTask = self

    def _outputTest(self, unimportance):
        """Test whether a message is important enough to output,
           if so return true and make sure our task headings have
           been printed.
           """
        if unimportance > self.verbosityLevel:
            return 0
        self.showTaskHeading()
        return 1
    
    def report(self, verb, noun, unimportance=1):
        if self._outputTest(unimportance):
            self.color.write("%10s " % verb)
            self.color.write(":", ('bold',))
            self.color.write(" %s\n" % noun)

    def task(self, name, unimportance=0):
        """Create a new Progress object representing a hierarchial task"""
        return Progress(self.verbosityLevel - unimportance, self, name)

    def warning(self, text, unimportance=1):
        self.message("Warning: " + text, unimportance, ('bold', 'brown'))
            
    def error(self, text, unimportance=-5):
        self.message("Error: " + text, unimportance, ('bold', 'red'))

    def message(self, text, unimportance=1, color=None):
        if self._outputTest(unimportance):
            self.color.write("\n")
            bullet = '*'
            for line in text.split("\n"):
                self.color.write(" %s " % bullet, ('bold',))
                self.color.write(line, color)
                self.color.write("\n")
                bullet = ' '
            self.color.write("\n")

### The End ###
        
    
