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

import os, tarfile, urllib2, shutil
import PGBuild.Repository

class Repository(PGBuild.Repository.RepositoryBase):
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
        return os.path.isfile(self.getCompletenessFile(destination))

    def download(self, destination, progress):
        downloadComplete = 0
        try:
            # Extract the file as we download it
            urlFile = urllib2.urlopen(self.url)
            tar = tarfile.TarFile.open(None, "r|%s" % self.compression, urlFile)
            for file in tar:
                # Normally we'd just fall tar.extract(file, destination) here,
                # but we want to strip off the topmost directory from the tar file
                # path, since it's customary for everything in a tar file to be
                # inside a directory named similarly to the tar file itself.
                # This seems to be the easiest, though not cleanest, way to
                # extract a file to an arbitrary destination path.
                relativeName = os.sep.join(file.name.split(os.sep)[1:])
                try:
                    tar._extract_member(file, os.path.join(destination, relativeName))
                    progress.report("added", relativeName)
                except EnvironmentError:
                    progress.warning("EnvironmentError while extracting:\n%s" % relativeName, 2)
                except tarfile.ExtractError:
                    progress.warning("ExtractError while extracting:\n%s" % relativeName, 2)
            tar.close()
            urlFile.close()
            downloadComplete = 1
        finally:
            # If our download was terminated, make sure we don't incorrectly leave a completion flag
            self.setCompleteness(destination, downloadComplete)

    def isUpdateAvailable(self, destination):
        # Since it's assumed Tar reporitories are for finalized releases, the only time we would
        # need an update is if we don't yet have a complete download of the archive.
        return not self.getCompleteness(destination)
            
    def update(self, destination, progress):
        """Update the package if possible. Return 1 if there was an update available, 0 if not."""
        if self.isUpdateAvailable(destination):
            self.download(destination, progress)
            return 1
        return 0

### The End ###
