""" PGBuild.Errors

Defines common exception classes for PGBuild
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

# A little explanation is in order for this code.
# As of this writing, Gentoo Linux includes a broken _xmlplus module.
# This wouldn't be so bad, since we only need core XML functionality,
# except that the xml module transparently replaces itself with
# _xmlplus if it's available. See this writeup on the "_xmlplus hack":
#    http://www.amk.ca/conceit/xmlplus.html
#
# So, in the interest of portability, (and so I don't have to wait for
# Gentoo to fix the bug) this wrapper exists. It performs a simple test
# of the xml module, and if it fails it uses a similarly hackish trick
# to prevent the xml module from loading _xmlplus.

class InternalError(Exception):
    def __init__(self, args=None):
        self.args = args

class UserError(Exception):
    def __init__(self, args=None):
        self.args = args

class EnvironmentError(Exception):
    def __init__(self, args=None):
        self.args = args

### The End ###
        
    
