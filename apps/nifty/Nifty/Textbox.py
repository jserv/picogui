# wrapper Textbox class
from PicoGUI import Widget
from PicoGUI.keys import names as key_names
import keybindings

class Textbox(Widget):
    def open(self, frame, page, buffer):
        self.tabpage = page
        self.side = 'All'
        page.textbox = self
        self.buffer = buffer
        self.frame = frame
        buffer.add_observer(self)
        frame.link(self.handle_key, self, 'kbd keyup')
        self._partial_command = None

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

    def handle_key(self, ev):
        key = self._resolve_key(ev)
        if key is None: return
        seq = (key,)
        if self._partial_command:
            seq = self._partial_command + seq
        self._partial_command = None
        self.extdevents = None
        cmd = keybindings.resolve(seq)
        if type(cmd) is dict:
            self._partial_command = seq
            self.extdevents = 'kbd'
            return
        if type(cmd) is str:
            try:
                exec cmd in self.frame.python_ns, self.buffer.python_ns
            except SystemExit:
                raise
            except:
                import traceback
                traceback.print_exc()
