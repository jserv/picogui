# wrapper Textbox class
from Workspace import Workspace
import keybindings

def _clip_print(template, text):
    if len(text) > 23:
        text = text[:20] + '...'
    print template % text

class Textbox(Workspace):
    def open(self, frame, page, buffer):
        Workspace.open(self, frame, page, buffer)
        frame.link(self.handle_key, self, 'kbd keyup')
        self._partial_command = None
        self._keybindings = {}
        self._mark = None
        self._paste_offset = 1

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
    
    def set_mark(self):
        self._mark = self.cursor_position
        print 'mark set'

    def copy(self):
        if self._mark is None:
            print 'Mark is not set'
            return
        here = self.cursor_position
        low = min(self._mark, here)
        hi = max(self._mark, here)
        if low == hi:
            return
        text = self.text.split('\n')[low[0]:hi[0]+1]
        text[-1] = text[-1][:hi[1]]
        text[0] = text[0][low[1]:]
        text = '\n'.join(text)
        self.frame.copy(text)
        self._paste_offset = 1
        _clip_print('copied %r', text)
        return low, hi

    def cut(self):
        if self._mark is None:
            print 'Mark is not set'
            return
        i = self.copy()
        try:
            low, hi = i
        except TypeError:
            return
        del i
        text = self.text.split('\n')
        before = text[:low[0]+1]
        after = text[hi[0]:]
        after[0] = before.pop()[:low[1]] + after[0][hi[1]:]
        self.insertmode = 'overwrite'
        self.text = '\n'.join(before + after)
        self.frame.send(self, 'changed')

    def paste(self):
        data = self.frame.paste(self._paste_offset)
        if data:
            self.insertmode = 'atcursor'
            self.write(str(data))
            self.frame.send(self, 'changed')

    def rotate_paste(self):
        # the semantics of this method are not the same as emacs' yank-pop
        # (but that's fine with me)
        self._paste_offset += 1
        _clip_print('to paste: %r', self.frame.paste(self._paste_offset))
