# wrapper Textbox class
from PicoGUI import Widget

class Textbox(Widget):
    def open(self, frame, page, buffer):
        self.tabpage = page
        self.side = 'All'
        page.textbox = self
        self.buffer = buffer
        self.frame = frame
        buffer.add_observer(self)

    def link(self, handler, evname=None):
        self.frame.link(handler, self, evname)
