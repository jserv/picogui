from Nifty.Buffer import Buffer
from Views import SchemaViewer, Viewer
import persistence

class PoingEditable(persistence.Persistent, Buffer):
    def __init__(self, **kw):
        persistence.Persistent.__init__(self)
        Buffer.__init__(self)
        self.__dict__.update(kw)

    def __getstate__(self):
        state = persistence.Persistent.__getstate__(self)
        del state['observers']
        del state['python_ns']
        return state

    def __setstate__(self, state):
        persistence.Persistent.__setstate__(self, state)
        self.observers = []
        self.python_ns = {'buffer': self}

    def update_observer(self, o):
        o.update(self)

class Schema(PoingEditable):
    widget = SchemaViewer

    def __init__(self):
        PoingEditable.__init__(self)
        self.fields = persistence.list.PersistentList()

class Datum(PoingEditable):
    widget = Viewer

    def __init__(self, schema):
        if not isinstance(schema, Schema):
            raise TypeError, 'schema is not a Schema'
        PoingEditable.__init__(self, schema=schema)
