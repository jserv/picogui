# wrapper Textbox class
from Workspace import Workspace
import keybindings

class Textbox(Workspace):
    def open(self, frame, page, buffer):
        Workspace.open(self, frame, page, buffer)
        frame.link(self.handle_key, self, 'kbd keyup')
        self._partial_command = None
        self._keybindings = {}

    def bind_key(self, keyseq, command, b_global=False):
        if b_global:
            keybindings.bind(keyseq, command)
        else:
            keybindings.bind(keyseq, command, self._keybindings)

    def handle_key(self, ev):
        key = self._resolve_key(ev)
        if key is None: return
        seq = (key,)
        if self._partial_command:
            seq = self._partial_command + seq
        self._partial_command = None
        self.extdevents = None
        cmd = keybindings.resolve(seq, (self._keybindings, keybindings.default))
        if type(cmd) is dict:
            self._partial_command = seq
            self.extdevents = 'kbd'
            return
        if type(cmd) is str:
            self.buffer.python_ns['workspace'] = self
            try:
                exec cmd in self.frame.python_ns, self.buffer.python_ns
            except SystemExit:
                raise
            except:
                import traceback
                traceback.print_exc()
        if callable(cmd):
            cmd(self)
