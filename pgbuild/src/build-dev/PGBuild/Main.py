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

import optik
import PGBuild
import os, re, sys
import xml.dom.minidom

exitStatus = 0

class OptionParser(optik.OptionParser):
    def __init__(self):
        optik.OptionParser.__init__(self, formatter=HelpFormatter(),
                                    usage="%prog [options] [targets] ...",
                                    version=PGBuild.version,
                                    option_class=Option)
        
        # Override Optik's default options (for consistent grammar and capitalization)
        
        optik.STD_HELP_OPTION.help    = "Shows this help message and exits."
        optik.STD_VERSION_OPTION.help = "Shows the version number and exits."

        # Please try to follow the following conventions when adding command line options:
        #
        #   1. Most options should be in a group. The built-in --help and --version won't be,
        #      so any options that follow a similar pattern should not be in a group.
        #
        #   2. All options must have a help string, beginning with a verb in the Simple Present
        #      tense and ending in a period.
        #
        #   3. All destinations must begin with a lowercase letter and capitalize the initial
        #      letter of subsequent words.
        #
        #   4. On options with the default action of "store", a sensible metavar should be
        #      provided, in all capitals.
        #
        #   5. Options and groups should be listed in alphabetical order. If you want
        #      options to be listed together, that's what groups are for.
        #

        self.add_option("--platform", action="platform",
                        help="Shows the detected platform and exits.")
        self.add_option("--scons-version", action="sconsVersion",
                        help="Shows the version of SCons in use and exits.")

        ############# Build options
        
        buildGroup = self.add_option_group("Build Options")
        buildGroup.add_option("--ignore-errors", dest="ignoreErrors", action="store_true",
                              help="Ignores errors from build actions.")
        buildGroup.add_option("-j", "--jobs", dest="numJobs", default=1,
                              help="Sets the number of jobs that may run concurrently.") 
        buildGroup.add_option("-k", "--keep-going", dest="keepGoing", action="store_true",
                              help="Keeps going when a target can't be built.")
        buildGroup.add_option("-n", "--no-build", dest="noBuild", action="store_true",
                              help="Skips any attempt at building targets. Usually this is used " +
                              "along with options like --dump-tree when you want to inhibit the " +
                              "normal build process.")
        
        ############# Configuration management

        configGroup = self.add_option_group("Configuration Management")
        configGroup.add_option("--dump-tree", dest="treeDumpFile",
                               help="Dumps the configuration tree to FILE.", metavar="FILE")
        configGroup.add_option("-l", "--list", dest="listPath", metavar="XPATH",
                               help="Lists items from the given configuration path. " +
                               'If you\'re unfamilair with PGBuild\'s XPaths, try "sites", or "packages".')
        configGroup.add_option("--retest-mirrors", dest="retestMirrors", action="store_true",
                               help="Re-runs any mirror speed tests, ignoring saved results.")
        
        ############# Package management
        
        packageGroup = self.add_option_group("Package Management")
        packageGroup.add_option("--force-minisvn", dest="forceMiniSVN", action="store_true",
                                help="Forces the downloading of packages using the standalone " +
                                "MiniSVN Subversion client even if a full Subversion command " +
                                "line client is detected.")
        packageGroup.add_option("--nuke", dest="nuke", action="store_true",
                                help="Unconditionally deletes local copies of all non-bootstrap packages.")
        packageGroup.add_option("-m", "--merge", dest="merge", action="append", metavar="PACKAGE",
                                help="Explicitly merges configuration from the specified package.")
        packageGroup.add_option("--merge-all", dest="mergeAll", action="store_true",
                                help="Merges configuration from all available packages.")
        packageGroup.add_option("-u", "--update", dest="update", action="store_true", 
                                help="Updates the specified package and merges its configuration.")
        
        ############# Reporting options
        
        reportingGroup = self.add_option_group("Reporting Options")
        reportingGroup.add_option("-i", "--ui", action="store", dest="ui", metavar="MODULE",
                                  help="Selects a user interface module. Try --ui=help to list " +
                                  "the available modules.")
        reportingGroup.add_option("-q", "--quiet", action="uncount", dest="verbosity", default=1,
                                  help="Reports progress in less detail.")    
        reportingGroup.add_option("--traceback", action="store_true", dest="traceback",
                                  help="Disables the user-friendly exception handler and gives " +
                                  "a traceback when an error occurs.")
        reportingGroup.add_option("-v", "--verbose", action="count", dest="verbosity", default=1,
                                  help="Reports progress in more detail.")    

    def print_platform(self, file=None):
        # This mirrors the SCons built-in print_version and print_help
        if file is None:
            file = sys.stdout
        file.write("%s\n" % PGBuild.platform)

    def print_scons_version(self, file=None):
        # This mirrors the SCons built-in print_version and print_help
        if file is None:
            file = sys.stdout
        file.write("%s\n" % PGBuild.sconsVersion)


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

    ACTIONS = optik.Option.ACTIONS + ("uncount", "platform", "sconsVersion")
    STORE_ACTIONS = optik.Option.STORE_ACTIONS + ("uncount",)
    
    def take_action(self, action, dest, opt, value, values, parser):
        if action == "uncount":
            setattr(values, dest, values.ensure_value(dest, 0) - 1)
        elif action == "platform":
            # This mirrors the built-in SCons actions "version" and "help"
            parser.print_platform()
            sys.exit(0)
        elif action == "sconsVersion":
            # This mirrors the built-in SCons actions "version" and "help"
            parser.print_scons_version()
            sys.exit(0)
        else:
            optik.Option.take_action(self, action, dest, opt, value, values, parser)

    
class OptionsXML(xml.dom.minidom.Document):
    """Convert options from the supplied hash into XML, suitable
       for mounting into the configuration tree.
       """
    def __init__(self, parseResults):
        (self.options, self.args) = parseResults
        xml.dom.minidom.Document.__init__(self)
        
        pgbuild = self.createElement("pgbuild")
        pgbuild.setAttribute("title", "Command Line Options")
        pgbuild.setAttribute("root", "invocation")
        self.appendChild(pgbuild)

        for option in self.options.__dict__:
            value = getattr(self.options, option)
            if value != None:
                node = self.createElement("option")
                node.setAttribute("name", str(option))
                for child in self.marshall(value):
                    node.appendChild(child)
                pgbuild.appendChild(node)

        for i in xrange(len(self.args)):
            node = self.createElement("target")
            node.setAttribute("index", str(i))
            node.appendChild(self.createTextNode(self.args[i]))
            pgbuild.appendChild(node)

    def marshall(self, value):
        """Marshall an option value, return a list of DOM nodes.
           Initially I tried to use XML-RPC marshalling for this, but besides
           being far too verbose for this, it didn't fit in with PGBuild.Config's
           requirements for tag distinctness.
           """
        nodes = []
        if type(value) == list or type(value) == tuple:
            for i in xrange(len(value)):
                node = self.createElement("item")
                node.setAttribute("index", str(i))
                for child in self.marshall(value[i]):
                    node.appendChild(child)
                nodes.append(node)
        else:
            nodes.append(self.createTextNode(str(value)))
        return nodes


class BootstrapXML(xml.dom.minidom.Document):
    """An object that wraps a Bootstrap object, providing an XML document that
       can be mounted into the configuration tree.
       """
    def __init__(self, bootstrap):
        xml.dom.minidom.Document.__init__(self)
        pgbuild = self.createElement("pgbuild")
        pgbuild.setAttribute("title", "Bootstrap Configuration")
        pgbuild.setAttribute("root", "bootstrap")
        self.appendChild(pgbuild)
        
        for path in bootstrap.paths:
            node = self.createElement("path")
            node.setAttribute("name", path)
            node.appendChild(self.createTextNode(bootstrap.paths[path]))
            pgbuild.appendChild(node)

        for package in bootstrap.packages:
            node = self.createElement("package")
            node.setAttribute("name", package)
            node.appendChild(self.createTextNode(bootstrap.packages[package]))
            pgbuild.appendChild(node)


class PackageXML(xml.dom.minidom.Document):
    """An object that wraps a python package, providing an XML document that
       can be mounted into the configuration tree, representing all of its
       public string attributes.
       """
    def __init__(self, package, root):
        xml.dom.minidom.Document.__init__(self)
        pgbuild = self.createElement("pgbuild")
        pgbuild.setAttribute("title", "%s package attributes" % package.__name__)
        pgbuild.setAttribute("root", root)
        self.appendChild(pgbuild)
        
        for attr in dir(package):
            if attr[0] != '_':
                value = str(getattr(package, attr))
                if not (value[0] == '<' and value[-1] == '>'):
                    node = self.createElement("attr")
                    node.setAttribute("name", attr)
                    node.appendChild(self.createTextNode(value))
                    pgbuild.appendChild(node)


class Baton(object):
    """This is inspired by Subversion: The Baton object is used to store all
       context information that commonly needs to be sent to other
       functions. It is named as such since it gets passed around a lot.
       In the context of PGBuild, this is mainly for the config and progress
       objects, but is good for holding references to any such pseudo-global
       variables. This is better than actually using global variables, since
       it's cleaner and allows deviating from global-like behavior when
       necessary, as with the project object when starting a new task.
       """
    def modify(self, **kwargs):
        """Modify named attributes in bulk using keyword args"""
        self.__dict__.update(kwargs)
    
    def clone(self, **kwargs):
        """Make a shallow copy of the baton so that it may be modified
           for passing to subclasses. keyword args can be used to modify
           the cloned baton.
           """
        import copy
        miniMe = copy.copy(self)
        miniMe.modify(**kwargs)
        return miniMe

    def task(self, name):
        """Convenience function for starting a new task- clones the baton,
           and replaces the progress object with subtask progress object
           created using its task() function.
           """
        return self.clone(progress=self.progress.task(name))
    

def boot(ctx, bootstrap, argv):
    """Performs initial setup of PGBuild's configuration tree."""

    # Parse the args as soon as possible- this ensures that getting output
    # from --help doesn't require initializing the config tree.
    parsedArgs = OptionParser().parse_args(argv[1:])

    # Initialize the config tree. Import modules here to make sure the
    # option parser runs as soon as possible
    import PGBuild.Package
    import PGBuild.Config
    ctx.config = PGBuild.Config.Tree()

    # Mount in an XML representation of the bootstrap object
    ctx.config.mount(BootstrapXML(bootstrap))

    # Mount an XML representation of the PGBuild package's attributes, including
    # the description and version of this build system
    ctx.config.mount(PackageXML(PGBuild, "sys"))

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
                    import shutil
                    shutil.copyfile(os.path.join(skelPath, skelFile),
                                    os.path.join(bootstrap.paths['localConf'], skelFile))

    # Initialize a package list
    ctx.packages = PGBuild.Package.PackageList(ctx.config)

    # Read in configuration from the bootstrap packages
    # Alas, this has to be done in a separate step from actually merging the
    # bootstrap packages since their configuration is necessary for merging :)
    # We do the merge after the UI is set up, so that if the packages are updated
    # we can get progress reports.
    for package in bootstrap.packages.values():
        ctx.config.dirMount(ctx, os.path.join(bootstrap.paths['packages'], package))

    # Mount the local configuration directory
    ctx.config.dirMount(ctx, bootstrap.paths['localConf'])

    # Parse user options. This is only meaningful on UNIXes, but should fail
    # uneventfully on other platforms.
    ctx.config.dirMount(ctx, os.path.expanduser("~/.pgbuild"))
    
    # Mount command line options
    ctx.config.mount(OptionsXML(parsedArgs))


def main(bootstrap, argv):
    """The entry point called by build.py. Most of the work is done by run(),
       this just handles:

         - Initializing the config tree
         - Initializing the UI module
         - Exception catching
       """
    global exitStatus

    ctx = Baton()
    ctx.config = None
    ctx.ui = None
    # Outer try - cleanups
    try:
        # Middle try - UI exception handlers
        try:
            # Inner try - Exception rewriting
            try:

                # Set up the configuration tree and package list
                boot(ctx, bootstrap, argv)

                # Load a UI module and run it
                import PGBuild.UI
                PGBuild.UI.find(ctx.config.eval("invocation/option[@name='ui']/text()")).Interface(ctx)
                ctx.ui.run(ctx)

            except KeyboardInterrupt:
                import PGBuild.Errors
                raise PGBuild.Errors.InterruptError()
        except:
            # If we have a UI yet, try to let it handle the exception. Otherwise just reraise it.
            if ctx.ui:
                ctx.ui.exception(ctx, sys.exc_info())
                exitStatus = 2
            else:
                raise
    finally:
        if ctx.config:
            ctx.config.commit()
    sys.exit(exitStatus)

### The End ###
        
    
