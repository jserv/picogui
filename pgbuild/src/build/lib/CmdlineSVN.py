#!/usr/bin/env python
"""
 CmdlineSVN.py - A wrapper around the command line Subversion client
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

import os, re

# The name of the svn command to use. This should be made customizable.
svnCommand = "svn"

class CmdlineSVNClientBroken(Exception):
    pass

class ErrorReturnCode(Exception):
    pass

def detectVersion():
    # Get the complete output from svn --version
    svn = os.popen("%s --version" % svnCommand)
    ver = svn.read()
    svn.close()

    # If the response doesn't contain "Subversion", assume something's wrong
    if ver.find("Subversion") < 0:
        raise CmdlineSVNClientBroken
    return ver

def openSvn(args):
    global svnCommand
    return os.popen('%s --non-interactive %s' % (svnCommand, args))

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

def collectProgress(file, progress):
    while 1:
        line = file.readline()
        if not line:
            break
        status = expandStatus(line)
        if status:
            progress.report(status, line[2:].strip())
    if file.close():
        raise ErrorReturnCode()

def rmtree(path, progress):
    if os.path.isdir(path):
        for file in os.listdir(path):
            rmtree(os.path.join(path, file), progress)
        os.rmdir(path)
    else:
        os.unlink(path)


# Since exceptions during import will be used to autodetect which Subversion
# module to use, we want to cause an exception here if it looks like the
# command line client is missing or broken.
version = detectVersion()


class SVNRepository:
    def __init__(self, url):
        self.url = url

    def download(self, destination, progress):
        finished = 0
        try:
            collectProgress(openSvn('co "%s" "%s"' % (self.url, destination)),progress)
            finished = 1
        finally:
            if not finished:
                # Remove the incomplete working copy.
                # This should be safe to do, since having called download() implies that
                # there wasn't a working copy to begin with, so there shouldn't be any
                # changes that this will nuke. If we don't do this, another attempt to
                # download() will fail, because svn can't currently resume checkouts.
                progress.warning("Removing partial Subversion checkout")
                rmtree(destination, progress)

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
        if self.isWorkingCopyPresent(destination):
            collectProgress(openSvn('up "%s"' % destination), progress)
        else:
            # No working copy- do a complete download
            self.download(destination, progress)


if __name__ == '__main__':
    import sys
    from StdProgress import StdProgress
    repo = SVNRepository(sys.argv[1])
    progress = StdProgress()
    progress.task('Checking for updates').report('result', repo.isUpdateAvailable(sys.argv[2]))
#    repo.update(sys.argv[2], progress.task('Testing CmdlineSVN update()'))
