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
_svn_id = "$Id$"

# Check for a suitable version of python- currently we require version 2.2 or
# later, mostly for compatibility with the various modules we need.
# This doesn't use anything fancy to report a version problem, to increase
# the chance of this working correctly if we get a really old version of python.
import sys
if sys.hexversion < 0x020200F0:
    print "This version of Python is too old. At least verison 2.2 is required."
    sys.exit(1)

import PGBuild.UI
import PGBuild.Errors
import optik
import PGBuild
import os, re, shutil
import StringIO


class OptionParser(optik.OptionParser):
    def __init__(self):
        optik.OptionParser.__init__(self, formatter=HelpFormatter(),
                                    usage="%prog [options] [targets] ...",
                                    version=PGBuild.version,
                                    option_class=Option)
        
        # Override Optik's default options (for consistent grammar and capitalization)
        
        optik.STD_HELP_OPTION.help    = "Shows this help message and exits."
        optik.STD_VERSION_OPTION.help = "Shows the version number and exits."

        ############# General options
        
        self.add_option("-v", "--verbose", action="count", dest="verbosity", default=1,
                          help="Reports progress in more detail.")    
        self.add_option("-q", "--quiet", action="uncount", dest="verbosity", default=1,
                          help="Reports progress in less detail.")    
        self.add_option("-u", "--ui", action="store", dest="ui", metavar="MODULE",
                          help="Selects a front-end module. Try --ui=help to list the available modules.")
        self.add_option("--traceback", action="store_true", dest="traceback",
                          help="Disables the user-friendly exception handler and gives a traceback when an error occurs.")
         
        ############# Configuration management

        configGroup = self.add_option_group("Configuration Management")
        configGroup.add_option("--dump-tree", dest="treeDumpFile",
                               help="Dumps the configuration tree to FILE.", metavar="FILE")
        configGroup.add_option("--retest-mirrors", dest="retestMirrors", action="store_true",
                               help="Re-runs any mirror speed tests, ignoring saved results.")
        configGroup.add_option("-l", "--list", dest="listPath", metavar="XPATH",
                               help="Lists configuration data from the given path. " +
                               'If you don\'t know how to use PGBuild\'s XPaths, try ".", "sites", or "packages".')
        
        ############# Package management
        
        packageGroup = self.add_option_group("Package Management")
        packageGroup.add_option("--nuke", dest="nuke", action="store_true",
                                help="Unconditionally deletes local copies of all non-bootstrap packages.")
        packageGroup.add_option("-m", "--merge", dest="merge", action="append", metavar="PACKAGE",
                                help="Forcibly updates the specified package and merges its configuration.")
        

class HelpFormatter(optik.IndentedHelpFormatter):
    """Custom help formatting- provides some extra information about
       the program above the 'usage' line.
       """
    def __init__ (self,
                  indent_increment=3,
                  max_help_position=40,
                  width=80,
                  short_first=1):
        optik.IndentedHelpFormatter.__init__(
            self, indent_increment, max_help_position, width, short_first)
        
    def format_usage(self, usage):
        return "%s\n\nusage: %s\n" % (PGBuild.about, usage)


class Option(optik.Option):
    """Subclass optik's Option in order to add new action types"""

    ACTIONS = optik.Option.ACTIONS + ("uncount",)
    STORE_ACTIONS = optik.Option.STORE_ACTIONS + ("uncount",)
    
    def take_action(self, action, dest, opt, value, values, parser):
        if action == "uncount":
            setattr(values, dest, values.ensure_value(dest, 0) - 1)
        else:
            optik.Option.take_action(self, action, dest, opt, value, values, parser)

    
class OptionsXML(object):
    """Convert options from the supplied hash into XML, suitable
       for mounting into the configuration tree.
       """
    def __init__(self, parseResults):
        (self.options, self.args) = parseResults
        self.xml = StringIO.StringIO()
        self.xml.write('<pgbuild title="Command Line Options" root="invocation">')
        for option in self.options.__dict__:
            value = getattr(self.options, option)
            if value != None:
                self.xml.write('<option name="%s">' % option)
                self.marshall(value)
                self.xml.write('</option>')
        for arg in self.args:
            self.xml.write('<target name="%s">%s</target>' % (arg, self.args[arg]))
        self.xml.write('</pgbuild>')

    def marshall(self, value):
        """Marshall an option value, writing the resulting XML to self.xml.
           Initially I tried to use XML-RPC marshalling for this, but besides
           being far too verbose for this, it didn't fit in with PGBuild.Config's
           requirements for tag distinctness.
           """
        if type(value) == list or type(value) == tuple:
            for i in xrange(len(value)):
                self.xml.write('<item index="%s">' % i)
                self.marshall(value[i])
                self.xml.write('</item>')
        else:
            self.xml.write(str(value))

    def get_contents(self):        
        return self.xml.getvalue()


class BootstrapXML:
    """An object that wraps a Bootstrap object, providing an XML document that
       can be mounted into the configuration tree.
       """
    def __init__(self, bootstrap):
        self.xml = StringIO.StringIO()
        self.xml.write('<pgbuild title="Bootstrap Configuration" root="bootstrap">')
        for path in bootstrap.paths:
            self.xml.write('<path name="%s">%s</path>' % (path, bootstrap.paths[path]))
        for package in bootstrap.packages:
            self.xml.write('<package name="%s">%s</package>' % (package, bootstrap.packages[package]))
        self.xml.write('</pgbuild>')

    def get_contents(self):
        return self.xml.getvalue()


class PackageXML:
    """An object that wraps a python package, providing an XML document that
       can be mounted into the configuration tree, representing all of its
       public string attributes.
       """
    def __init__(self, package, root):
        self.xml = StringIO.StringIO()
        self.xml.write('<pgbuild title="%s package attributes" root="%s">' % (package.__name__, root))
        for attr in dir(package):
            if attr[0] != '_' and type(getattr(package,attr)) == str:
                self.xml.write('\t<attr name="%s">%s</attr>' % (attr, getattr(package,attr)))
        self.xml.write('</pgbuild>')

    def get_contents(self):
        return self.xml.getvalue()
        

def boot(bootstrap, argv):
    """Performs initial setup of PGBuild's configuration tree"""

    # Parse the args as soon as possible- this ensures that getting output
    # from --help doesn't require initializing the config tree.
    parsedArgs = OptionParser().parse_args(argv[1:])

    # Initialize the config tree. Import modules here to make sure the
    # option parser runs as soon as possible
    import PGBuild.Package
    import PGBuild.Config
    config = PGBuild.Config.Tree()

    # Mount in an XML representation of the bootstrap object
    config.mount(BootstrapXML(bootstrap))

    # Mount an XML representation of the PGBuild package's attributes, including
    # the description and version of this build system
    config.mount(PackageXML(PGBuild, "sys"))

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

    # Parse user options. This is only meaningful on UNIXes, but should fail
    # uneventfully on other platforms.
    config.dirMount(os.path.expanduser("~/.pgbuild"))
    
    # Mount command line options
    config.mount(OptionsXML(parsedArgs))
    return config


def main(bootstrap, argv):
    """The entry point called by build.py. Most of the work is done by run(),
       this just handles:

         - Initializing the config tree
         - Initializing the UI module
         - Exception catching
       """

    config = None
    ui = None
    # Outer try - cleanups
    try:
        # Middle try - UI exception handlers
        try:
            # Inner try - Exception rewriting
            try:
                
                # Set up the configuration tree
                config = boot(bootstrap, argv)

                # Load a UI module and run it
                ui = PGBuild.UI.find(config.eval("invocation/option[@name='ui']/text()")).Interface(config)
                ui.run()

            except KeyboardInterrupt:
                raise PGBuild.Errors.InterruptError()
        except:
            # If we have a UI yet, try to let it handle the exception. Otherwise just reraise it.
            if ui:
                ui.exception(sys.exc_info())
            else:
                raise
    finally:
        if config:
            config.commit()

### The End ###
        
    
