""" PGBuild.Repository.Subversion

Automatically chooses a Subversion implementation
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

import PGBuild.Errors

implementation = None

def Repository(config, url):
    """Repository factory function that automatically chooses an implementation module"""
    global implementation
    if not implementation:
        # Is an implementation being forced in the invocation options?
        if config.eval("invocation/option[@name='forceMiniSVN']/text()"):
            import MiniSVN
            implementation = MiniSVN

        # Automatically find one
        if not implementation:
            try:
                import CmdlineSVN
                implementation = CmdlineSVN
            except:
                try:
                    import MiniSVN
                    implementation = MiniSVN
                except:
                    raise PGBuild.Errors.EnvironmentError("No working Subversion implementation found")

    return implementation.Repository(config, url)

### The End ###
