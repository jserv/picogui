from Buffer import Buffer
import os

class FileBuffer(Buffer):
    "Represents a file"

    def __init__(self, path=''):
        self.path = path
        self.changed = False
        if os.path.exists(path):
            f = file(path, 'r')
            text=f.read()
            f.close()
        else:
            print 'New file'
            text = ''
        Buffer.__init__(self, name=path, text=text)

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
