# wrapper Textbox class
from Workspace import Workspace
import keybindings

class Textbox(Workspace):
    def open(self, frame, page, buffer):
        Workspace.open(self, frame, page, buffer)
        frame.link(self.handle_key, self, 'kbd keyup')
        self._partial_command = None

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
