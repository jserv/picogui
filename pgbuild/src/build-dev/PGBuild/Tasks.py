""" PGBuild.Tasks

PGBuild Tasks are separate from SCons tasks. These are used mainly
for coordinating the execution of command line options and GUI options
with the rest of the build process, hence this system is much simpler
than SCons' task system, which is mostly for building a hierarchy of
nodes correctly.
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

class Task(object):
    """Base class for PGBuild tasks"""
    def __init__(self, taskList):
        self.taskList = taskList
        self.config = taskList.config
        self.progress = taskList.progress
        self.ui = taskList.ui
        self.init()

    def init(self):
        """Extra initialization that the subclass can do without changing
           the list of parameters that __init__ expects. Particularly,
           caching config values.
           """
        pass
    
    def isActive(self):
        """This should return True if the task should be executed"""
        return True
    
    def execute(self):
        """Run the task- a return from this without an exception is
           assumed to mean the task was successful.
           """
        pass

    def preventAutoBuild(self):
        """Return true if this task should prevent automatically building
           the default targets if no explicit targets have been specified.
           """
        return False

class UserTask(Task):
    """A task that is designed to be triggered by user actions"""
    pass

class InternalTask(Task):
    """The opposite of a UserTask"""
    pass

class TaskList(object):
    def __init__(self, ui, tasks=[]):
        self.config = ui.config
        self.progress = ui.progress
        self.ui = ui
        self.all = []
        self.pending = []
        self.completed = []
        self.inactive = []
        self.add(tasks)

    def add(self, tasks):
        if type(tasks) != type(()) and type(tasks) != type([]):
            tasks = [tasks]
        for task in tasks:
            # Allow passing classes rather than instances
            if callable(task):
                task = task(self)
            self.all.append(task)
            
            # We pop elements off the end of the pending list
            # during execution, so to preserve it as a FIFO we
            # add elements at the beginning.
            if task.isActive():
                self.pending.insert(0, task)
            else:
                self.inactive.append(task)

    def run(self):
        while self.pending:
            task = self.pending.pop()
            task.execute()
            self.completed.append(task)

################### Tasks

class BuildSystemInitTask(InternalTask):
    """Invoke the build system to build targets"""
    def execute(self):
        import PGBuild.Build
        self.ui.buildSystem = PGBuild.Build.System(self.config)

class MergeBootstrapTask(InternalTask):
    def execute(self):
        """Merge the bootstrap packages. This performs everything
           except the actual configuration mount, since we had to do that
           during PGBuild.Main.boot()
           """
        bootMergeTask = self.progress.task("Merging bootstrap packages")
        for package in self.config.packages.getBootstrapPackages():
            self.config.packages.findPackageVersion(package).merge(bootMergeTask, False)

class NukeTask(UserTask):
    """Handle --nuke command line option"""
    def isActive(self):
        return self.config.eval("invocation/option[@name='nuke']/text()")

    def execute(self):
        self.config.packages.nuke(self.progress)

    def preventAutoBuild(self):
        return True

class MergeAllTask(UserTask):
    """Handle --merge-all command line option"""
    def isActive(self):
        return self.config.eval("invocation/option[@name='mergeAll']/text()")

    def execute(self):
        mergeTask = self.progress.task("Merging all packages")
        packages = self.config.listEval("packages/package/@name")
        packages.sort()
        for name in packages:
            self.config.packages.findPackageVersion(name).merge(mergeTask)

class MergeTask(UserTask):
    """Handle --merge command line option"""
    def init(self):
        self.mergeList = self.config.listEval("invocation/option[@name='merge']/item/text()")

    def isActive(self):
        return not not self.mergeList

    def execute(self):
        mergeTask = self.progress.task("Merging user-specified packages")
        for name in self.mergeList:
            self.config.packages.findPackageVersion(name).merge(mergeTask)

class BuildSystemRunTask(UserTask):
    """Invoke the build system to build targets"""
    def execute(self):
        # The build task is always active, but it only takes action if:
        #  ... the user explicitly specified targets
        #  ... or, all pending and completed tasks' preventAutoBuild() members return False
        # Note that this logic would generally go in isActive(), but since this depends
        # on having all other tasks already sorted into pending or inactive, we have to do
        # it here.
        enabled = True
        for task in self.taskList.pending + self.taskList.completed:
            if task.preventAutoBuild():
                enabled = False
        if self.config.listEval('invocation/target/text()'):
            enabled = True
        if enabled:
            if not self.ui.buildSystem.run(self.progress):
                import PGBuild.Errors
                raise PGBuild.Errors.ExternalError("No targets to build")

class CleanupUITask(InternalTask):
    """Call the UI's cleanup() method to, if applicable, put us back in plain text mode"""
    def execute(self):
        self.ui.cleanup()

class DumpTreeTask(UserTask):
    """Handle --dump-tree command line option"""
    def init(self):
        self.dumpFile = self.config.eval("invocation/option[@name='treeDumpFile']/text()")

    def isActive(self):
        return not not self.dumpFile

    def execute(self):
        self.config.dump(self.dumpFile, self.progress)

    def preventAutoBuild(self):
        return True

class ListTask(UserTask):
    """Handle --list command line option"""
    def init(self):
        self.listPath = self.config.eval("invocation/option[@name='listPath']/text()")

    def isActive(self):
        return not not self.listPath

    def execute(self):
        self.ui.list(self.listPath)

    def preventAutoBuild(self):
        return True

################### Primary task list
#
# Note that order is important here!
# It wouldn't make sense to run --nuke after --merge, for example,
# and tasks that output directly to stdout should be run after the UI
# cleanup task.
#

allTasks = [
    BuildSystemInitTask,
    MergeBootstrapTask,
    NukeTask,
    MergeAllTask,
    MergeTask,
    BuildSystemRunTask,
    CleanupUITask,
    DumpTreeTask,
    ListTask,
    ]

### The End ###
        
    
