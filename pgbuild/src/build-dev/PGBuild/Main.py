""" PGBuild.Main

Frontend for PGBuild- provides command line processing and functions for
carrying out high-level tasks. This module's main() is invoked by build.py
as soon as it creates a Bootstrap object with vital path and package names.
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

# Check for a suitable version of python- currently we require version 2.2 or
# later, mostly for compatibility with the various modules we need.
# This doesn't use anything fancy to report a version problem, to increase
# the chance of this working correctly if we get a really old version of python.
import sys
if sys.hexversion < 0x020200F0:
    print "This version of Python is too old. At least verison 2.2 is required."
    sys.exit(1)

import PGBuild.Package
import PGBuild.UI
import PGBuild.CommandLine.Options
import PGBuild.Config
import os, re, shutil


def boot(config, bootstrap):
    """Initialize the configuration tree from a Bootstrap object- This takes
       care of mounting all the configuration files required to get us started,
       and stores the bootstrap object's information in the config tree.
       """

    class BootstrapXML:
        """An object that wraps a Bootstrap object, providing an XML document that
           can be mounted into the configuration tree.
           """
        def __init__(self, bootstrap):
            self.bootstrap = bootstrap

        def get_contents(self):
            xml = '<pgbuild title="Bootstrap Configuration" root="bootstrap">\n'                
            for path in self.bootstrap.paths:
                xml += '\t<path name="%s">%s</path>\n' % (path, self.bootstrap.paths[path])
            for package in self.bootstrap.packages:
                xml += '\t<package name="%s">%s</package>\n' % (package, self.bootstrap.packages[package])                    
            xml += '</pgbuild>\n'
            return xml

    # Mount in an XML representation of the bootstrap object
    config.mount(BootstrapXML(bootstrap))

    # Try to make sure all our bootstrap paths exist
    for path in bootstrap.paths.values():
        try:
            os.makedirs(path)
        except OSError:
            pass

    # Copy skeleton local files from the conf package if they haven't been
    # copied or manually created yet.
    skelPath = os.path.join(os.path.join(bootstrap.paths['packages'],
                                         bootstrap.packages['conf']), 'local')
    for skelFile in os.listdir(skelPath):
        if re.match(".*\.%s" % PGBuild.Config.configFileExtension, skelFile):
            if os.path.isfile(os.path.join(skelPath, skelFile)):
                if not os.path.isfile(os.path.join(bootstrap.paths['localConf'], skelFile)):
                    shutil.copyfile(os.path.join(skelPath, skelFile),
                                    os.path.join(bootstrap.paths['localConf'], skelFile))

    # Initialize a package list for this config tree
    config.packages = PGBuild.Package.PackageList(config)

    # Read in configuration from the bootstrap packages
    for package in bootstrap.packages.values():
        config.dirMount(os.path.join(bootstrap.paths['packages'], package))

    # Mount the local configuration directory
    config.dirMount(bootstrap.paths['localConf'])


def main(bootstrap, argv):
    """The entry point called by build.py. Most of the work is done by run(),
       this just handles:

         - Initializing the config tree
         - Initializing the UI module
         - Exception catching
       """

    config = PGBuild.Config.Tree()
    try:
        # Load the options passed to use by build.py into the <bootstrap> section
        boot(config, bootstrap)

        # Parse command line options into the <invocation> section
        PGBuild.CommandLine.Options.parse(config, argv)

        # Load a UI module and run it
        PGBuild.UI.find(config.eval("invocation/option[@name='gui']/text()")).Interface(config).run()
    finally:
        config.commit()

### The End ###
        
    
