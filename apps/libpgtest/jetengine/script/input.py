# $Id: input.py,v 1.2 2002/11/26 19:18:07 micahjd Exp $
#
# input.py - Handlers for user input, based on input filters
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

class Input:
    def __init__(self, game):
        self.game = game
        self.app = game.app

        # Set up an input filter to catch all events that
        # aren't absorbed by any picogui widgets.
        # This adds a filter that recieves all events and absorbs none,
        # placed immediately after the input filter that dispatches pointing
        # events to picogui widgets.
        self.app.link(self.handler, self.app.addInfilter(
            self.app.server.getresource('infilter pntr dispatch')))

    def handler(self, t, sender):
        if t.dev == 'mouse':
            if t.name == 'scrollwheel':
                self.scrollWheel(t.y)
            if t.name == 'down':
                self.fire(t.x, t.y)
            if t.name == 'move':
                self.moveMouse(t.x, t.y)


    def scrollWheel(self, y):
        self.game.setVelocity(self.game.velocity - y/20.0)

    def fire(self, x, y):
        pass

    def moveMouse(self, x, y):
        pass
        
