""" PGBuild.UI.Curses

A Curses-based frontend for PGBuild
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

import PGBuild.UI.None
import PGBuild.Errors
import os, struct
try:
    import curses, termios, signal, fcntl
except ImportError:
    raise PGBuild.Errors.EnvironmentError("Curses doesn't seem to be installed")


class CursesWrangler:
    """Abstraction for our particular interface built with curses"""
    def __init__(self):
        try:
            self.stdscr = curses.initscr()
            curses.noecho()
            curses.cbreak()
            self.stdscr.keypad(1)
            signal.signal(signal.SIGWINCH, self.resize)
            self.resize()
        except:
            self.cleanup()
            raise

    def resize(self, signum=None, frame=None):
        (height, width) = self.getHeightWidth()
        
        self.taskWin = curses.newwin(10,width,0,0)
        self.taskWin.box()
        self.taskWin.refresh()
        
    def getHeightWidth(self):
        """ getwidth() -> (int, int)
            Return the height and width of the console in characters
            This is from:
            http://groups.google.com/groups?selm=uk7xtwasm.fsf%40python.net
            """
        try:
            return int(os.environ["LINES"]), int(os.environ["COLUMNS"])
        except KeyError:
            height, width = struct.unpack(
                "hhhh", fcntl.ioctl(0, termios.TIOCGWINSZ ,"\000"*8))[0:2]
            if not height: return 25, 80
            return height, width
    
    def cleanup(self):
        self.stdscr.keypad(0)
        curses.echo()
        curses.nocbreak()
        curses.endwin()


class Progress(PGBuild.UI.None.Progress):
    def _init(self):
        if self.parent:
            self.curses = self.parent.curses
        else:
            self.curses = CursesWrangler()

    def cleanup(self):
        self.curses.cleanup()
        

class Interface(PGBuild.UI.None.Interface):
    progressClass = Progress

    def cleanup(self):
        self.progress.cleanup()

### The End ###
        
    
