# $Id: game.py,v 1.26 2002/11/26 23:03:05 micahjd Exp $
#
# game.py - The main module imported by the game engine,
#           and containing the thread() function it invokes.
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

import PicoGUI, math, time
import hud, input, mainmenu, console

print "Python module imported"

def thread():
    print "Starting game thread"

    # FIXME: This time.sleep seems to keep the two threads from
    #        fighting over the python lock a bunch and being slow
    #        for the first several seconds of execution.
    print "Unnecessary delay"
    time.sleep(1)
    game()


class game:
    def __init__(self):
        self.app = PicoGUI.InvisibleApp()

        self.camera = camera
        self.world = world
        self.skybox = skybox
        self.ship = ship
        
        self.input = input.Input(self)
        self.hud = hud.Hud(self)
        #self.mainmenu = mainmenu.MainMenu(self)

        self.setVelocity(10)
        self.setLasers(100)

        self.app.run()


    def toggleConsole(self):
        if hasattr(self,'console'):
            self.console.destroy()
            del self.console
        else:
            self.console = console.Console(self)


    def setVelocity(self, v):
        # Completely nonscientific equations to both set the velocity
        # of the moving texture, and stretch out the texture when going
        # fast, to give a magnified sense of motion.
        if v < 0.01:
            v = 0
        self.velocity = v
        self.world.velocity = v/10.0
        if v < 1:
            x = 1
        else:
            x = v
        self.world.vrepeat = self.world.hrepeat / x
        if hasattr(self,'hud'):
            self.hud.setVelocity(v)

    def setLasers(self, lasers):
        if hasattr(self,'hud'):
            self.hud.setLasers(lasers)
