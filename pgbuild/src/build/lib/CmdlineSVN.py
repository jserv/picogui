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

import os

# The name of the svn command to use. This should be made customizable.
svnCommand = "svn"

class CmdlineSVNClientBroken(Exception):
    pass

def detectVersion():
    s = os.popen("%s --version" % svnCommand)
    ver = s.read()
    s.close()
    if ver.find("Subversion") < 0:
        raise CmdlineSVNClientBroken
    return ver

def runCommand(args):
    global svnCommand
    print os.system('%s --non-interactive %s' % (svnCommand, args))


# Since exceptions during import will be used to autodetect which Subversion
# module to use, we want to cause an exception here if it looks like the
# command line client is missing or broken.
version = detectVersion()


class SVNRepository:
    def __init__(self, url):
        self.url = url

    def download(self, destination):
        runCommand('co "%s" "%s"' % (self.url, destination))

    def isUpdateAvailable(self, destination):
        pass
            
    def update(self, destination):
        runCommand('up "%s"' % destination)


if __name__ == '__main__':
    import sys
    repo = SVNRepository(sys.argv[1])
    repo.update(sys.argv[2])
