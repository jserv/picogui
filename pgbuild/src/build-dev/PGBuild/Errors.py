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
_svn_id = "$Id$"

class Error(Exception):
    def __init__(self, args=None):
        self.args = args

class InternalError(Error):
    """Internal errors are generally ignored by PGBuild, so that
       the full trace is available without any fidgeting.
       """
    def __init__(self, args=None):
        self.args = args

class ExternalError(Error):
    """External errors are generally converted into a more
       user-friendly form so that trivial problems don't scare
       users away :)
       """
    def __init__(self, args=None):
        self.args = args

class InterruptError(ExternalError):
    def __init__(self, args="The build was interrupted"):
        self.args = args

class ConfigError(ExternalError):
    explanation = "There was a problem with the configuration"
    def __init__(self, args=None):
        self.args = args

class UserError(ExternalError):
    explanation = "A problem was found with the user-supplied settings"
    def __init__(self, args=None):
        self.args = args

class EnvironmentError(ExternalError):
    explanation = "There was a problem with the build environment"
    def __init__(self, args=None):
        self.args = args

### The End ###
        
    
