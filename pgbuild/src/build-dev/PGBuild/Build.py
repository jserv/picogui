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
        self.config = config

        # Initialize SCons
        import SCons.Node
        import SCons.Defaults
        import SCons.Script
        SCons.Node.FS.default_fs.set_toplevel_dir(config.eval('bootstrap/path[@name="root"]/text()'))
        self.defaultEnv = SCons.Defaults._default_env = Environment(config)

        # Set default targets
        self.targets = SCons.Script.SConscript.default_targets

        # If targets were specified on the command line, run those instead
        invocationTargets = config.listEval('invocation/target/text()')
        if invocationTargets:
            self.targets = invocationTargets

    def run(self, progress):
        import SCons.Node
        import SCons.Taskmaster
        import SCons.Job
        
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
            progress.message("Building targets:" + " ".join(map(lambda x:(" " + str(x)), nodes)))
        else:
            import PGBuild.Errors
            raise PGBuild.Errors.ExternalError("No targets to build")

        # Create a taskmaster using a list of target nodes
        taskmaster = SCons.Taskmaster.Taskmaster(nodes)

        # This sets up several job threads to run tasks concurrently
        jobs = SCons.Job.Jobs(int(self.config.eval("invocation/option[@name='numJobs']/text()")), taskmaster)
        jobs.run()

### The End ###
        
    
