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

import os
import PGBuild.Errors


class PackageVersion:
    """A single version of a package, representing a local copy and a repository.
       Supports updating the local copy from the repository, and performing builds
       on the local copy.
       """

    def __init__(self, package, configNode):
        self.package = package
        self.configNode = configNode

        # The local path for this package
        #self.path = os.path.join(config.eval('bootstrap/path[@name="packages"]/text()'), self.name)


def decomposeVersion(version):
    """Utility for version number manipulation- separate a version number into
       groups of numbers, punctuation, and letters.
       """
    groups = [""]

    def categorize(char):
        if char.isalpha():
            return 0
        if char.isdigit():
            return 1
        if char.isspace():
            return 2
        return 3
    
    for char in version:
        if len(groups[-1]) == 0 or categorize(char) == categorize(groups[-1][-1]):
            groups[-1] = groups[-1] + char
        else:
            groups.append(char)

    # Convert numeric parts of the version to numbers
    for i in xrange(len(groups)):
        try:
            groups[i] = int(groups[i])
        except ValueError:
            pass
    return groups
            

def compareVersion(a,b):
    """Try to compare two versions, determining which is newer"""
    a = decomposeVersion(a)
    b = decomposeVersion(b)
    if len(a) < len(b):
        return 1
    if len(a) > len(b):
        return -1
    for i in xrange(len(a)):
        if a[i] < b[i]:
            return 1
        if a[i] > b[i]:
            return -1
    return 0

        
class VersionSpec:
    """A specification for one or more versions of a package.

       FIXME: For now this only supports simple version names, but
              this is the place to add support for version lists and
              ranges.
    """
    def __init__(self, specString):
        self.specString = specString

    def __str__(self):
        return self.specString

    def match(self, version):
        """Return true if the given version name matches this spec"""
        if version == self.specString:
            return 1
        else:
            return 0

    def compareMatch(self, a, b):
        """Given two versions that match the spec, compare their quality.
           For now this just favors newer versions.
           """
        return compareVersion(a,b)

    def matchList(self, list):
        """Return all the matches in the given list, sorted using compareMatch"""
        matches = []
        for versionName in list:
            if self.match(versionName):
                matches.append(versionName)
        matches.sort(self.compareMatch)
        return matches
        
        
class Package:
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

    def findVersion(self, version):
        """Find a particular version of thie package. If the given version
           isn't already a VersionSpec it is converted to one.
           """
        if not isinstance(version, VersionSpec):
            version = VersionSpec(version)

        # Find all nodes that the VersionSpec matches-
        # If there is more than one match, use the last one (they're sorted
        # by a VersionSpec-defined quality factor).
        matches = version.matchList(self.versions.keys())
        if len(matches) == 0:
            raise PGBuild.Errors.ConfigError(
                "Can't find version '%s' of package '%s'" % (version, self.name))
        return self.versions[matches[-1]]

    def getVersionNode(self, packageNode):
        """Given a package node, return the version node that
           best matches our version spec."""
        packageName = packageNode.attributes['name'].value

        # Get a list of package versions that match according to self.match()
        matches = []
        for version in versions:
            if self.match(version.attributes['name'].value):
                matches.append(version)

        # FIXME: we don't support ambiguous version specs yet.
        #        Eventually there should be a good way to determine which version is 'better'
        #        and pick that one.
        if len(matches) > 1:
            raise PGBuild.Errors.ConfigError(
                "Ambiguous version specification '%s' for package '%s', not supported yet" %
                (self.specString, packageName))
        
        return matches[0]


def splitPackageName(name):
    """Split a package name into a (name,version) tuple. If there
       is no version specified, version will be None."""
    l = name.split("-",1)
    if len(l) < 2:
        l.append(None)
    return l


class PackageList:
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

### The End ###
        
    
