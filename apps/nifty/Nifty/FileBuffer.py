from Buffer import Buffer

class FileBuffer(Buffer):
    "Represents a file"

    def __init__(self, path=''):
        self.path = path
        self.changed = False
        f = file(path, 'r')
        Buffer.__init__(self, name=path, text=f.read())
        f.close()

    def notify_changed(self, ev):
        Buffer.notify_changed(self, ev)
        self.changed = True
        self.change_name('*' + self.path)

    def save(self):
        f = file(self.path, 'w')
        f.write(self.text)
        f.close()
        print 'buffer %r saved' % self.name
        self.changed = False
        self.change_name(self.path)
