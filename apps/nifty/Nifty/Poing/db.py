import zodb.db, zodb.storage.file, persistence.list, transaction
import Nifty.config

class DB(object):
    def __init__(self, fsname=None):
        if fsname is None:
            fsname = Nifty.config.get_config_file('poing.fs')
        self.__dict__['_db'] = zodb.db.DB(zodb.storage.file.FileStorage(fsname))
        self.__dict__['_r'] = self._db.open().root()

    def abort(self):
        transaction.get_transaction().abort()

    def commit(self):
        transaction.get_transaction().commit()

    def __getattr__(self, name):
        return self._r[name]

    def __setattr__(self, name, value):
        self._r[name] = value
        self.commit()

class Container(persistence.Persistent):
    pass

List = persistence.list.PersistentList
