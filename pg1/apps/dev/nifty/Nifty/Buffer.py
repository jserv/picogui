from Workspace import Workspace
from Textbox import Textbox

class Buffer(object):
    "Represents something that may be displayed in a workspace"

    default_name = '__unnamed__'
    widget = 'Box'

    def __init__(self, name=None):
        if name is None:
            name = self.default_name
        self.name = name
        self.observers = []

    def save(self):
        raise NotImplemented

    def update_observer(self, o):
        raise NotImplemented

    def update_all_observers_but(self, but):
        for o in self.observers:
            if o is not but:
                self.update_observer(o)

    def update_from(self, o):
        raise NotImplemented

    def notify_changed(self, ev):
        self.update_from(ev.widget)
        self.update_all_observers_but(ev.widget)

    def add_observer(self, o):
        self.observers.append(o)
        o.link(self.notify_changed, 'changed')
        o.tabpage.text = self.name
        self.update_observer(o)

    def del_observer(self, o):
        self.observers.remove(o)

    def change_name(self, name):
        self.name = name
        for o in self.observers:
            o.tabpage.text = self.name


class TextBuffer(Buffer):
    "A buffer that makes sense in a textbox; usually a file or similar"

    widget = Textbox

    def __init__(self, name=None, text=''):
        Buffer.__init__(self, name)
        self.text = text
        self.python_ns = {'buffer': self}
        self.autoindent = False

    def update_observer(self, o):
        where = o.cursor_position
        o.text = self.text
        o.cursor_position = where

    def update_from(self, o):
        self.text = o.text[:-1] # remove the extra \n inserted by picogui

    def sort(self):
        lines = self.text.split('\n')
        lines.sort()
        self.text = '\n'.join(lines)
        self.update_all_observers_but(None)

    def _get_indentation(self, lines, index, unindent=False):
        if unindent or index == 0:
            return 0
        prevl = lines[index-1].expandtabs()
        return len(prevl) - len(prevl.lstrip())

    def indent(self, observer):
        self.update_from(observer)
        line = observer.cursor_position[0]
        lines = self.text.split('\n')
        indentation = self._get_indentation(lines, line)
        lines[line] = (' ' * indentation) + lines[line].lstrip()
        self.text = '\n'.join(lines)
        self.update_all_observers_but(None)
        observer.cursor_position = (line, indentation)

    def indent_if_auto(self, observer):
        if self.autoindent:
            self.indent(observer)

    def unindent_if_at_start(self, workspace):
        self.update_from(observer)
        line, column = observer.cursor_position[0]
        lines = self.text.split('\n')
        for char in lines[line][:column]:
            if not char.isspace():
                return
        indentation = self._get_indentation(lines, line, True)
        lines[line] = (' ' * indentation) + lines[line].lstrip()
        self.text = '\n'.join(lines)
        self.update_all_observers_but(None)
        observer.cursor_position = (line, indentation)
