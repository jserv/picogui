""" PGBuild.Build

Manages the actual build process using SCons.
This includes facilities to construct an SCons environment using the
configuration database, to set up other SCons parameters, execute
SConscripts, and build targets.
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

# Acceptable names for an SCons script, in order of preference
scriptNames = ['SConscript', 'Sconscript', 'sconscript']

import SCons.Taskmaster
import SCons.Node
import sys


class BuildTask(SCons.Taskmaster.Task):
    """An SCons build task. Note that this is mostly copied from SCons.Script.BuildTask,
       but since the original code contained a lot of print'ing and sys.stdout.write,
       subclassing wouldn't have been effective.
       """

    # Our class will aquire a 'ctx' member here to store the current Baton.
    # Not too clean, but I don't feel like modifying SCons to
    # propagate the Baton properly :)

    def display(self, message):
        self.ctx.progress.messsage(message)

    def execute(self):
        target = self.targets[0]
        if target.get_state() == SCons.Node.up_to_date:
            if self.top and target.has_builder():
                self.display('"%s" is up to date.' % str(target))
        elif target.has_builder() and not hasattr(target.builder, 'status'):
            SCons.Taskmaster.Task.execute(self)

    def do_failed(self, status=2):
        if self.ctx.config.eval("invocation/option[@name='ignoreErrors']/text()"):
            SCons.Taskmaster.Task.executed(self)
        elif self.ctx.config.eval("invocation/option[@name='keepGoing']/text()"):
            SCons.Taskmaster.Task.fail_continue(self)
            import PGBuild.Main
            PGBuild.Main.exitStatus = status
        else:
            SCons.Taskmaster.Task.fail_stop(self)
            import PGBuild.Main
            PGBuild.Main.exitStatus = status
            
    def executed(self):
        t = self.targets[0]
        if self.top and not t.has_builder() and not t.side_effect:
            if not t.exists():
                s = "Do not know how to make target `%s'." % t
                self.ctx.progress.error(s)
                self.do_failed()
            else:
                self.ctx.progress.message("Nothing to be done for `%s'." % t)
                SCons.Taskmaster.Task.executed(self)
        else:
            SCons.Taskmaster.Task.executed(self)

    def failed(self):
        e = sys.exc_value
        status = 2
        if sys.exc_type == SCons.Errors.BuildError:
            self.ctx.progress.error("[%s] %s" % (e.node, e.errstr))
            if e.errstr == 'Exception':
                traceback.print_exception(e.args[0], e.args[1], e.args[2])
        elif sys.exc_type == SCons.Errors.UserError:
            # We aren't being called out of a user frame, so
            # don't try to walk the stack, just print the error.
            self.ctx.progress.error(e)
        elif sys.exc_type == SCons.Errors.StopError:
            s = str(e)
            self.ctx.progress.error(s)
        elif sys.exc_type == SCons.Errors.ExplicitExit:
            status = e.status
            self.ctx.progress.message("[%s] Explicit exit, status %s" % (e.node, e.status))
        else:
            if e is None:
                e = sys.exc_type
            self.ctx.progress.error(e)

        self.do_failed(status)


def loadScript(ctx, name):
    """Load one SCons script. Note that this doesn't set the current
       SCons directory- that is handled in loadScriptDir()
       """
    import SCons.Script
    ctx.progress.showTaskHeading()
    SCons.Script.SConscript.SConscript(name)
    ctx.progress.report("loaded", name)
    

def loadScriptDir(ctx, dir):
    """Look for a script in the given directory and run it if it's found"""
    import os
    import SCons.Node
    ctx.fs.chdir(dir)
    SCons.Node.FS.default_fs.set_SConstruct_dir(dir)
    for name in scriptNames:
        fObject = dir.File(name)
        if fObject.exists():
            loadScript(ctx, fObject)
            break


def Environment(ctx):
    """Factory to create an SCons environment from a PGBuild Baton"""
    import SCons.Environment
    env = SCons.Environment.Environment()
    env.ctx = ctx
    return env


class System(object):
    """Object encapsulating high-level control for the build system.
       This object should be created after the config tree has been
       bootstrapped and filled with command line options.
       """
    def __init__(self, ctx):
        import SCons.Node
        import SCons.Defaults
        import SCons.Script
        ctx.buildSystem = self
        self.defaultEnv = SCons.Defaults._default_env = Environment(ctx)
        self.processInvocationOptions(ctx)

    def processInvocationOptions(self, ctx):
        # Default task is to build targets
        self.task_class = BuildTask  
        self.invocationTargets = ctx.config.listEval('invocation/target/text()')

    def run(self, ctx):
        """Actually builds the targets we selected during initialization. Returns
           True if any targets were built.
           """
        import SCons.Node
        import SCons.Job
        import SCons.Taskmaster

        # Set default targets
        self.defaultTargets = SCons.Script.SConscript.default_targets

        # Determine a final list of targets to build
        if self.invocationTargets:
            self.targets = self.invocationTargets
        else:
            self.targets = self.defaultTargets
        
        # Convert our list of targets to nodes. The targets may be originally specified
        # as nodes, filenames, or aliases.
        target_top = None
        def Entry(x, top=target_top):
            if isinstance(x, SCons.Node.Node):
                node = x
            else:
                node = SCons.Node.Alias.default_ans.lookup(x)
                if node is None:
                    node = SCons.Node.FS.default_fs.Entry(x,
                                                          directory = top,
                                                          create = 1)
            if top and not node.is_under(top):
                if isinstance(node, SCons.Node.FS.Dir) and top.is_under(node):
                    node = top
                else:
                    node = None
            return node
        if self.targets:
            nodes = filter(lambda x: x is not None, map(Entry, self.targets))
        else:
            nodes = []
    
        if not nodes:
            return False

        # Set up the task class' Baton inside a subtask
        self.task_class.ctx = ctx.task("Building targets: " + ", ".join(map(str, nodes)))

        # Create a taskmaster using a list of target nodes
        taskmaster = SCons.Taskmaster.Taskmaster(nodes, self.task_class)

        # This sets up several job threads to run tasks concurrently
        jobs = SCons.Job.Jobs(ctx.config.intEval("invocation/option[@name='numJobs']/text()"), taskmaster)
        jobs.run()
        return True

### The End ###
        
    
