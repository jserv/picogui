# Package init for the high-level PicoGUI library
# This module is here only to re-export stuff from other modules

# The PicoGUI python client library is copyright (C) 2002
# by Lalo Martins <lalo@laranja.org> and may be freely distributable
# under the terms of the GNU Lesser Public License, version 2.1
# or later, as published by the Free Software Foundation. You should
# have received a copy of this license in the file COPYING, if you
# didn't they you didn't obtain this package legally and you should
# warn the person or site where you got it that it's illegal to
# distribute this code without the license file. (And in the meanwhile
# you can download it from http://picogui.org/ or http://PyPicoGUI.laranja.org/
# or download the license terms from http://www.gnu.org/)

import Server
server = Server
Server = server.Server
WTFile = server.WTFile
import Widget
widget = Widget
Widget = widget.Widget
import Application
application = Application
Application = application.Application
ToolbarApp = application.ToolbarApp
InvisibleApp = application.InvisibleApp
TemplateApp = application.TemplateApp
import Gropseq
gropseq = Gropseq
Gropseq = gropseq.Gropseq


