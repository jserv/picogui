from Buffer import Buffer

class FileBuffer(Buffer):
    "Represents a file"

    def __init__(self, path=''):
        self.path = path
        f = file(path, 'r')
        Buffer.__init__(self, path, f.read())
        f.close()

    def save(self):
        f = file(self.path, 'w')
        f.write(self.text)
        f.close()
        print 'buffer %r saved' % self.name
