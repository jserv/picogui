class Buffer(object):
    "Represents something that may be displayed in a textbox; usually a file or similar"

    default_name = '__unnamed__'

    def __init__(self, name=None, text=''):
        if name is None:
            name = self.default_name
        self.name = name
        self.text = text
        self.observers = []

    def save(self):
        raise NotImplemented

    def notify_changed(self, ev):
        self.text = ev.widget.text
        for o in self.observers:
            if o is not ev.widget:
                o.text = self.text

    def add_observer(self, o):
        self.observers.append(o)
        o.link(self.notify_changed, 'changed')
        o.tabpage.text = self.name
        o.text = self.text

    def del_observer(self, o):
        self.observers.remove(o)

    def change_name(self, name):
        self.name = name
        for o in self.observers:
            o.tabpage.text = self.name
