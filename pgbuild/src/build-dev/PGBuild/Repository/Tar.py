""" PGBuild.Repository.Tar

Repository implementation for fetching and extracting Tar files.
This repository implementation does not provide for updating- it
is assumed that the tar files are not snapshots, if they're changed
there will be a new version.
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
from tarfile import TarFile
import urllib2, shutil

class Repository:
    def __init__(self, url):
        self.url = url

        # Detect the type of compression, based on the file extension
        if self.url[-3:] == ".gz":
            self.compression = "gz"
        elif self.url[-4:] == ".bz2":
            self.compression = "bz2"
        else:
            self.compression = ""

    def getCompletenessFile(self, destination):
        """Get the name of a hidden file we use to mark a download as complete"""
        return os.path.join(destination, ".pgb-complete-download")

    def setCompleteness(self, destination, value):
        if value:
            open(self.getCompletenessFile(destination), "w").close()
        else:
            try:
                os.unlink(self.getCompletenessFile(destination))
            except OSError:
                pass

    def getCompleteness(self, destination):
        return os.isfile(self.getCompletenessFile(destination))

    def download(self, destination, progress):
        downloadComplete = 0
        try:
            # Extract the file as we download it
            urlFile = urllib2.urlopen(self.url)
            tar = TarFile.open(None, "r|%s" % self.compression, urlFile)
            for file in tar:
                tar.extract(file, destination)
                progress.report("added", file.name)
            tar.close()
            urlFile.close()
            downloadComplete = 1
        finally:
            self.setCompleteness(destination, downloadComplete)

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
        self.connect()
        if self.isUpdateAvailable(destination):
            # We can't update, just redownload the sources.
            self.download(destination, progress)

### The End ###
