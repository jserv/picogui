""" PGBuild.Repository

Contains the Repository class that links a URI with a working directory,
with methods to determine if the working copy is up to date and update it.
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

import urlparse
import PGBuild.Errors

def open(url):
    """Repository factory- given a URL, this guesses what type of repository
       it points to and instantiates the proper Repository object for it.
       """
    (scheme, server, path, parameters, query, fragment) = urlparse.urlparse(url)

    # Anything ending in .tar, .tar.gz, or .tar.bz2 is a tar file
    if path[-7:] == ".tar.gz" or path[-8:] == ".tar.bz2" or path[-4:] == ".tar":
        import PGBuild.Repository.Tar
        return PGBuild.Repository.Tar.Repository(url)

    # Assume any remaining URLs with a http, https, or svn scheme are subversion repositories
    if scheme == "http" or scheme == "https" or scheme == "svn":
        import PGBuild.Repository.Subversion
        return PGBuild.Repository.Subversion.Repository(url)

    raise PGBuild.Errors.ConfigError("Unable to determine repository type for the URL '%s'" % url)


class RepositoryBase(object):
    """Base class for all Repository implementations"""
    
    def download(self, destination, progress):
        """Force the repository to be downloaded in its entirety to the given destination,
           outputting progress information to the given progress object.
           """
        pass
    
    def isUpdateAvailable(self, destination):
        """Check whether an update is available"""
        pass

    def update(self, destination, progress):
        """Update the local copy at the given destination if there's an update available,
           creating the destination directory if necessary. Progress is output to the given
           progress object. Returns nonzero if there was an update.
           """
        pass


### The End ###

