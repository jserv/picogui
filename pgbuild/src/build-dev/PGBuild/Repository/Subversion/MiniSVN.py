""" PGBuild.Repository.Subversion.MiniSVN

A minimalist Subversion client capable of doing atomic checkouts
using only the Python standard library and MiniDAV.
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

from urlparse import urlparse, urlunparse
from MiniDAV import DavObject
import os, re, time
import pickle
import PGBuild.Repository

class Repository(PGBuild.Repository.RepositoryBase):
    def __init__(self, config, url):
        self.config = config
        self.url = url
        self.root = None

    def connect(self):
        # The caller may pass a URL of the form "/repository/!svn/bc/<revision>/path"
        # to check out a particular revision. If they haven't already specified a
        # special path like this, find the latest revision number and use that.
        # This part is what makes checkouts atomic.
        url = self.url
        if url.find("!svn") < 0:
            # This part comes from guesstimation and looking at apache logs...
            # I couldn't find any docs on the WebDAV URLs used by Subversion, and
            # this was faster than digging through the source :)
            reposPath = DavObject(url).getProperties()['DAV::checked-in']
            reposPath = reposPath.replace("/!svn/ver/", "/!svn/bc/")
            parsed = urlparse(url)
            url = urlunparse((parsed[0], parsed[1], reposPath, '', '', ''))
        self.root = DavObject(url)

    def getPropertyFile(self, destination):
        """Get the properties filename associated with the given destination directory"""
        return os.path.join(destination, ".minisvn-properties")

    def saveProperties(self, destination):
        try:
            os.makedirs(destination)
        except OSError:
            pass
        propFile = open(self.getPropertyFile(destination), "w")
        pickle.dump(self.root.getProperties(), propFile)
        propFile.close()

    def getSavedProperties(self, destination):
        propFile = open(self.getPropertyFile(destination), "r")
        props = pickle.load(propFile)
        propFile.close()
        return props

    def download(self, destination, progress, numThreads=5):
        self.connect()
        downloadComplete = 0
        try:
            self.saveProperties(destination)

            # If this repository consists of just one file, go ahead and join that to
            # the destination path.
            if len(self.root.getChildren()) == 0:
                suffix = re.search("/([^/]+)$", self.root.path).group(1)
                destination = os.path.join(destination, suffix)

            queue = [(self.root, destination)]

            while len(queue) > 0:
                item = queue.pop()
                (object, objDest) = item
    
                if object.getType() == 'collection':
                    # This object is a directory- recursively create it
                    try:
                        os.makedirs(objDest)
                    except OSError:
                        # We don't care if it already exists
                        pass

                elif object.getType() == 'file':
                    # This object is a file- download it, creating the directory if necessary
                    try:
                        f = open(objDest, "wb")
                    except IOError:
                        os.makedirs(os.path.join(os.path.split(objDest)[:-1])[0])
                        f = open(objDest, "wb")                
                    f.write(object.read())
                    f.close()

                progress.report('added', objDest)
        
                for child in object.getChildren():
                    # Find the part of the child's URL that was (presumably)
                    # appended to this object's URL and append it to the destination
                    suffix = child.url[len(object.url):].replace("/","")
                    queue.append((child, os.path.join(objDest, suffix) ))

            downloadComplete = 1
        finally:
            if not downloadComplete:
                # If we were interrupted, delete the repository's properties file so
                # next time we have the option to do an update, we will.
                os.unlink(self.getPropertyFile(destination))

    def isUpdateAvailable(self, destination):
        self.connect()
        try:
            # DAV::version-name should have the latest revision that this subdirectory
            # or any of its children were modified in. This means that if we only have a
            # subdirectory of the repository checked out, we won't have to do an update
            # if some other part of it changes
            downloadedVersion = self.getSavedProperties(destination)['DAV::version-name']
            latestVersion = self.root.getProperties()['DAV::version-name']
            if downloadedVersion == latestVersion:
                return 0
        except IOError:
            # If the repository hasn't been downloaded at all yet, or our properties file
            # can't be read, force an update.
            pass
        return 1
            
    def update(self, destination, progress):
        """Update the package if possible. Return 1 if there was an update available, 0 if not."""
        self.connect()
        if self.isUpdateAvailable(destination):
            # We can't update, just redownload the sources.
            self.download(destination, progress)
            return 1
        return 0

### The End ###
