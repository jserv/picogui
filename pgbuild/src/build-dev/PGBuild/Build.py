""" PGBuild.Build

Manages the actual build process using SCons.
This includes facilities to construct an SCons environment using the
configuration database, to set up other SCons parameters, execute
SConscripts, and build targets. Parts of this code have been adapted
from SCons' Script module.
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
import sys

# FIXME: move these to invocation options
keep_going_on_error = False
ignore_errors = False
exit_status = 0

class BuildTask(SCons.Taskmaster.Task):
    """An SCons build task."""
    def display(self, message):
        display('scons: ' + message)

    def execute(self):
        target = self.targets[0]
        if target.get_state() == SCons.Node.up_to_date:
            if self.top and target.has_builder():
                display('scons: "%s" is up to date.' % str(target))
        elif target.has_builder() and not hasattr(target.builder, 'status'):
            if print_time:
                start_time = time.time()
            SCons.Taskmaster.Task.execute(self)
            if print_time:
                finish_time = time.time()
                global command_time
                command_time = command_time+finish_time-start_time
                print "Command execution time: %f seconds"%(finish_time-start_time)

    def do_failed(self, status=2):
        global exit_status
        if ignore_errors:
            SCons.Taskmaster.Task.executed(self)
        elif keep_going_on_error:
            SCons.Taskmaster.Task.fail_continue(self)
            exit_status = status
        else:
            SCons.Taskmaster.Task.fail_stop(self)
            exit_status = status
            
    def executed(self):
        t = self.targets[0]
        if self.top and not t.has_builder() and not t.side_effect:
            if not t.exists():
                sys.stderr.write("scons: *** Do not know how to make target `%s'." % t)
                if not keep_going_on_error:
                    sys.stderr.write("  Stop.")
                sys.stderr.write("\n")
                self.do_failed()
            else:
                print "scons: Nothing to be done for `%s'." % t
                SCons.Taskmaster.Task.executed(self)
        else:
            SCons.Taskmaster.Task.executed(self)

        # print the tree here instead of in execute() because
        # this method is serialized, but execute isn't:
        if print_tree and self.top:
            print
            print SCons.Util.render_tree(self.targets[0], get_all_children)
        if print_dtree and self.top:
            print
            print SCons.Util.render_tree(self.targets[0], get_derived_children)
        if print_includes and self.top:
            t = self.targets[0]
            tree = t.render_include_tree()
            if tree:
                print
                print tree

    def failed(self):
        e = sys.exc_value
        status = 2
        if sys.exc_type == SCons.Errors.BuildError:
            sys.stderr.write("scons: *** [%s] %s\n" % (e.node, e.errstr))
            if e.errstr == 'Exception':
                traceback.print_exception(e.args[0], e.args[1], e.args[2])
        elif sys.exc_type == SCons.Errors.UserError:
            # We aren't being called out of a user frame, so
            # don't try to walk the stack, just print the error.
            sys.stderr.write("\nscons: *** %s\n" % e)
        elif sys.exc_type == SCons.Errors.StopError:
            s = str(e)
            if not keep_going_on_error:
                s = s + '  Stop.'
            sys.stderr.write("scons: *** %s\n" % s)
        elif sys.exc_type == SCons.Errors.ExplicitExit:
            status = e.status
            sys.stderr.write("scons: *** [%s] Explicit exit, status %s\n" % (e.node, e.status))
        else:
            if e is None:
                e = sys.exc_type
            sys.stderr.write("scons: *** %s\n" % e)

        self.do_failed(status)

def loadScript(name, progress):
    """Load one SCons script"""
    progress.showTaskHeading()
    d = SCons.Node.FS.default_fs.File(name).dir
    SCons.Node.FS.default_fs.set_SConstruct_dir(d)
    SCons.Script.SConscript.SConscript(name)
    progress.report("loaded", name)

def loadScriptDir(dir, progress):
    """Look for a script in the given directory and run it if it's found"""
    import os
    for name in scriptNames:
        path = os.path.join(dir, name)
        if os.path.isfile(path):
            loadScript(path, progress)
            break

def Environment(config):
    """Factory to create an SCons environment from a PGBuild configuration"""
    import SCons.Environment
    env = SCons.Environment.Environment()
    env.config = config
    return env

class System(object):
    """Object encapsulating high-level control for the build system.
       This object should be created after the config tree has been
       bootstrapped and filled with command line options.
       """

    def __init__(self, config):
        import SCons.Node
        import SCons.Defaults
        import SCons.Script
        self.config = config
        SCons.Node.FS.default_fs.set_toplevel_dir(config.eval('bootstrap/path[@name="root"]/text()'))
        self.defaultEnv = SCons.Defaults._default_env = Environment(config)
        self.processInvocationOptions()

    def processInvocationOptions(self):
        # Default task is to build targets
        self.task_class = BuildTask
        
        self.invocationTargets = self.config.listEval('invocation/target/text()')

    def run(self, progress):
        import SCons.Node
        import SCons.Job

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
    
        if nodes:
            progress.message("Building targets: " + ", ".join(map(str, nodes)))
        else:
            import PGBuild.Errors
            raise PGBuild.Errors.ExternalError("No targets to build")

        # Create a taskmaster using a list of target nodes
        taskmaster = SCons.Taskmaster.Taskmaster(nodes, self.task_class)

        # This sets up several job threads to run tasks concurrently
        jobs = SCons.Job.Jobs(self.config.intEval("invocation/option[@name='numJobs']/text()"), taskmaster)
        jobs.run()

### The End ###
        
    
