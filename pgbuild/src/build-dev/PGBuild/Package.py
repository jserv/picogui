""" PGBuild.Package

Objects to support package manipulation.
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

import os, shutil
import PGBuild.Errors
import PGBuild.Site
import PGBuild.Repository
import PGBuild.Version

class PackageVersion(object):
    """A single version of a package, representing a local copy and a repository.
       Supports updating the local copy from the repository, and performing builds
       on the local copy.
       """

    def __init__(self, package, configNode):
        self.config = package.config
        self.package = package
        self.configNode = configNode
        self.name = configNode.attributes['name'].value
        self.repository = None

        # The local path for this package
        #self.path = os.path.join(config.eval('bootstrap/path[@name="packages"]/text()'), self.name)

    def __str__(self):
        """Returns a name of the form packagename-version"""
        return "%s-%s" % (self.package.name, self.name)

    def findMirror(self, progress):
        """Find the fastest mirror for this package version. Returns a PGBuild.Site.Location"""
        task = progress.task("Finding the fastest mirror for %s" % self)
        return PGBuild.Site.resolve(self.config, self.configNode.getElementsByTagName('a'), task)

    def getRepository(self, progress):
        """Get a Repository class for the fastest mirror of this package version"""
        if not self.repository:
            self.repository = PGBuild.Repository.open(self.findMirror(progress).absoluteURI)
        return self.repository

    def getLocalPath(self):
        """Using the current bootstrap configuration, get the local path for this package"""
        name = str(self)
        # The name may contain slashes to indicate directories- the package name always
        # uses forward slashes, here we convert them to the local OS's format
        name = os.sep.join(name.split("/"))
        return os.path.join(self.config.eval('bootstrap/path[@name="packages"]/text()'), name)

    def update(self, progress):
       """Update the package if possible. Return 1 if there was an update available, 0 if not."""
       return self.getRepository(progress).update(self.getLocalPath(), progress.task("Updating package %s" % self))

    def merge(self, progress):
        """Make sure a package is up to date, then mount its configuration into our config tree"""
        mergeTask = progress.task("Merging configuration from package %s" % self)
        self.update(mergeTask)

        # Report the individual mounts in an unimportant task, so we only see them with -v
        confTask = mergeTask.task("Mounting config files", 1)
        self.config.dirMount(self.getLocalPath(), confTask)
        
        
class Package(object):
    """A package object, initialized from the configuration tree.
       Holds details common to all package versions.
       """
    
    def __init__(self, config, name):
        self.config = config
        self.name = name
        
        # Save the root of the package configuration
        self.configNode = config.xpath('packages/package[@name="%s"]' % self.name)
        if len(self.configNode) > 1:
            raise PGBuild.Errors.ConfigError("More than one package with the name '%s'" % self.name)
        if len(self.configNode) == 0:
            raise PGBuild.Errors.ConfigError("Can't find a package with the name '%s'" % self.name)
        self.configNode = self.configNode[0]

        # Load each version of this package
        self.versions = {}
        versionNodes = self.configNode.getElementsByTagName('version')
        for versionNode in versionNodes:
            self.versions[versionNode.attributes['name'].value] = PackageVersion(self, versionNode)

    def findVersion(self, version=None):
        """Find a particular version of thie package. If the given version
           isn't already a VersionSpec it is converted to one.
           """
        if not isinstance(version, PGBuild.Version.VersionSpec):
            version = PGBuild.Version.VersionSpec(version)

        # Find all nodes that the VersionSpec matches-
        # If there is more than one match, use the last one (they're sorted
        # by a VersionSpec-defined quality factor).
        matches = version.matchList(self.versions.keys())
        if len(matches) == 0:
            raise PGBuild.Errors.ConfigError(
                "Can't find version '%s' of package '%s'" % (version, self.name))
        return self.versions[matches[-1]]


def splitPackageName(name):
    """Split a package name into a (name,version) tuple. If there
       is no version specified, version will be None."""
    l = name.split("-",1)
    if len(l) < 2:
        l.append(None)
    return l


class PackageList(object):
    """Represents all the packages specified in a given configuration tree,
       and performs lookups on packages and package versions.
       """
    def __init__(self, config):
        self.packages = {}
        self.config = config

    def findPackage(self, name, version=None):
        """The name given here may or may not contain a version. Either way,
           if a version is specified in the 'version' parameter it overrides
           any version from the 'name'.
           """
        (name, nameVersion) = splitPackageName(name)
        if version == None:
            version = nameVersion

        if not self.packages.has_key(name):
            self.packages[name] = Package(self.config, name)
        package = self.packages[name]

        if version == None:
            return package
        else:
            return package.findVersion(version)

    def findPackageVersion(self, name, version=None):
        """Like findPackage(), but if no version was specified, return the latest"""
        pkg = self.findPackage(name, version)
        if isinstance(pkg, PackageVersion):
            return pkg
        else:
            return pkg.findVersion()

    def isPackage(self, name, version=None):
        """Test whether the given package (with or without version) exists in the configuration"""
        try:
            self.findPackageVersion(name, version)
            return 1
        except:
            return 0

    def getLocalPackages(self):
        """Retrieve a list of all packages with local copies"""
        # Every directory in our package path is potentially a package-
        # check them against the config's pacage list using isPackage.
        pkgDir = self.config.eval("bootstrap/path[@name='packages']/text()")
        pkgList = []
        def visit(arg, dirname, names):
           # Chop off the leading directory and convert from the OS's separator back to forward slashes
           pkgName = dirname[len(pkgDir)+1:]
           pkgName = "/".join(pkgName.split(os.sep))
           if self.isPackage(pkgName):
              pkgList.append(pkgName)
              # If this is a package, don't descend into subdirectories
              while names:
                 del names[0]
        os.path.walk(self.config.eval("bootstrap/path[@name='packages']/text()"), visit, None)
        return pkgList

    def getBootstrapPackages(self):
        """Retrieve a list of all packages mentioned in the <bootstrap> section.
           These packages are essential for PGBuild's operation and should not be deleted.
           """
        return self.config.eval("bootstrap/package/text()")

    def nuke(self, progress):
        """Delete local copies of all non-bootstrap packages"""
        task = progress.task("Deleting local copies of all non-bootstrap packages")
        locals = self.getLocalPackages()
        boots  = self.getBootstrapPackages()
        basePath = self.config.eval("bootstrap/path[@name='packages']/text()")
        for package in locals:
            if not package in boots:
                task.showTaskHeading()
                shutil.rmtree(os.path.join(basePath, package))
                task.report("removed", package)
    
### The End ###
        
    
