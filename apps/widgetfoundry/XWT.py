# $Id: XWT.py,v 1.4 2002/11/15 07:38:21 micahjd Exp $
#
# XWT.py - Utilities for dealing with widget template files
#          in the Widget Foundry UI
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
from Components import PanelComponent
from PicoGUI import XWTParser

def Load(ev, button):
    print "not here yet"
    pass


def Save(ev, button):
    xwt = button.main.workArea.root.toXWT()
    file = open('test.xwt','w')
    file.write(xwt)
    file.close()
    print "Wrote test.xwt"
    pass


def ExportWT(ev, button):
    xwt = button.main.workArea.root.toXWT()
    parser = XWTParser.XWTParser()
    wt = parser.Parse(xwt)
    file = open('test.wt','wb')
    file.write(wt)
    file.close()
    print "Wrote test.wt"
    pass


class XWTView(PanelComponent):
    "View, load, save, import, and export the XWT tree"
    def __init__(self, main):
        PanelComponent.__init__(self,main)
        self.widget.side = 'bottom'
        self.widget.text = 'XWT Viewer'

        self.main.changeNotify[self] = self.changeNotify

        self.scroll = self.widget.addWidget('scrollbox','inside')
        self.viewer = self.scroll.addWidget('textbox','inside')
        self.viewer.side = 'all'
        self.viewer.readonly = 1
        self.update()

    def changeNotify(self, widget, property):
        self.update()

    def update(self):
        # Create and delete the handle ourselves,
        # since we don't want to be leaking a big XWT string
        # handle every time this updates.
        xwt = self.main.workArea.root.toXWT()
        hstring = self.main.app.server.mkstring(xwt)
        self.viewer.text = hstring
        self.main.app.server.free(hstring)

    def destroy(self):
        self.main.app.delWidget(self.scroll)
        self.main.app.delWidget(self.viewer)
        PanelComponent.destroy(self)

