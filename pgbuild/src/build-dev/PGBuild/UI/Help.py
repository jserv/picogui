""" PGBuild.UI.Help

The 'help' UI, that just lists the available UIs and exits
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

# We'll base this off the Text UI so we can use it's purdy Progress class
import PGBuild.UI.Text

class Interface(PGBuild.UI.Text.Interface):
    def run(self):
        text = "Available UI modules:\n\n"
        for module in PGBuild.UI.catalog:
            text += "%10s: %s\n" % module
        self.progress.message(text[:-1])
        
### The End ###
        
    
