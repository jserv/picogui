#!/usr/bin/env python
#
# Introduction to PicoGUI, originally written
# to run on the picogui.org VNC demo.
#
# This simple app runs a book-like demo where
# each page is an XWT file that is compiled and
# loaded as it's needed.
# A widgets name can tie it to one of several
# built-in event handlers.
#
# -- Micah Dowty
#
import PicoGUI
from PicoGUI import XWTParser
from PicoGUI import template

class xwtBook:
    def __init__(self, title, pageformat):
        self.pageformat = pageformat

        self.app = PicoGUI.Application(title)
        self.run = self.app.run

        # Build the shell that all the pages fit into
        self.navbar = self.app.addWidget('toolbar')
        self.navbar.side = 'bottom'
        self.status = self.navbar.addWidget('label', 'inside')
        self.status.side = 'all'
        self.status.font = ':0:bold'
        self.prev = self.navbar.addWidget('button', 'inside')
        self.prev.side = 'right'
        self.prev.text = 'Previous'
        self.app.link(self.prevHandler, self.prev, 'activate')
        self.next = self.navbar.addWidget('button', 'inside')
        self.next.side = 'right'
        self.next.text = 'Next'
        self.app.link(self.nextHandler, self.next, 'activate')
        
        # Scan for page files
        self.pageFiles = []
        n = 0
        try:
            while True:
                name = pageformat % n
                f = open(name,'r')
                f.close()
                self.pageFiles.append(name)
                n += 1
        except IOError:
            pass

        self.template = None
        self.showPage(0)

    def showPage(self, number):
        self.currentPage = number
        if self.template:
            self.template.destroy()

        # Parse, load, and attach the XWT
        file = self.pageformat % number
        xwt = open(file).read()
        wt = XWTParser.XWTParser().Parse(xwt)
        self.template = template.Template(self.app, wt)
        inst = self.template.instantiate()
        inst.side = 'all'
        inst.attach(self.navbar,'after')

        # Update the next and previous buttons
        self.prev.disabled = number == 0
        self.next.disabled = number == len(self.pageFiles)-1

        # Update the status line
        self.status.text = "Page %d of %d" % (number+1, len(self.pageFiles))

    def prevHandler(self, ev, widget):
        if self.currentPage != 0:
            self.showPage(self.currentPage-1)

    def nextHandler(self, ev, widget):
        if self.currentPage != len(self.pageFiles)-1:
            self.showPage(self.currentPage+1)


if __name__ == '__main__':
    xb = xwtBook('Welcome to PicoGUI', 'page%02d.xwt')
    xb.run()
