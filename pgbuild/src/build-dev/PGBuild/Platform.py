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


# List of platform aliases- these will be looked up in the profile/platform
# tags, and they all default to PGBuild.platform
platformAliases = ['host', 'build', 'target']

# This specifies the format for platform specifiers. It's used for
# parsing platforms and representing them as strings.
platformFormat = ['operatingSystem', 'architecture', 'libc']

# Following suit with the os module
sep = "-"


class Platform(object):
    """Object to represent a set of platform specifiers"""
    def __init__(self, **kwargs):
        """This should be passed platform specifiers from platformFormat.
           Any specifiers not given are assumed None.
           """
        for arg in kwargs:
            if not arg in platformFormat:
                import PGBuild.Errors
                raise PGBuild.Errors.InternalError("%s is not a platform specifier" % arg)
        for specifier in platformFormat:
            try:
                setattr(self, specifier, kwargs[specifier])
            except KeyError:
                setattr(self, specifier, None)

    def __str__(self):
        specs = []
        for spec in platformFormat:
            specValue = getattr(self, spec)
            if specValue:
                specs.append(str(specValue))
        return sep.join(specs)


def evalPlatformAlias(ctx, name):
    """Resolve a platform alias into a Platform descriptor string"""
    value = ctx.config.eval("profile/platform[@name='%s']/text()" % name)
    if value:
        return value
    else:
        import PGBuild
        return PGBuild.platform


def parse(ctx, name):
    """Given a platform specification string, a platform alias,
       or a Platform instance, return a Platform object."""
    if isinstance(name, Platform):
        return name
    elif name in platformAliases:
        return parse(ctx, evalPlatformAlias(ctx, name))
    else:
        import re
        if re.search(r'\s', name):
            import PGBuild.Errors
            raise PGBuild.Errors.ConfigError('Platform name "%s" contains whitespace' % name)
        descriptors = str(name).split(sep)
        platform = Platform()
        for descriptorName in platformFormat:
            if not descriptors:
                break
            setattr(platform, descriptorName, descriptors[0])
            descriptors = descriptors[1:]
        if descriptors:
            import PGBuild.Errors
            raise PGBuild.Errors.ConfigError('Platform name "%s" contains too many descriptors' % name)
        return platform


def determinePlatform():
    """Guess the current system's platform. This will be run once at startup
       and stored in sys/platform by the config bootstrapping.
       """
    import os
    
    if os.name == 'posix':
        # On POSIX systems, we can get most of our info from uname
        uname = os.uname()
        return Platform(operatingSystem=uname[0].lower(),
                        architecture=uname[4].lower())

    else:
        # All other systems, we can only provide the OS name python gives us
        return Platform(operatingSystem=os.name.lower())
    

### The End ###
        
    
