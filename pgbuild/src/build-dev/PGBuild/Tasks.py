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
    def __init__(self, ctx):
        self.ctx = ctx
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

class UserTask(Task):
    """A task that is designed to be triggered by user actions"""
    pass

class InternalTask(Task):
    """The opposite of a UserTask"""
    pass

class TaskList(object):
    def __init__(self, ctx, tasks=[]):
        ctx.taskList = self
        self.all = []
        self.pending = []
        self.completed = []
        self.inactive = []
        self.add(ctx, tasks)

    def add(self, ctx, tasks):
        if type(tasks) != type(()) and type(tasks) != type([]):
            tasks = [tasks]
        for task in tasks:
            # Allow passing classes rather than instances
            if callable(task):
                task = task(ctx)
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
        self.ctx.buildSystem = PGBuild.Build.System(self.ctx)

class MergeBootstrapTask(InternalTask):
    def execute(self):
        """Merge the bootstrap packages. This performs everything
           except the actual configuration mount, since we had to do that
           during PGBuild.Main.boot()
           """
        ctx = self.ctx.task("Merging bootstrap packages")
        for package in ctx.config.packages.getBootstrapPackages(ctx):
            ctx.config.packages.findPackageVersion(ctx, package).merge(ctx, False)

class NukeTask(UserTask):
    """Handle --nuke command line option"""
    def isActive(self):
        return self.ctx.config.eval("invocation/option[@name='nuke']/text()")

    def execute(self):
        self.ctx.config.packages.nuke(self.ctx)

class MergeAllTask(UserTask):
    """Handle --merge-all command line option"""
    def isActive(self):
        return self.ctx.config.eval("invocation/option[@name='mergeAll']/text()")

    def execute(self):
        mergeTaskCtx = self.ctx.task("Merging all packages")
        packages = self.ctx.config.listEval("packages/package/@name")
        packages.sort()
        for name in packages:
            self.ctx.packages.findPackageVersion(name).merge(mergeTaskCtx)

class MergeTask(UserTask):
    """Handle --merge command line option"""
    def init(self):
        self.mergeList = self.ctx.config.listEval("invocation/option[@name='merge']/item/text()")

    def isActive(self):
        return not not self.mergeList

    def execute(self):
        mergeTaskCtx = self.ctx.task("Merging user-specified packages")
        for name in self.mergeList:
            self.ctx.config.packages.findPackageVersion(self.ctx, name).merge(mergeTaskCtx)

class BuildSystemRunTask(UserTask):
    """Invoke the build system to build targets"""
    def isActive(self):
        return not self.ctx.config.eval("invocation/option[@name='noBuild']/text()")
    def execute(self):
        if not self.ctx.buildSystem.run(self.ctx):
            import PGBuild.Errors
            raise PGBuild.Errors.ExternalError("No targets to build")

class CleanupUITask(InternalTask):
    """Call the UI's cleanup() method to, if applicable, put us back in plain text mode"""
    def execute(self):
        self.ctx.ui.cleanup(self.ctx)

class ConsoleTask(UserTask):
    """Handle --console command line option"""
    def isActive(self):
        return not not self.ctx.config.eval("invocation/option[@name='console']/text()")

    def execute(self):
        import code, PGBuild
        try:
            import readline
        except ImportError:
            pass
        local = {
            'ctx': self.ctx,
            }
        code.interact(PGBuild.about, raw_input, local)

class DumpTreeTask(UserTask):
    """Handle --dump-tree command line option"""
    def init(self):
        self.dumpFile = self.ctx.config.eval("invocation/option[@name='treeDumpFile']/text()")

    def isActive(self):
        return not not self.dumpFile

    def execute(self):
        self.ctx.config.dump(self.ctx, self.dumpFiles)

class ListTask(UserTask):
    """Handle --list command line option"""
    def init(self):
        self.listPath = self.ctx.config.eval("invocation/option[@name='listPath']/text()")

    def isActive(self):
        return not not self.listPath

    def execute(self):
        self.ctx.ui.list(self.ctx, self.listPath)

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
    ConsoleTask,
    DumpTreeTask,
    ListTask,
    ]

### The End ###
        
    
