# Application class

from Widget import Widget

class Application(Widget):
    def run(self):
        while 1:
            ev = self.server.wait()
            if ev.widget_id == self.handle and ev.name in ('close', 'stop'):
                return
