from Buffer import TextBuffer
from Frame import file_detectors
import os

class FileBuffer(TextBuffer):
    "Represents a file"

    def __init__(self, path=''):
        self.path = os.path.expanduser(os.path.expandvars(path))
        self.basename = os.path.basename(self.path)
        self.changed = False
        if os.path.exists(self.path):
            f = file(self.path, 'r')
            text=f.read()
            f.close()
        else:
            print 'New file'
            text = ''
        TextBuffer.__init__(self, name=self.basename, text=text)

    def notify_changed(self, ev):
        old_text = self.text
        TextBuffer.notify_changed(self, ev)
        if (not self.changed) and old_text != self.text:
            self.changed = True
            self.change_name('*' + self.basename)

    def save(self):
        f = file(self.path, 'w')
        f.write(self.text)
        f.close()
        print 'buffer %r saved' % self.basename
        self.changed = False
        self.change_name(self.basename)

def detector(path):
    if not os.path.isdir(path):
        return FileBuffer(path)

file_detectors.append(detector)
