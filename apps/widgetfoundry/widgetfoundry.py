#!/usr/bin/env python
# $Id: widgetfoundry.py,v 1.6 2002/11/15 07:38:21 micahjd Exp $
#
# widgetfoundry.py - Main module for the Widget Foundry WT editor
#
# PicoGUI small and efficient client/server GUI
# Copyright (C) 2000-2002 Micah Dowty <micahjd@users.sourceforge.net>
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
import Components, CommandLine, Toolbox, WTWidget, XWT


class Main:
    def __init__(self):
        self.app = PicoGUI.Application('Widget Foundry')
        self.selectNotify = {}
        self.changeNotify = {}
        self.selection = None

        # Give the window an initial size if it supports it
        try:
            self.app.width  = 800
            self.app.height = 600
        except PicoGUI.responses.ParameterError:
            pass

        self.toolbar = Components.Toolbar(self)
        self.workArea = WorkArea(self)

        self.toolbar.add([
            ('Toolbox',        Components.toggleComponent, {
                'extdevents' : 'toggle',
                'component'  : Toolbox.Toolbox,
                'attr'       : 'toolbox'
                }),
            ('Command Line',   Components.toggleComponent, {
                'extdevents' : 'toggle',
                'component'  : CommandLine.CommandLine,
                'attr'       : 'commandLine'
                }),
            ('XWT View',       Components.toggleComponent, {
                'extdevents' : 'toggle',
                'component'  : XWT.XWTView,
                'attr'       : 'xwtView'
                }),
            ('Export WT',      XWT.ExportWT, {
                'side'       : 'right',
                }),
            ('Save XWT',       XWT.Save, {
                'side'       : 'right',
                }),
            ('Load XWT',       XWT.Load, {
                'side'       : 'right',
                }),
            ])

    def select(self, widget):
        for i in self.selectNotify.keys():
            self.selectNotify[i](self.selection,widget)
        self.selection = widget


class WorkArea:
    "This is where widget templates are constructed"
    def __init__(self, main):
        self.root = WTWidget.Widget(main.app,main.toolbar.widget.addWidget('box'))
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
       

if __name__ == '__main__':
    Main().app.run()
