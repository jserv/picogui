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

import SCons.Defaults
import SCons.Script.SConscript
import SCons.Job
import SCons.Taskmaster
import PGBuild.Errors
import os

# Acceptable names for an SCons script, in order of preference
scriptNames = ['SConscript', 'Sconscript', 'sconscript']


def startup(config):
    """Initialize SCons defaults. This should be called
       after the config tree has been booted and filled with command line options.
       """
    SCons.Node.FS.default_fs.set_toplevel_dir(config.eval('bootstrap/path[@name="root"]/text()'))
    SCons.Defaults._default_env = Environment(config)

def run(config, progress):
    # Code to specify targets would go here
    targets = None
    if not targets:
        targets = SCons.Script.SConscript.default_targets
    
    # Convert our list of targets to nodes. The targets may be originally specified
    # as nodes, filenames, or aliases.
    target_top = SCons.Node.FS.default_fs.Entry("src/hello-dev")
    print target_top
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
    if targets:
        nodes = filter(lambda x: x is not None, map(Entry, targets))
    else:
        nodes = []

    if nodes:
        progress.message("Building targets:" + " ".join(map(lambda x:(" " + str(x)), nodes)))
    else:
        raise PGBuild.Errors.ExternalError("No targets to build")

    # Create a taskmaster using a list of target nodes
    taskmaster = SCons.Taskmaster.Taskmaster(nodes)

    # This sets up several job threads to run tasks concurrently
    jobs = SCons.Job.Jobs(int(config.eval("invocation/option[@name='numJobs']/text()")), taskmaster)
    jobs.run()

def loadScript(name, progress):
    """Load one SCons script"""
    progress.showTaskHeading()
    d = SCons.Node.FS.default_fs.File(name).dir
    SCons.Node.FS.default_fs.set_SConstruct_dir(d)
    SCons.Script.SConscript.SConscript(name)
    progress.report("loaded", name)

def loadScriptDir(dir, progress):
    """Look for a script in the given directory and run it if it's found"""
    for name in scriptNames:
        path = os.path.join(dir, name)
        if os.path.isfile(path):
            loadScript(path, progress)
            break

def Environment(config):
    """Factory to create an SCons environment from a PGBuild configuration"""
    env = SCons.Environment.Environment()
    env.config = config
    return env

### The End ###
        
    
