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
_svn_id = "$Id$"

import PGBuild
import PGBuild.UI.None
import PGBuild.Errors
import os, struct, time, threading
try:
    import curses, termios, signal, fcntl
except ImportError:
    raise PGBuild.Errors.EnvironmentError("Curses doesn't seem to be installed")


class Rect(object):
    """Rectangle wrapper that provides some layout functionality"""
    def __init__(self, x, y, w, h):
        self.x = x
        self.y = y
        self.w = w
        self.h = h

    def __str__(self):
        return "(%s, %s, %s, %s)" % (self.x, self.y, self.w, self.h)

    def __repr__(self):
        return "<%s.%s %s>" % (self.__module__, self.__class__.__name__, self)

    def set(self, r):
        """Set this rectangle to be equal to the given one"""
        self.x = r.x
        self.y = r.y
        self.w = r.w
        self.h = r.h

    def hSplit(self, topHeight):
        """Split the rectangle into two horizontally, given the height
           of the top window. Returns a tuple of the form (top, bottom)
           """
        return (Rect(self.x, self.y, self.w, topHeight),
                Rect(self.x, self.y+topHeight, self.w, self.h-topHeight))

    def vSplit(self, leftWidth):
        """Split the rectangle into two vertically, given the width
           of the left window. Returns a tuple of the form (left, right)
           """
        return (Rect(self.x, self.y, leftWidth, self.h),
                Rect(self.x+leftWidth, self.y, self.w-leftWidth, self.h))

    def sliceTop(self, topHeight):
        """Slice a rectangle off the top of this one, returning the new
           rectangle and shortening this rectangle.
           """
        (top, bottom) = self.hSplit(topHeight)
        self.set(bottom)
        return top

    def sliceBottom(self, bottomHeight):
        """Slice a rectangle off the bottom of this one, returning the new
           rectangle and shortening this rectangle.
           """
        (top, bottom) = self.hSplit(self.h-bottomHeight)
        self.set(top)
        return bottom

    def sliceLeft(self, leftWidth):
        """Slice a rectangle off the left side of this one, returning the new
           rectangle and shortening this rectangle.
           """
        (left, right) = self.vSplit(leftWidth)
        self.set(right)
        return left

    def sliceRight(self, rightWidth):
        """Slice a rectangle off the right side of this one, returning the new
           rectangle and shortening this rectangle.
           """
        (left, right) = self.vSplit(self.w-rightWidth)
        self.set(left)
        return right


class Window(object):
    """Base class for wrapping a curses window. Adds some initialization
       and constructs the window from a Rect object.
       """
    def __init__(self, rect):
        self.rect = rect
        self.win = curses.newwin(rect.h,rect.w,rect.y,rect.x)
        self.win.idlok(1)
        self.win.scrollok(1)

    def addText(self, text, x=None, y=None):
        """Add text at the given location. The text can be a string,
           a (string, attribute) tuple, or a sequence of such tuples.
           The "string" can also be an integer, to allow the use of
           special character codes curses defines.
           """
        if x != None:
            self.win.move(y,x)
        try:
            text[0]
        except IndexError:
            text = [text]
        for item in text:
            if type(item) == type(()):
                self.win.attrset(item[1])
                string = item[0]
            else:
                string = item
            if type(string) == int:
                self.win.addch(string)
            else:
                self.win.addstr(item)
            self.win.attrset(0)

    def clear(self):
        self.win.clear()
        self.win.move(0,0)
        

class Heading(Window):
    """Curses window providing a heading with left, right, or center justified text"""
    def __init__(self, rect, text, side='left',
                 bgChar=None, fgAttr=None, bgAttr=None, sideMargin=4):
        if not bgAttr:
            bgAttr = curses.color_pair(070)
        if not fgAttr:
            fgAttr = curses.color_pair(070)
        if not bgChar:
            bgChar = '-'
            
        Window.__init__(self, rect)
        self.sideMargin = sideMargin
        text = " %s " % text
        textX = getattr(self,'align_%s' % side)(rect.w, len(text))
        self.win.attrset(bgAttr)
        self.win.hline(0,0,bgChar,rect.w)
        self.win.attrset(fgAttr)
        self.win.addstr(0, textX, text)
        self.win.attrset(0)
        self.win.refresh()
        
    def align_left(self, container, text):
        return self.sideMargin

    def align_right(self, container, text):
        return container-text-self.sideMargin

    def align_center(self, container, text):
        return (container-text)/2

    
class ScrollingWindow(Window):
    """Window that scrolls new text in from the bottom """
    def __init__(self, rect):
        self.firstLine = 1
        Window.__init__(self, rect)

    def clear(self):
        Window.clear(self)
        self.firstLine = 1

    def addLine(self, line):
        if self.firstLine:
            self.firstLine = 0
        else:
            self.addText("\n")
        self.addText(line)
        self.win.refresh()

    def textBlock(self, text, attribute=0, bullet="*"):
        """Output a block of text in the given color attribute,
           with an optional preceeding bullet.
           """
        formatted = []
        stamp = str(Interface.timeStampClass())
        for line in text.split("\n"):
            formatted.append(stamp)
            formatted.append((" %s " % bullet, curses.A_BOLD))
            formatted.append((line, attribute))
            formatted.append("\n")
            bullet = ' '
        self.addLine(formatted[:-1])


class TaskWindow(Window):
    """Window that displays the list of active tasks"""
    def show(self, list):
        self.clear()
        attribute = 0
        for level in xrange(len(list)):
            if level == len(list)-1:
                attribute = curses.color_pair(006) | curses.A_BOLD
            task = list[level]
            self.addText(str(task.timeStamp) + " " * (level+1))
            self.addText(( (curses.ACS_HLINE, curses.A_BOLD),
                           " ", (list[level].taskName, attribute), "\n"))
        self.win.refresh()


class ClockWindow(Window):
    """Window that displays the current time,
       in the same format used for timestamps.
       """
    def __init__(self, rect, attr=None):
        Window.__init__(self, rect)
        if not attr:
            attr = curses.color_pair(070)
        self.attr = attr
        self.clear()
        self.win.bkgd(' ', self.attr)
        self.update()

    def update(self):
        self.addText(((str(Interface.timeStampClass()), self.attr),), 0, 0)
        self.win.refresh()


class ClockUpdater(threading.Thread):
    """Thread to update a ClockWindow"""
    def __init__(self):
        threading.Thread.__init__(self)
        # It is important to set the run flag here, even though we aren't
        # actually running yet. Consider what would happen if we set
        # running to 1 in run(), but CursesWrangler.cleanup() was called
        # immediately after calling ClockUpdater.start()- the thread could
        # be busy initializing when running is set to zero, then running
        # is set to 1 in run() and cleanup() hangs forever at the join().
        self.running = 1
        self.clocks = []
        
    def run(self):
        while self.running:
            time.sleep(1)
            for clock in self.clocks:
                clock.update()
       

class CursesWrangler(object):
    """Abstraction for our particular interface built with curses"""
    def __init__(self):
        try:
            self.cursesSem = threading.Semaphore()
            self.clockUpdater = ClockUpdater()

            self.cursesSem.acquire()
            self.stdscr = curses.initscr()
            curses.start_color()
            curses.noecho()
            curses.cbreak()
            curses.curs_set(0)
            self.stdscr.keypad(1)

            # Initialize all possible color pairs so we can specify
            # foreground and background colors in octal.
            for color in range(1,64):
                curses.init_pair(color, color & 7, color >> 3) 
            self.cursesSem.release()

            signal.signal(signal.SIGWINCH, self.resize)

            self.resize()
            self.clockUpdater.start()
        except:
            self.cleanup()
            raise

    def cleanup(self):
        try:
            self.clockUpdater.running = 0
            self.clockUpdater.join()
        except:
            pass
        self.cursesSem.acquire()
        self.stdscr.keypad(0)
        curses.echo()
        curses.nocbreak()
        curses.curs_set(1)
        curses.endwin()
        self.cursesSem.release()

    def resize(self, signum=None, frame=None):
        """Called on terminal resize, and once to get the initial size"""
        (self.height, self.width) = self.getHeightWidth()
        self.layout()

    def layout(self):
        """Set up us our windows, called whenever the size changes"""
        self.cursesSem.acquire()
        remaining = Rect(0,0,self.width,self.height)
        footer = Heading(remaining.sliceBottom(1),
                         "%s version %s - Curses frontend" % (PGBuild.name, PGBuild.version),'center', ' ')
        Heading(remaining.sliceTop(1), "Active Tasks")
        self.taskWin = TaskWindow(remaining.sliceTop(self.height / 4))
        Heading(remaining.sliceTop(1), "Messages")
        self.messageWin = ScrollingWindow(remaining.sliceTop(self.height / 4))
        Heading(remaining.sliceTop(1), "Progress")
        self.reportWin = ScrollingWindow(remaining)
        self.clock = ClockWindow(footer.rect.sliceRight(10))
        self.clockUpdater.clocks = [self.clock]
        self.cursesSem.release()

    def getHeightWidth(self):
        """ getHeightWidth() -> (int, int)
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
    """Progress reporter using Curses via the CursesWrangler"""
    def _init(self):
        if self.parent:
            self.curses = self.parent.curses
        else:
            self.curses = CursesWrangler()

    def _showTaskHeading(self):
        # Timestamp this task if we haven't seen it before
        self.curses.cursesSem.acquire()
        if not getattr(self, 'timeStamp', None):
            self.timeStamp = time.time()
        task = self
        list = []
        while task:
            if task.taskName:
                list.insert(0, task)
            task = task.parent
        self.curses.taskWin.show(list)
        self.curses.cursesSem.release()

    def cleanup(self):
        self.curses.cleanup()

    def _report(self, verb, noun):
        self.curses.cursesSem.acquire()
        stamp = str(Interface.timeStampClass())
        self.curses.reportWin.addLine(( ("%s %10s" % (stamp, verb),0),
                                        (" : ", curses.A_BOLD),
                                        (noun,0)
                                        ))
        self.curses.cursesSem.release()
        
    def _warning(self, text):
        self.curses.cursesSem.acquire()
        self.curses.messageWin.textBlock("Warning:\n" + text, curses.color_pair(003) | curses.A_BOLD)
        self.curses.cursesSem.release()
            
    def _error(self, text):
        self.curses.cursesSem.acquire()
        self.curses.messageWin.textBlock("Error:\n" + text, curses.color_pair(001) | curses.A_BOLD)
        self.curses.cursesSem.release()

    def _message(self, text):
        self.curses.cursesSem.acquire()
        self.curses.messageWin.textBlock(text)
        self.curses.cursesSem.release()        


class Interface(PGBuild.UI.None.Interface):
    """PGBuild Interface implementation using our Curses-based progress reporter"""
    progressClass = Progress

    def run(self, ctx):
        PGBuild.UI.None.Interface.run(self, ctx)
        print "Done."

    def cleanup(self, ctx):
        ctx.progress.cleanup()

    def exitWithError(self, ctx, message):
        """Exit with an error, ending the Curses environment
           and printing it using the Text UI
           """
        self.cleanup()
        import PGBuild.UI.Text, sys
        PGBuild.UI.Text.Progress().error(message)
        sys.exit(1)
    
### The End ###
        
    
