class Buffer(object):
    "Represents something that may be displayed in a textbox; usually a file or similar"

    def __init__(self, name='', text=''):
        self.name = name
        self.text = text
        #self.observers = []

    def save(self):
        raise NotImplemented
