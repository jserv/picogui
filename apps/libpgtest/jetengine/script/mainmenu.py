# $Id: mainmenu.py,v 1.3 2002/11/27 01:28:12 micahjd Exp $
#
# mainmenu.py - The game's main menu, based on Widget Templates
#
# Copyright (C) 2002 Micah Dowty and David Trowbridge
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#

import PicoGUI

class MainMenu:
    def __init__(self, game):
        self.game = game
        self.app = game.app
        self.show()

    def show(self):
        # Load our widget template, importing widgets from it
        self.template = self.app.newTemplate(open("data/mainmenu.wt").read())
        self.inst = self.template.instantiate([
            ])

    def hide(self):
        self.template.destroy()
