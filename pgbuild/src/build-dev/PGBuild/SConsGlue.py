""" PGBuild.SConsGlue

Glue between SCons and PGBuild. This includes facilities to construct
an SCons environment using the configuration database, to set up other
SCons parameters, execute SConscripts, and build targets.
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
import os

# Acceptable names for an SCons script, in order of preference
scriptNames = ['SConscript', 'Sconscript', 'sconscript']


def startup(config):
    """Initialize SCons defaults. This should be called
       after the config tree has been booted and filled with command line options.
       """
    SCons.Node.FS.default_fs.set_toplevel_dir(config.eval('bootstrap/path[@name="root"]/text()'))
    SCons.Defaults._default_env = Environment(config)

def run(config):
    print SCons.Script.SConscript.default_targets
    taskmaster = SCons.Taskmaster.Taskmaster(nodes, task_class, calc, order)
    jobs = SCons.Job.Jobs(ssoptions.get('num_jobs'), taskmaster)
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
        
    
