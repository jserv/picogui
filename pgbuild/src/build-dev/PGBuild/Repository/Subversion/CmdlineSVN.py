""" PGBuild.Repository.Subversion.CmdlineSVN

A wrapper around the command line Subversion client providing
a Repository implementation.
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

import os, re, shutil

# The name of the svn command to use. This should be made customizable.
svnCommand = "svn"

import PGBuild.Errors
import PGBuild.Repository

def detectVersion():
    # Get the complete output from svn --version
    svn = os.popen("%s --version" % svnCommand)
    ver = svn.read()
    svn.close()

    # If the response doesn't contain "Subversion", assume something's wrong
    if ver.find("Subversion") < 0:
        raise PGBuild.Errors.EnvironmentError("The command line subversion client is broken or not installed")
    return ver

class ptyopen(object):
    """Helper class that emulates popen() to the extent we'll
       need below, using pseudoterminals
       """
    def __init__(self, cmdline):
        import pty
        (self.pid, self.fd) = pty.fork()
        if self.pid == 0:
            os.execlp("sh", "sh", "-c", cmdline)
        self.file = os.fdopen(self.fd, 'r', 1)   # Line buffered

    def readline(self):
        try:
            return self.file.readline()
        except IOError:
            return None

    def close(self):
        self.file.close()
        (pid, status) = os.waitpid(self.pid, 0)
        return status >> 8


def openSvn(args):
    global svnCommand
    cmdline = '%s --non-interactive %s' % (svnCommand, args)

    # Our first choice is to use a pseudoterminal. This gets us nice line-buffered
    # text rather than having it plop out in big chunks. If we have a problem with
    # ptys, fall back on popen.
    try:
        return ptyopen(cmdline)
    except:
        return os.popen(cmdline)

def expandStatus(line):
    if len(line) < 2:
        return None
    if line[1] != ' ':
        return None
    try:
        return {
            'A': 'added',
            'D': 'deleted',
            'U': 'updated',
            'C': 'conflict',
            'G': 'merged',
            }[line[0]]
    except KeyError:
        return None

def collectProgress(file, progress, destination):
    updatedFiles = 0
    allOutput = ""
    while 1:
        line = file.readline()
        if not line:
            break
        allOutput += line
        status = expandStatus(line)
        if status:
            path = os.path.normpath(line[2:].strip())
            # Try to strip off the destination path
            if path.startswith(destination):
                path = path[len(destination)+1:]
            progress.report(status, path)
            updatedFiles += 1
    if file.close():
        raise PGBuild.Errors.EnvironmentError("The Subversion command returned an error:\n\n%s" % allOutput)
    return updatedFiles

# Since exceptions during import will be used to autodetect which Subversion
# module to use, we want to cause an exception here if it looks like the
# command line client is missing or broken.
version = detectVersion()


class Repository(PGBuild.Repository.RepositoryBase):
    def __init__(self, url):
        self.url = url

    def download(self, destination, progress):
        finished = 0
        try:
            collectProgress(openSvn('co "%s" "%s"' % (self.url, destination)),
                            progress, destination)
            finished = 1
        finally:
            if not finished:
                # Remove the incomplete working copy.
                # This should be safe to do, since having called download() implies that
                # there wasn't a working copy to begin with, so there shouldn't be any
                # changes that this will nuke. If we don't do this, another attempt to
                # download() will fail, because svn can't currently resume checkouts.
                progress.warning("Removing partial Subversion checkout")
                try:
                    shutil.rmtree(destination)
                except:
                    progress.warning(("Couldn't remove the partial Subversion checkout!\n" +
                                      "Please try removing the directory %s") % destination)

    def isWorkingCopyPresent(self, destination):
        try:
            # Determine if the destination has a repository. This will fail if not.
            open(os.path.join(destination, os.path.join(".svn", "format"))).close()
            return 1
        except IOError:
            return 0
        
    def isUpdateAvailable(self, destination):
        if not self.isWorkingCopyPresent(destination):
            return 1

        svn = openSvn('status --show-updates "%s"' % destination)
        # If we have any lines beginning with (ignoring whitespace) an asterisk,
        # we need to update something.
        while 1:
            line = svn.readline()
            if not line:
                svn.close()
                return 0
            if line.lstrip()[0] == '*':
                svn.close()
                return 1
            
    def update(self, destination, progress):
        """Update the package if possible. Return 1 if there was an update available, 0 if not."""
        if self.isWorkingCopyPresent(destination):
            return collectProgress(openSvn('up "%s"' % destination), progress, destination) != 0
        else:
            # No working copy- do a complete download
            self.download(destination, progress)
            return 1

### The End ###
