import zodb.db, zodb.storage.file, persistence.list, transaction, os
import Nifty.config
from Views import Lister

# TODO: need to be able to delattr from DB
# TODO: this stuff should inherit from Buffer to get the whole observer thing

class DB(object):
    widget = Lister

    def __init__(self, fsname=None):
        if fsname is None:
            fsname = Nifty.config.get_config_file('poing.fs')
        self.__dict__['_db'] = zodb.db.DB(zodb.storage.file.FileStorage(fsname))
        self.__dict__['_r'] = self._db.open().root()
        self.__dict__['name'] = os.path.splitext(os.path.basename(fsname))[0]

    def abort(self):
        transaction.get_transaction().abort()

    def commit(self):
        transaction.get_transaction().commit()
        print 'Poing db saved'

    save = commit

    def __getattr__(self, name):
        return self._r[name]

    def __setattr__(self, name, value):
        self._r[name] = value
        self.commit()

    def dir(self):
        return [(n, o) for n, o in self._r.items() if getattr(o, 'widget', None)]

class Container(persistence.Persistent):
    widget = Lister

    def dir(self):
        return [(n, o) for n, o in self.__dict__.items()
                if getattr(o, 'widget', None) and n[0] != '_']

    def save(self):
        transaction.get_transaction().commit()
        print 'Poing db saved'

class List(persistence.list.PersistentList):
    widget = Lister

    def save(self):
        transaction.get_transaction().commit()
        print 'Poing db saved'
