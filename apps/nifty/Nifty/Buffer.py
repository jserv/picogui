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

    def add_observer(self, o):
        self.observers.append(o)

    def del_observer(self, o):
        self.observers.remove(o)
