# Application class

import Widget, Server

class Application(Widget.Widget):
    _type = 1 # apptype parameter for the register request
    
    def __init__(self, title, server=None):
        if not server:
            server = Server.Server()
        Widget.Widget.__init__(self, server, server.register(title, self._type))
        self.default_relationship = 'inside'

    def run(self):
        self.server.update()
        while 1:
            ev = self.server.wait()
            if ev.widget_id == self.handle and ev.name in ('close', 'stop'):
                return

    def shutdown(self):
        self.server.close_connection()

class ToolbarApp(Application):
    _type = 2
