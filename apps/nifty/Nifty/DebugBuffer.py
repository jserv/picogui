from Buffer import Buffer

class DebugBuffer(Buffer):
    "A buffer to redirect stderr to"

    def __init__(self, frame):
        self.frame = frame
        import sys
        self.stderr = sys.stderr
        Buffer.__init__(self, '__debug__', 'Debugging information (sys.stderr):\n')

    def save(self):
        print 'cannot save a DebugBuffer'

    def add_observer(self, o):
        Buffer.add_observer(self, o)
        o.readonly = 1
        o.insertmode = 'append'

    def write(self, text):
        self.stderr.write(text)
        self.text += text
        if self.observers:
            for o in self.observers:
                o.readonly = 0
                o.stream(text)
                o.readonly = 1
        else:
            self.frame.open(self)
            self.frame.current = -1