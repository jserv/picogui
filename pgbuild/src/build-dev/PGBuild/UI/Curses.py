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

import PGBuild
import PGBuild.UI.None
import PGBuild.Errors
import os, struct
try:
    import curses, termios, signal, fcntl
except ImportError:
    raise PGBuild.Errors.EnvironmentError("Curses doesn't seem to be installed")


class Window(object):
    """Wrapper around a curses window, provides extra initialization and a heading"""
    def __init__(self, x, y, w, h, heading):
        self.win = curses.newwin(h,w,y,x)
        self.win.hline(0,0,curses.ACS_HLINE,w)
        self.win.setscrreg(1,h-1)
        self.win.idlok(1)
        self.win.scrollok(1)
        self.win.addstr(0, w - len(heading) - 10, " %s " % heading)
        self.win.move(1,1)
        self.win.refresh()

    def addLine(self, line):
        self.win.addstr(line + "\n")
        self.win.refresh()
        

class CursesWrangler(object):
    """Abstraction for our particular interface built with curses"""
    def __init__(self):
        try:
            self.stdscr = curses.initscr()
            curses.noecho()
            curses.cbreak()
            curses.curs_set(0)
            self.stdscr.keypad(1)
            signal.signal(signal.SIGWINCH, self.resize)
            self.resize()
        except:
            self.cleanup()
            raise
    
    def cleanup(self):
        self.stdscr.keypad(0)
        curses.echo()
        curses.nocbreak()
        curses.curs_set(1)
        curses.endwin()

    def resize(self, signum=None, frame=None):
        # Yucky hardcoded layout
        (height, width) = self.getHeightWidth()
        self.stdscr.addstr(0,0,"%s version %s - Curses frontend" % (PGBuild.name, PGBuild.version))
        self.stdscr.refresh()
        taskHeight = height/3
        self.taskWin = Window(0,1,width,taskHeight, "Active Tasks")
        self.reportWin = Window(0,taskHeight+1,width,height-taskHeight-1, "Progress")
        
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
        

class Progress(PGBuild.UI.None.Progress):
    def _init(self):
        if self.parent:
            self.curses = self.parent.curses
        else:
            self.curses = CursesWrangler()

    def cleanup(self):
        self.curses.cleanup()

    def _report(self, verb, noun):
        self.curses.reportWin.addLine(noun)
        

class Interface(PGBuild.UI.None.Interface):
    progressClass = Progress

    def cleanup(self):
        self.progress.cleanup()

### The End ###
        
    
