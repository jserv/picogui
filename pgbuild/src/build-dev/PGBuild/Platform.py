""" PGBuild.Platform

Module for working with platform names. Includes facilities to
guess the platform of the build host, and look up platforms
specified in the configuration tree.

The terminology used here tries to mirror GNU closely, however
the actual platform specifications are a little less verbose to
keep the binary directories from being hopelessly annoying :)

Platform aliases:

  host   - The platform a binary is intended to run on
  build  - The platform a binary is being compled on
  target - The platform a binary procuces output for (used for build tools)

Platform specifications:

  OperatingSystem-Architecture-LibC

  The specification can be truncated at any separator, when only
  general information about the platform is known. More detailed
  optional specifiers can be added to the end.

OS-specific information:

  posix:
  
    On platforms that Python reports as 'posix', this module will
    use the 'uname' command to dig a little deeper. It should always
    be able to detect the operating system and architecture:

    - OperatingSystem will always be a lowercased version of `uname`
    - Architecture will always be a lowercased version of `uname -m`
    - LibC will not be autodetected

  other:

    Any platforms not specifically handled here will have their
    OperatingSystem set to os.name and all other specifiers blank.
    
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
_svn_id = "$Id: __init__.py 4093 2003-05-30 05:33:04Z micah $"


platformAliases = ['host', 'build', 'target']


class Platform(object):
    """Object to represent a set of platform specifiers"""
    def __init__(self, operatingSystem, architecture=None, libC=None):
        self.operatingSystem = operatingSystem
        self.architecture = architecture
        self.libC = libC

    def __str__(self):
        spec = self.operatingSystem
        if self.architecture:
            spec += '-' + self.architecture
        if self.libC:
            spec += '-' + self.libC
        return spec


def evalPlatformAlias(config, name):
    """Resolve a platform alias (recursively if necessary) to a Platform"""
    if name in platformAliases:
        # See if we have a matching invocation option
        opt = config.eval("invocation/option[@name='%sPlatform']/text()" % name)
        if not opt:
            opt = config.eval("sys/platform/text()")
        return parse(opt)
    


def parse(name):
    """Given a platform specification string, a platform alias,
       or a Platform instance, construct a Platform object"""

    
    pass


def guess():
    """Guess the current system's platform"""
    import os
    
    if os.name == 'posix':
        # On POSIX systems, we can get most of our info from uname
        uname = os.uname()

        # Heuristics to detect the standard library in use
        libC = None
        try:
            import dl
            libc = dl.open("libc.so.6")
            x.call("gnu_get_libc_version")
            toolchain = "gnu"
        except:
            pass
        return Platform(uname[0].lower(), uname[4].lower(), libC)

    else:
        # All other systems, we can only provide the OS name python gives us
        return Platform(os.name.lower())
    

### The End ###
        
    
