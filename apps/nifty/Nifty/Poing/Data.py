from Nifty.Buffer import Buffer
from Views import Viewer
import persistence, transaction

class Field(persistence.Persistent):
    def __init__(self, name, widget='Field'):
        persistence.Persistent.__init__(self)
        self.name = name
        self.widget = widget

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
        o.update_from(self)

    def update_from(self, o):
        o.update(self)

    def save(self):
        transaction.get_transaction().commit()
        print 'Poing db saved'

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

    def addField(self, name, widget='Field'):
        try:
            self.getField(name)
        except KeyError:
            pass
        else:
            raise ValueError, 'duplicate field name %r' % name
        self.fields.append(Field(name, widget))

    def delField(self, name):
        for field in self:
            if field.name == name:
                self.fields.remove(field)
                return
        raise KeyError, name

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
