""" PGBuild.UI.None

Base classes for all UI modules, doesn't implement any UI at all.
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

class Progress:
    """Base class for progress reporting. This manages verbosity levels and other such
       tedium, passing on the actual message output to a subclass to handle.

       This class supports dividing the program's execution into a hierarchy of tasks,
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
            self.indentLevel = parent.indentLevel + 1
            self.root = parent.root
        else:
            self.indentLevel = 0
            self.root = self
            self.lastTask = self
        self._init()

    def _init(self):
        """Hook for extra initialization after the standard constructor"""
        pass

    def _showTaskHeading(self):
        """Hook for outputting a message when a task is started or changed"""
        pass

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
                self._showTaskHeading()
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

    def _report(self, verb, noun):
        """Hook for reporting progress, given an action and subject"""
        pass
    
    def report(self, verb, noun, unimportance=1):
        if self._outputTest(unimportance):
            self._report(verb, noun)

    def task(self, name, unimportance=0):
        """Create a new Progress object representing a hierarchial task"""
        return self.__class__(self.verbosityLevel - unimportance, self, name)

    def _warning(self, text):
        """Hook for displaying a warning message"""
        pass

    def warning(self, text, unimportance=1):
        if self._outputTest(unimportance):
            self._warning(text)

    def _error(self, text):
        """Hook for displaying an error message"""
        pass
            
    def error(self, text, unimportance=-5):
        if self._outputTest(unimportance):
            self._warning(text)

    def _message(self, text):
        """Hook for displaying a generic message"""
        pass

    def message(self, text, unimportance=1):
        if self._outputTest(unimportance):
            self._message(text)


class Interface:
    """Class responsible for driving PGBuild's execution based on user input
       and configuration settings. This base class only considers configuration
       settings, but it includes hooks for adding a UI.
       """
    progressClass = Progress
    
    def __init__(self, config):
        self.config = config

        # Set up a progress reporter object at the specified verbosity
        self.verbosity = int(config.eval("invocation/option[@name='verbosity']/text()"))
        self.progress = self.progressClass(self.verbosity)

        self.progress.message("Using UI module %s" % self.__class__.__module__, 2)

    def run(self):
        """Examine the provided configuration and take the specified actions"""

        import PGBuild.Site
        t = self.progress.task("Debuggative cruft")
        pkg = self.config.packages.findPackageVersion('picogui')
        pkg.merge(t)

        treeDumpFile = self.config.eval("invocation/option[@name='treeDumpFile']/text()")
        if treeDumpFile:
            f = open(treeDumpFile, "w")
            f.write(self.config.toprettyxml())
            f.close()

    def exception(self, exc_info):
        """This is called when PGBuild.Main catches an exception. The default implementation
           transparently passes it along, but this gives subclasses a chance to reformat
           the exception in a way that makes sense for a particular UI.
           """
        raise exc_info[0], exc_info[1], exc_info[2]

### The End ###
        
    