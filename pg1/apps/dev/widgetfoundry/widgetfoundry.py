#!/usr/bin/env python
# $Id$
#
# widgetfoundry.py - Main module for the Widget Foundry WT editor
#
# PicoGUI small and efficient client/server GUI
# Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
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
import PicoGUI, sys
import WTWidget
from code import InteractiveConsole


class Main:
    def __init__(self):
        self.app = PicoGUI.TemplateApp(open('main.wt').read(),[
            'WidgetTree',
            'Properties',
            'WidgetPalette',
            'PythonPrompt',
            'PythonCommand',
            'PythonConsole',
            'WorkArea',
            'XWTViewer',
            'WidgetInfo',
            ])
        self.selectNotify = {}
        self.changeNotify = {}
        self.selection = None

        self.workArea = WorkArea(self)
        self.propertyBox = PropertyBox(self)
        self.commandLine = CommandLine(self)
        self.xwtView = XWTView(self)
        
    def select(self, widget):
        for i in self.selectNotify.keys():
            self.selectNotify[i](self.selection,widget)
        self.selection = widget


class WorkArea:
    "This is where widget templates are constructed"
    def __init__(self, main):
        self.root = WTWidget.Widget(main.app,main.app.WorkArea.addWidget('box','inside'))
        self.root.side = 'all'
        self.main = main
        self.root.changeNotify = self.changeNotify
        
        w1 = self.root.addWidget('label','inside')
        w2 = w1.addWidget('label')
        w3 = w2.addWidget('button')

        w1.text = "This..."
        w2.text = "... is a test!"
        w3.text = "Yay"
        w3.side = "top"
        
        main.select(w3)

    def changeNotify(self, widget, property):
        for i in self.main.changeNotify.keys():
            self.main.changeNotify[i](widget,property)

       
class PropertyBox:
    "List of properties for the selected widget"
    def __init__(self, main):
        self.main = main
        self.main.selectNotify[self] = self.selectNotify
        self.main.changeNotify[self] = self.changeNotify
        self.selectNotify(None, self.main.selection)

    def selectNotify(self, previous, current):
        if hasattr(self,'propList'):
            self.propList.destroy()
        if current:
            self.propList = WTWidget.PropertyList(self.main.app, current, self.main.app.Properties)
            self.updateInfo()

    def changeNotify(self, widget, property):
        if widget == self.main.selection and property == 'name':
            self.updateInfo()
            
    def updateInfo(self):
        if self.main.selection.properties.has_key('name'):
            name = repr(self.main.selection.name)
        else:
            name = '<anonymous>'
        self.main.app.WidgetInfo.text = "%s widget: %s" % (self.main.selection.wtype.name, name)


class CommandLine(InteractiveConsole):
    def __init__(self, main):
        # Redirect stdout and stderr to our own write() function
        main.app.PythonConsole.write = main.app.PythonConsole.stream
        sys.stdout = main.app.PythonConsole
        sys.stderr = main.app.PythonConsole
        
        locals = {
            "__name__":   "__console__",
            "__doc__":    None,
            "main":       main,
            "PicoGUI":    PicoGUI,
            "sys":        sys,
            }
        InteractiveConsole.__init__(self,locals)

        try:
            sys.ps1
        except AttributeError:
            sys.ps1 = ">>> "
        try:
            sys.ps2
        except AttributeError:
            sys.ps2 = "... "

        self.main = main
        self.prompt = sys.ps1
        main.app.PythonPrompt.text = self.prompt
        main.app.link(self.enterLine, main.app.PythonCommand, 'activate')

        print "Python %s on %s\n(Widget Foundry shell, See main.__dict__ for useful variables)\n" %\
              (sys.version, sys.platform)

    def enterLine(self, ev, widget):
        line = widget.text[:]
        print self.prompt + line
        if self.push(line):
            self.prompt = sys.ps2
        else:
            self.prompt = sys.ps1
        self.main.app.PythonPrompt.text = self.prompt
        widget.text = ''
        

class XWTView:
    def __init__(self, main):
        self.main = main
        self.main.changeNotify[self] = self.changeNotify
        self.update()

    def changeNotify(self, widget, property):
        self.update()

    def update(self):
        self.main.app.XWTViewer.stream(self.main.workArea.root.toXWT())


if __name__ == '__main__':
    saved_stdout = sys.stdout
    saved_stderr = sys.stderr
    try:
        Main().app.run()
    finally:
        sys.stdout = saved_stdout
        sys.stderr = saved_stderr
