""" PGBuild.Version

Utilities for manipulating version numbers
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

import PGBuild.Errors


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
    """Compare two strings as version numbers"""
    return cmp(decomposeVersion(a), decomposeVersion(b))

        
class VersionSpec(object):
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
        # If we have no spec, match all versions
        if self.specString == None:
            return 1
        # We only support exact matches so far
        if version == self.specString:
            return 1
        else:
            return 0

    def matchList(self, list):
        """Return a sorted list all the matches in the given list"""
        matches = []
        for versionName in list:
            if self.match(versionName):
                matches.append(versionName)
        matches.sort(compareVersion)
        return matches
    

def scanIds(package):
    """Scans all our python source files for _svn_id lines of the form
       seen in this file above, returning a dictionary of Id strings for
       each filename. This doesn't import the file, since we want to
       scan its version even if it won't import (for example, it needs
       a module we don't have)
       """
    import os, re
    import PGBuild
    import PGBuild.Errors
    from xreadlines import xreadlines

    packagePath = package.__path__[0]
    moduleDict = {}
    svnlineRe = re.compile("^\s*_svn_id")

    # First, search for source files and fill moduleDict with
    # module name - module path pairs, without importing anything.
    def visit(arg, dirname, names):
        for name in names:
            # Ignore editor backups and such
            if name.endswith(".py") and not (name.startswith(".") or name.startswith("#")):
                modulePath = os.path.join(dirname, name)
                modPathList = modulePath[len(packagePath):].split(os.sep)
                # Strip off the .py extension, strip off __init__ modules from packages
                modPathList[-1] = modPathList[-1][:-3]
                if modPathList[-1] == "__init__":
                    del modPathList[-1]
                moduleName = package.__name__ + ".".join(modPathList)
                moduleDict[moduleName] = modulePath
    os.path.walk(packagePath, visit, None)

    # Open each module and search for the _svn_id line. The line must start with _svn_id,
    # but we do allow a little flexibility since we exec the line to get its value.
    for module in moduleDict:
        path = moduleDict[module]
        file = open(path)
        id = None
        for line in xreadlines(file):
            if svnlineRe.search(line):
                env = {'_svn_id': None}
                try:
                    exec line in env
                except:
                    raise PGBuild.Errors.InternalError("Malformed _svn_id found in module %s" % module)
                id = env['_svn_id']
                break
        file.close()
        if not id:
            raise PGBuild.Errors.InternalError("Module %s has no _svn_id" % module)
        moduleDict[module] = id
    return moduleDict


def findLatestRevision(idDict):
    """Given a module->id dictionary, returns the latest revision number"""
    latest = 0
    for module in idDict:
        id = idDict[module]

        # Id tag not expanded?
        if len(id)==4:
            raise PGBuild.Errors.InternalError(("Module %s has an unexpanded Id tag even " +
                                                "though the main package presumably has one. " +
                                                "There could be an error in your source checkout, " +
                                                "or a missing svn:keywords property on this module.") % module)
        idSplit = id.split(" ", 3)

        # Id tag filename doesn't match the module name?
        if (not idSplit[1].startswith("__")) and idSplit[1][:-3] != module.split(".")[-1]:
            raise PGBuild.Errors.InternalError(("Module %s has an Id tag with the wrong filename. " +
                                                "This probably means there is no svn:keywords tag " +
                                                "for that module, or that your current source isn't "
                                                "up to date.") % module)

        # Extract the revision
        rev = int(idSplit[2])
        if rev > latest:
            latest = rev
    return latest


def determineVersion():
    """This is called by PGBuild.__init__ to find out the current
       version number. If PGBuild.release is set, we'll use the release
       version. Otherwise we'll try to find the repository revision,
       and use that.
       """
    import PGBuild

    if PGBuild.release:
        return PGBuild.release

    # If this was an svn checkout we should have valid Id tags
    # and we can scan them to find the latest revision number.
    # If this was pulled straight out of WebDAV, for example by
    # the MiniSVN module, it won't have valid Id tags and we
    # will have to settle for a completely useless version number.
    if len(_svn_id) == 4:
        # An unexpanded id tag should be 4 characters. If it's
        # a different length, it's either valid or something's wrong.
        return "svn-unknown"

    # Scan the project's $Id$ tags and get the latest revision
    PGBuild._svn_id_dict = scanIds(PGBuild)
    PGBuild.revision = findLatestRevision(PGBuild._svn_id_dict)
    return "svn-r%d" % PGBuild.revision


### The End ###
        
    
