# wrapper Textbox class
from PicoGUI import Widget
from PicoGUI.keys import names as key_names

class Workspace(Widget):
    widget_type = 'Box'
    
    def open(self, frame, page, buffer):
        self.tabpage = page
        self.side = 'All'
        page.workspace = self
        self.buffer = buffer
        self.frame = frame
        if hasattr(buffer, 'add_observer'):
            buffer.add_observer(self)

    def link(self, handler, evname=None):
        self.frame.link(handler, self, evname)

    def _resolve_key(self, event):
        if event.data in range(300, 311):
            # modifier key
            return None
        name = key_names.get(event.data, None) or '<%r>' % event.data
        if event.hasMod('shift'):
            name = name.upper()
        if event.hasMod('ctrl'):
            name = 'C-' + name
        if event.hasMod('alt'):
            name = 'A-' + name
        if event.hasMod('meta'):
            name = 'M-' + name
        return name
