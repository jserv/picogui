from Nifty.Buffer import Buffer
from Views import Viewer
import persistence, transaction

class Field(persistence.Persistent):
    def __init__(self, name, widget=None):
        persistence.Persistent.__init__(self)
        self.name = name
        self.widget = widget or 'Field'

class PoingEditable(persistence.Persistent, Buffer):
    widget = Viewer

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
        o.update_view()

    def update_from(self, o):
        o.update_buffer()

    def save(self):
        transaction.get_transaction().commit()
        print 'Poing db saved'

    def Schema(self):
        raise NotImplemented

    def getField(self, name):
        raise NotImplemented

    def setField(self, name, value):
        raise NotImplemented

    def title(self):
        return type(self).__name__

class Schema(PoingEditable):
    def __init__(self):
        PoingEditable.__init__(self)
        self.fields = persistence.list.PersistentList()

    def __iter__(self):
        return iter(self.fields)

    def Schema(self):
        # metaschema - all widgets set to 'Field'
        return [Field(field.name) for field in self]

    def getField(self, name):
        # metaschema
        for field in self:
            if field.name == name:
                return field.widget
        raise KeyError, name

    def setField(self, name, value):
        # metaschema
        for field in self:
            if field.name == name:
                field.widget = value
                return
        raise KeyError, name

    def addField(self, name, widget=None):
        try:
            self.getField(name)
        except KeyError:
            pass
        else:
            raise ValueError, 'duplicate field name %r' % name
        self.fields.append(Field(name, widget))
        self._notify_siblings()

    def delField(self, name):
        for field in self:
            if field.name == name:
                self.fields.remove(field)
                self._notify_siblings()
                return
        raise KeyError, name

    def _notify_siblings(self):
        # look for buffers that have us as their schema and tell
        # them we changed
        frames = []
        for o in self.observers:
            if o.frame not in frames:
                frames.append(o.frame)
        for frame in frames:
            for ws in frame.workspaces():
                if getattr(ws.buffer, 'schema', None) is self:
                    ws.update_view()

class Datum(PoingEditable):
    def __init__(self, schema):
        if not isinstance(schema, Schema):
            raise TypeError, 'schema is not a Schema'
        PoingEditable.__init__(self, schema=schema)

    def Schema(self):
        return self.schema

    def getField(self, name):
        return getattr(self, name, '')

    def setField(self, name, value):
        setattr(self, name, value)

    def title(self):
        return self.schema.name
