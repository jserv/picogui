# wrapper Terminal class
from Workspace import Workspace

class Terminal(Workspace):
    def open(self, frame, page, buffer):
        Workspace.open(self, frame, page, buffer)
        frame.link(buffer.notify_changed, self, 'data')
        frame.link(buffer.handle_resize, self, 'resize')
