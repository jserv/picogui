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

    def update_observer(self, o):
        o.text = self.text

    def update_from(self, o):
        self.text = o.text
