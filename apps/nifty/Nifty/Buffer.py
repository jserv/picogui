from Textbox import Textbox

class Buffer(object):
    "Represents something that may be displayed in a workspace; usually a file or similar"

    default_name = '__unnamed__'
    widget = Textbox

    def __init__(self, name=None, text=''):
        if name is None:
            name = self.default_name
        self.name = name
        self.text = text
        self.observers = []
        self.python_ns = {'buffer': self}

    def save(self):
        raise NotImplemented

    def update_observer(self, o):
        o.text = self.text

    def update_all_observers_but(self, but):
        for o in self.observers:
            if o is not but:
                self.update_observer(o)

    def notify_changed(self, ev):
        self.text = ev.widget.text
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
