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

class PackageVersion(object):
    """A single version of a package, representing a local copy and a repository.
       Supports updating the local copy from the repository, and performing builds
       on the local copy.
       """

    def __init__(self, package, configNode, appendPaths=[]):
        """package is the Package instance this belongs to,
           configNode is the DOM node for our <version> tag,
           appendPaths is a list of paths that should be appended to our URI,
             as specified in <versiongroup> tags.
           """
        self.package = package
        self.configNode = configNode
        self.name = configNode.attributes['name'].value
        self.repository = None
        self.appendPaths = appendPaths

    def __str__(self):
        """Returns a name of the form packagename-version"""
        return "%s-%s" % (self.package.name, self.name)

    def findMirror(self, ctx):
        """Find the fastest mirror for this package version. Returns a PGBuild.Site.Location"""
        ctx = ctx.task("Finding the fastest mirror for %s" % self)
        import PGBuild.Site
        return PGBuild.Site.resolve(ctx, self.configNode.getElementsByTagName('a'), self.appendPaths)

    def getRepository(self, ctx):
        """Get a Repository class for the fastest mirror of this package version"""
        if not self.repository:
            import PGBuild.Repository
            self.repository = PGBuild.Repository.open(ctx, self.findMirror(ctx).absoluteURI)
        return self.repository

    def getPathName(self):
        """Convert the package name to a path name suitable for the local OS. This is
           necessary because in configuration all packages use '/' as a separator,
           regardless of the local OS' separator.
           """
        return os.sep.join(str(self).split("/"))

    def getLocalPath(self, ctx):
        """Get the local path for this package. This links the source and binary directories."""
        local = ctx.paths['packages'].Dir(self.getPathName())
        self.getBinaryPath(ctx).link(local, False)
        return local

    def getBinaryPath(self, ctx):
        """Get the binary path for this package"""
        return ctx.paths['bin'].Dir(str(self.package.getHostPlatform(ctx))).Dir(self.getPathName())

    def update(self, ctx):
       """Update the package if possible. Return 1 if there was an update available, 0 if not."""
       localPath = self.getLocalPath(ctx).abspath
       ctx = ctx.task("Checking for updates in package %s" % self)
       repo = self.getRepository(ctx)

       # Since this could involve a lot of slow network activity, show our task heading now
       ctx.progress.showTaskHeading()

       # If we're updating a bootstrap package we have to do a little dance
       # so that our build system doesn't get hosed if the update fails.
       if str(self) in ctx.packages.getBootstrapPackages(ctx):
           splitLocalPath = os.path.split(localPath)
           tempPathNew = os.path.join(splitLocalPath[0], "temp_new_" + splitLocalPath[1])
           tempPathOld = os.path.join(splitLocalPath[0], "temp_old_" + splitLocalPath[1])

           # Since updating a bootstrap package is a big deal, check for updates first
           if repo.isUpdateAvailable(localPath):
               ctx.progress.message("Updating bootstrap package %s" % self)
               isUpdated = repo.update(ctx, tempPathNew)
               if isUpdated:
                   if os.path.isdir(tempPathOld):
                       # An old temp directory is in the way
                       try:
                           shutil.rmtree(tempPathOld)
                       except OSError:
                           ctx.progress.error(("There is an old temporary directory for package %s in the way.\n" +
                                               "Please try to remove %s") % (self, tempPathOld))
                   try:
                       os.rename(localPath, tempPathOld)
                       os.rename(tempPathNew, localPath)
                   except OSError:
                       import PGBuild.Errors
                       raise PGBuild.Errors.EnvironmentError(
                           ("There was a problem renaming the %s package to install an update.\n" +
                            "This will happen on Windows systems if you have a file open in that package.") % self)
                   try:
                       shutil.rmtree(tempPathOld)
                   except OSError:
                       ctx.progress.warning(("There was a problem removing the old version of %s after " +
                                             "upgrading.\nPlease try to remove the directory %s") %
                                            (self, tempPathOld))
           else:
               isUpdated = False
       else:

           # Not a bootstrap package, normal update
           isUpdated = repo.update(ctx, localPath)
       return isUpdated

    def merge(self, ctx, performMount=True):
        """Make sure a package is up to date, then load in configuration and build targets from it
           To handle bootstrap packages more efficiently, the actual config mounting can be disabled.
           """
        ctx = ctx.task("Merging configuration from package %s" % self)

        # We only update a package if there's no local copy, or if the --update option was
        # specified. This avoids having to wait on a lot of network traffic for every single invocation.
        if (not self.getRepository(ctx).isLocalCopyValid(ctx, self.getLocalPath(ctx).abspath)) or \
               ctx.config.eval("invocation/option[@name='update']/text()"):
            self.update(ctx)
        if performMount:
            ctx.config.dirMount(ctx.task("Mounting config files"), self.getLocalPath(ctx))
        import PGBuild.Build
        PGBuild.Build.loadScriptDir(ctx.task("Loading SCons scripts"), self.getLocalPath(ctx))

    def removeLocalCopy(self, ctx):
        """Deletes the local copy if there is one"""
        pkgPath = self.getLocalPath(ctx).abspath
        shutil.rmtree(pkgPath)

        # Remove as many empty directories above the package as we can
        splitPath = pkgPath.split(os.sep)
        try:
            while True:
                del splitPath[-1]
                dirPath = os.sep.join(splitPath)
                os.rmdir(dirPath)
                ctx.progress.report("removed", "empty directory %s" % dirPath)
        except OSError:
            pass

        
class Package(object):
    """A package object, initialized from the configuration tree.
       Holds details common to all package versions.
       """    
    def __init__(self, ctx, name):
        self.name = name
        
        # Save the root of the package configuration
        self.configNode = ctx.config.xpath('packages/package[@name="%s"]' % self.name)
        if len(self.configNode) > 1:
            import PGBuild.Errors
            raise PGBuild.Errors.ConfigError("More than one package with the name '%s'" % self.name)
        if len(self.configNode) == 0:
            import PGBuild.Errors
            raise PGBuild.Errors.ConfigError("Can't find a package with the name '%s'" % self.name)
        self.configNode = self.configNode[0]

        self.versions = {}
        self._loadVersions(ctx, self.configNode)

    def _loadVersions(self, ctx, node, paths=[]):
        """Load <version> tags from the given DOM node, recursively loading <versiongroup>s"""
        for versionNode in node.getElementsByTagName('version'):
            self.versions[versionNode.attributes['name'].value] = PackageVersion(self, versionNode, paths)
        for groupNode in node.getElementsByTagName('versiongroup'):
            try:
                groupName = groupNode.attributes['name'].value
            except KeyError:
                import PGBuild.Errors
                raise PGBuild.Errors.ConfigError("Found a <versiongroup> tag with no 'name' attribute in package %s" % self.name)
            try:
                groupPath = [groupNode.attributes['path'].value]
            except KeyError:
                groupPath = []
            groupNode = ctx.config.xpath('versiongroups/versiongroup[@name="%s"]' % groupName)
            if len(groupNode) > 1:
                import PGBuild.Errors
                raise PGBuild.Errors.ConfigError("More than one version group with the name '%s'" % groupName)
            if len(groupNode) == 0:
                import PGBuild.Errors
                raise PGBuild.Errors.ConfigError("Can't find a version group with the name '%s'" % groupName)
            self._loadVersions(ctx, groupNode[0], paths + groupPath)

    def findVersion(self, version=None):
        """Find a particular version of thie package. If the given version
           isn't already a VersionSpec it is converted to one.
           """
        import PGBuild.Version
        if not isinstance(version, PGBuild.Version.VersionSpec):
            version = PGBuild.Version.VersionSpec(version)

        # Find all nodes that the VersionSpec matches-
        # If there is more than one match, use the last one (they're sorted
        # by a VersionSpec-defined quality factor).
        matches = version.matchList(self.versions.keys())
        if len(matches) == 0:
            import PGBuild.Errors
            raise PGBuild.Errors.ConfigError(
                "Can't find version '%s' of package '%s'" % (version, self.name))
        return self.versions[matches[-1]]

    def getHostPlatform(self, ctx):
        """Find the platform this package will be built to run on. This will first
           try the package's host platform, falling back on the default host.
           """
        import PGBuild.Platform
        return PGBuild.Platform.parse(ctx, ctx.config.eval('hostPlatform',
                                                           self.configNode) or "host")


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
    def __init__(self, ctx):
        ctx.packages = self
        self.packages = {}

    def findPackage(self, ctx, name, version=None):
        """The name given here may or may not contain a version. Either way,
           if a version is specified in the 'version' parameter it overrides
           any version from the 'name'.
           """
        (name, nameVersion) = splitPackageName(name)
        if version == None:
            version = nameVersion

        if not self.packages.has_key(name):
            self.packages[name] = Package(ctx, name)
        package = self.packages[name]

        if version == None:
            return package
        else:
            return package.findVersion(version)

    def findPackageVersion(self, ctx, name, version=None):
        """Like findPackage(), but if no version was specified, return the latest"""
        pkg = self.findPackage(ctx, name, version)
        if isinstance(pkg, PackageVersion):
            return pkg
        else:
            return pkg.findVersion()

    def isPackage(self, ctx, name, version=None):
        """Test whether the given package (with or without version) exists in the configuration"""
        try:
            self.findPackageVersion(ctx, name, version)
            return 1
        except:
            return 0

    def getLocalPackages(self, ctx):
        """Retrieve a list of all packages with local copies"""
        # Every directory in our package path is potentially a package-
        # check them against the config's pacage list using isPackage.
        pkgDir = ctx.paths['packages'].abspath
        pkgList = []
        def visit(arg, dirname, names):
           # Chop off the leading directory and convert from the OS's separator back to forward slashes
           pkgName = dirname[len(pkgDir)+1:]
           pkgName = "/".join(pkgName.split(os.sep))
           if self.isPackage(ctx, pkgName):
              pkgList.append(pkgName)
              # If this is a package, don't descend into subdirectories
              while names:
                 del names[0]
        os.path.walk(pkgDir, visit, None)
        return pkgList

    def getBootstrapPackages(self, ctx):
        """Retrieve a list of all packages mentioned in the <bootstrap> section.
           These packages are essential for PGBuild's operation and should not be deleted.
           """
        return ctx.config.eval("bootstrap/package/text()")

    def nuke(self, ctx):
        """Delete local copies of all non-bootstrap packages"""
        ctx = ctx.task("Deleting local copies of all non-bootstrap packages")
        locals = self.getLocalPackages(ctx)
        boots  = self.getBootstrapPackages(ctx)
        removedPackages = 0
        for package in locals:
            if not package in boots:
                ctx.progress.showTaskHeading()
                self.findPackageVersion(ctx, package).removeLocalCopy(ctx)
                ctx.progress.report("removed", "package %s" % package)
                removedPackages += 1
        if not removedPackages:
            ctx.progress.message("No packages to remove")
    
### The End ###
        
    
