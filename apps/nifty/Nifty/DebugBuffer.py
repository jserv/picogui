from Buffer import TextBuffer

class DebugBuffer(TextBuffer):
    "A buffer to redirect stderr to"

    def __init__(self, frame):
        self.frame = frame
        TextBuffer.__init__(self, '__debug__', 'Debugging information (sys.stderr):\n')

    def save(self):
        print 'cannot save a DebugBuffer'

    def add_observer(self, o):
        TextBuffer.add_observer(self, o)
        o.readonly = 1
        o.insertmode = 'append'

    def write(self, text):
        self.text += text
        if self.observers:
            for o in self.observers:
                o.readonly = 0
                o.write(text)
                o.readonly = 1
                if getattr(o, 'frame', None) is self.frame:
                    self.frame.current = o
        else:
            self.frame.open(self)
            self.frame.current = -1
