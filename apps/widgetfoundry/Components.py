# $Id: Components.py,v 1.2 2002/11/15 07:38:21 micahjd Exp $
#
# Components.py - Base for Widget Foundry's UI components
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


class PanelComponent:
    "Generic toggleable component in a panel"
    def _close(self, ev, widget):
        self.destroy()

    def __init__(self, main):
        self.widget = main.toolbar.widget.addWidget('panel')
        self.main = main
        self.widget.sizemode = 'percent'
        self.widget.size = 30
        main.app.link(self._close, self.widget, 'close')

    def destroy(self):
        self.main.app.delWidget(self.widget)
        self.toggleButton.on = 0


def toggleComponent(ev, widget):
    "Handler that can toggle on and off any UI component"
    if widget.on:
        component = widget.component(widget.main)
        component.toggleButton = widget
        widget.main.__dict__[widget.attr] = component
    else:
        widget.main.__dict__[widget.attr].destroy()
        del widget.main.__dict__[widget.attr]


class Toolbar:
    def __init__(self, main):
        "Create a toolbar with the buttons listed above"
        self.widget = main.app.addWidget('toolbar')
        self.main = main

    def add(self, buttons):
        buttons.reverse()
        for button in buttons:
            w = self.widget.addWidget('button','inside')
            w.text = button[0]
            self.main.app.link(button[1], w, 'activate')
            proplist = button[2]
            w.main = self.main
            for prop in proplist.keys():
                setattr(w,prop,proplist[prop])
